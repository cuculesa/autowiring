// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "CoreContext.h"
#include "AutoInjectable.h"
#include "AutoPacketFactory.h"
#include "CoreContextStateBlock.h"
#include "CoreThread.h"
#include "demangle.h"
#include "GlobalCoreContext.h"
#include "JunctionBox.h"
#include "ManualThreadPool.h"
#include "MicroBolt.h"
#include "NullPool.h"
#include "SystemThreadPool.h"
#include "thread_specific_ptr.h"
#include "ThreadPool.h"
#include <sstream>
#include <stack>
#include <stdexcept>

using namespace autowiring;

class DelayedContextHold:
  public CoreRunnable
{
public:
  DelayedContextHold(std::shared_ptr<CoreContext> context):
    m_context(context)
  {}

  // A pointer to the enclosing context.  This pointer is held until this instance is started or stopped
  std::shared_ptr<CoreContext> m_context;

  bool OnStart(void) override {
    m_context.reset();
    return false;
  }

  void OnStop(bool) override {
    m_context.reset();
  }
};

/// <summary>
/// A pointer to the current context, specific to the current thread.
/// </summary>
/// <remarks>
/// All threads have a current context, and this pointer refers to that current context.  If this value is null,
/// then the current context is the global context.  It's very important that threads not attempt to hold a reference
/// to the global context directly because it could change teardown order if the main thread sets the global context
/// as current.
/// </remarks>
static thread_specific_ptr<std::shared_ptr<CoreContext>> autoCurrentContext;

// Peer Context Constructor. Called interally by CreatePeer
CoreContext::CoreContext(const std::shared_ptr<CoreContext>& pParent, t_childList::iterator backReference) :
  m_pParent(pParent),
  m_backReference(backReference),
  m_stateBlock(std::make_shared<CoreContextStateBlock>(pParent ? pParent->m_stateBlock : nullptr)),
  m_junctionBoxManager(new JunctionBoxManager),
  m_threadPool(std::make_shared<NullPool>())
{}

CoreContext::~CoreContext(void) {
  // Evict from the parent's child list first, if we have a parent:
  if(m_pParent)
  {
    // Notify AutowiringEvents listeners
    GetGlobal()->Invoke(&AutowiringEvents::ExpiredContext)(*this);

    // Also clear out any parent pointers:
    std::lock_guard<std::mutex> lk(m_pParent->m_stateBlock->m_lock);
    m_pParent->m_children.erase(m_backReference);
  }

  // The autoCurrentContext pointer holds a shared_ptr to this--if we're in a dtor, and our caller
  // still holds a reference to us, then we have a serious problem.
  assert(
    !autoCurrentContext.get() ||
    !autoCurrentContext.get()->use_count() ||
    autoCurrentContext.get()->get() != this
  );

  // Notify all ContextMember instances that their parent is going away
  NotifyTeardownListeners();

  // Make sure events aren't happening anymore:
  UnregisterEventReceiversUnsafe();

  // Tell all context members that we're tearing down:
  for(ContextMember* q : m_contextMembers)
    q->NotifyContextTeardown();
}

std::shared_ptr<CoreContext> CoreContext::CreateInternal(t_pfnCreate pfnCreate) {
  return CreateInternal(pfnCreate, AutoInjectable());
}

std::shared_ptr<CoreContext> CoreContext::CreateInternal(t_pfnCreate pfnCreate, AutoInjectable&& inj)
{
  // don't allow new children if shutting down
  if(IsShutdown())
    throw autowiring_error("Cannot create a child context; this context is already shut down");
    
  t_childList::iterator childIterator;
  {
    // Lock the child list while we insert
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

    // Reserve a place in the list for the child
    childIterator = m_children.insert(m_children.end(), std::weak_ptr<CoreContext>());
  }

  // Create the shared pointer for the context--do not add the context to itself,
  // this creates a dangerous cyclic reference.
  std::shared_ptr<CoreContext> retVal = pfnCreate(shared_from_this(), childIterator);

  // Notify AutowiringEvents listeners
  GetGlobal()->Invoke(&AutowiringEvents::NewContext)(*retVal);
  
  // Remainder of operations need to happen with the created context made current
  CurrentContextPusher pshr(retVal);

  // If we're currently running, we would like the child context to know that it can optionally
  // transition directly to the running state without having to wait in Initiated
  if (IsRunning())
    retVal->m_state = State::CanRun;

  // Inject before broadcasting the creation notice
  inj();

  // Fire all explicit bolts if not an "anonymous" context (has void sigil type)
  BroadcastContextCreationNotice(retVal->GetSigilType());

  // We only consider the context to be completely constructed at this point, after all bolts
  // have been fired, the injection has taken place, and we're about to return.  We delay
  // finalizing the introduction of the return value into the list to this point for that
  // reason.
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  *childIterator = retVal;
  if(IsShutdown())
    retVal->SignalShutdown();
  return retVal;
}

size_t CoreContext::GetChildCount(void) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  return m_children.size();
}

std::vector<CoreRunnable*> CoreContext::GetRunnables(void) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  return std::vector<CoreRunnable*>(m_threads.begin(), m_threads.end());
}

std::shared_ptr<CoreContext> CoreContext::FirstChild(void) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

  // Just return the first child we successfully obtain a shared pointer for:
  for(const auto& cur : m_children) {
    auto child = cur.lock();
    if(child)
      return child;
  }

  // Seems like we have no children, return here
  return std::shared_ptr<CoreContext>();
}

std::shared_ptr<CoreContext> CoreContext::NextSibling(void) const {
  // Root contexts do not have siblings
  if(!m_pParent)
    return std::shared_ptr<CoreContext>();

  // Our iterator will always be valid in our parent collection.  Take a copy, lock the parent collection down
  // to prevent it from being modified, and then see what happens when we increment

  std::lock_guard<std::mutex> lk(m_pParent->m_stateBlock->m_lock);
  for(
    auto cur = m_backReference;
    ++cur != m_pParent->m_children.end();
  ) {
    auto sibling = cur->lock();
    if(sibling)
      return sibling;
  }

  // Failed to lock any successor child in the parent context, return unsuccessful
  return std::shared_ptr<CoreContext>();
}

const std::type_info& CoreContext::GetAutoTypeId(const AnySharedPointer& ptr) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

  const std::type_info& ti = ptr->type();
  auto q = m_typeMemos.find(ti);
  if (q == m_typeMemos.end() || !q->second.pObjTraits)
    throw autowiring_error("Attempted to obtain the true type of a shared pointer that was not a member of this context");

  const CoreObjectDescriptor* pObjTraits = q->second.pObjTraits;
  return *pObjTraits->type;
}

void CoreContext::AddInternal(const CoreObjectDescriptor& traits) {
  {
    std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);

    // Validate that this addition does not generate an ambiguity.  We need to use the actual type of
    // the value, rather than the type passed in via traits.type, because the proper type might be a
    // concrete type defined in another context or potentially a unifier type.  Creating a slot here
    // is also undesirable because the complete type is not available and we can't create a dynaimc
    // caster to identify when this slot gets satisfied. If a slot was non-local, overwrite it.
    auto q = m_typeMemos.find(*traits.actual_type);
    if(q != m_typeMemos.end()) {
      auto& v = q->second;

      if (v.m_local) {
        if (traits.pCoreObject && *v.m_value == traits.pCoreObject)
          throw std::runtime_error("An attempt was made to add the same value to the same context more than once");
        if (*v.m_value)
          throw std::runtime_error("An attempt was made to add the same type to the same context more than once");
      }
      else {
        v.m_value = traits.value;
        v.m_local = true;
      }
    }

    // Add the new concrete type:
    m_concreteTypes.push_back(traits);

    // Insert each context element:
    if(traits.pContextMember)
      AddContextMember(traits.pContextMember);

    if(traits.pFilter)
      m_filters.push_back(traits.pFilter.get());

    if(traits.pBoltBase)
      AddBolt(traits.pBoltBase);

    // Notify any autowiring field that is currently waiting that we have a new member
    // to be considered.
    UpdateDeferredElements(std::move(lk), m_concreteTypes.back(), true);
  }
  
  // Moving this outside the lock because AddCoreRunnable will perform the checks inside its function
  if(traits.pCoreRunnable)
    AddCoreRunnable(traits.pCoreRunnable);

  // Event receivers:
  if(traits.receivesEvents) {
    JunctionBoxEntry<CoreObject> entry(this, traits.pCoreObject);

    // Add to our vector of local receivers first:
    (std::lock_guard<std::mutex>)m_stateBlock->m_lock,
    m_eventReceivers.insert(entry);

    // Recursively add to all junction box managers up the stack:
    AddEventReceiver(entry);
  }

  // Subscribers, if applicable:
  if(!traits.subscriber.empty())
    AddPacketSubscriber(traits.subscriber);

  // Signal listeners that a new object has been created
  GetGlobal()->Invoke(&AutowiringEvents::NewObject)(*this, traits);
}

void CoreContext::AddInternal(const AnySharedPointer& ptr) {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);

  // Verify that this type isn't already satisfied
  MemoEntry& entry = m_typeMemos[ptr->type()];
  if (entry.m_value)
    throw autowiring_error("This interface is already present in the context");

  // Now we can satisfy it:
  entry.m_value = ptr;
  UpdateDeferredElement(std::move(lk), entry);
}

void CoreContext::FindByType(AnySharedPointer& reference, bool localOnly) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  FindByTypeUnsafe(reference, localOnly);
}

MemoEntry& CoreContext::FindByTypeUnsafe(AnySharedPointer& reference, bool localOnly) const {
  const std::type_info& type = reference->type();

  // If we've attempted to search for this type before, we will return the value of the memo immediately:
  auto q = m_typeMemos.find(type);
  if(q != m_typeMemos.end()) {
    // We can copy over and return here
    if (!localOnly || q->second.m_local){
      reference = q->second.m_value;
    }
    return q->second;
  }

  // Resolve based on iterated dynamic casts for each concrete type:
  const CoreObjectDescriptor* pObjTraits = nullptr;
  for(const auto& type : m_concreteTypes) {
    if(!reference->try_assign(*type.value))
      // No match, try the next entry
      continue;

    if (pObjTraits)
      // Resolution ambiguity, cannot proceed
      throw autowiring_error("An attempt was made to resolve a type which has multiple possible clients");

    // Update the object traits reference:
    pObjTraits = &type;
  }

  // This entry was not formerly memoized.  Memoize unconditionally.
  MemoEntry& retVal = m_typeMemos[type];
  retVal.m_value = reference;
  retVal.pObjTraits = pObjTraits;
  return retVal;
}

void CoreContext::FindByTypeRecursive(AnySharedPointer& reference, const AutoSearchLambda& searchFn) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  FindByTypeUnsafe(reference);
  if(!m_pParent || reference)
    // Type satisfied in current context, or there is nowhere to recurse to--call the terminal function in any case
    return searchFn();

  // Recurse while holding lock on this context
  // NOTE: Racing Deadlock is only possible if there is an ambiguity or cycle in the traversal order
  // of contexts.  Because context trees are guaranteed to be acyclical and are also fixed at construction
  // time, a strict locking heirarchy is inferred, and a deadlock therefore impossible.
  m_pParent->FindByTypeRecursive(reference, searchFn);
}

std::shared_ptr<CoreContext> CoreContext::GetGlobal(void) {
  return std::static_pointer_cast<CoreContext, GlobalCoreContext>(GlobalCoreContext::Get());
}

std::vector<std::shared_ptr<BasicThread>> CoreContext::CopyBasicThreadList(void) const {
  std::vector<std::shared_ptr<BasicThread>> retVal;

  // It's safe to enumerate this list from outside of a protective lock because a linked list
  // has stable iterators, we do not delete entries from the interior of this list, and we only
  // add entries to the end of the list.
  for(CoreRunnable* q : m_threads){
    BasicThread* thread = dynamic_cast<BasicThread*>(q);
    if (thread)
      retVal.push_back(thread->GetSelf<BasicThread>());
  }
  return retVal;
}

void CoreContext::Initiate(void) {
  // First-pass check, used to prevent recursive deadlocks traceable to here that might
  // result from entities trying to initiate subcontexts from CoreRunnable::Start
  if (IsInitiated())
    return;

  // Get the beginning of the thread list that we have at the time of lock acquisition
  // New threads are added to the front of the thread list, which means that objects
  // after this iterator are the ones that will need to be started
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);

  // Now we can transition to initiated or running:
  switch (m_state) {
  case State::Initiated:
  case State::Running:
    // Double-check
    return;
  case State::NotStarted:
    if (IsGlobalContext())
      // Global context is permitted to transition directly to running
      m_state = State::Running;
    else
      // Transition to initiated state, can't start threads yet
      m_state = State::Initiated;
    break;
  case State::CanRun:
    // Parent already started, we can run if we want to
    m_state = State::Running;
    break;
  case State::Shutdown:
  case State::Abandoned:
    // Already in a terminal state
    return;
  }

  // State change has taken place, we can signal
  m_stateBlock->m_stateChanged.notify_all();

  // Now we can add the event receivers we haven't been able to add because the context
  // wasn't yet started:
  AddEventReceiversUnsafe(std::move(m_delayedEventReceivers));
  m_delayedEventReceivers.clear();

  // Spin up the junction box before starting threads
  m_junctionBoxManager->Initiate();

  // Notify all child contexts that they can start if they want
  if (!IsRunning()) {
    lk.unlock();

    // Need to inject a delayed context type so that this context will not be destroyed until
    // it has an opportunity to start.
    Inject<DelayedContextHold>(shared_from_this());
    return;
  }

  // Now we can recover the first thread that will need to be started
  auto beginning = m_threads.begin();

  // Start our threads before starting any child contexts:
  std::shared_ptr<ThreadPool> threadPool;
  auto nullPool = std::dynamic_pointer_cast<NullPool>(m_threadPool);
  if (nullPool) {
    // Decide which pool will become our current thread pool.  Global context is the final case,
    // which defaults to the system thread pool
    if (!nullPool->GetSuccessor())
      nullPool->SetSuccessor(m_pParent ? m_pParent->GetThreadPool() : SystemThreadPool::New());

    // Trigger null pool destruction at this point:
    m_threadPool = nullPool->MoveDispatchersToSuccessor();
  }

  // The default case should not generally occur, but if it were the case that the null pool were
  // updated before the context was initiated, then we would have no work to do as no successors
  // exist to be moved.  In that case, simply take a record of the current thread pool for the
  // call to Start that follows the unlock.
  threadPool = m_threadPool;
  lk.unlock();

  // Start the thread pool out of the lock, and then update our start token if our thread pool
  // reference has not changed.  The next pool could potentially be nullptr if the parent is going
  // down while we are going up.
  if (threadPool) {
    // Initiate 
    auto startToken = threadPool->Start();

    // Transfer all dispatchers from the null pool to the new thread pool:
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

    // If the thread pool was updated while we were trying to start the pool we observed earlier,
    // then allow our token to expire and do not do any other work.  Whomever caused the thread
    // pool pointer to be updated would also have seen that the context is currently started,
    // and would have updated both the thread pool pointer and the start token at the same time.
    if (m_threadPool == threadPool)
      // Swap, not assign; we don't want teardown to happen while synchronized
      std::swap(m_startToken, startToken);
  }

  if (beginning != m_threads.end()) {
    auto outstanding = m_stateBlock->IncrementOutstandingThreadCount(shared_from_this());
    for (auto q = beginning; q != m_threads.end(); ++q)
      (*q)->Start(outstanding);
  }

  // Update state of children now that we are initated
  TryTransitionChildrenState();
}

void CoreContext::SignalShutdown(bool wait, ShutdownMode shutdownMode) {
  // As we signal shutdown, there may be a CoreRunnable that is in the "running" state.  If so,
  // then we will skip that thread as we signal the list of threads to shutdown.
  std::list<CoreRunnable*>::iterator firstThreadToStop;

  // Trivial return check
  if (IsShutdown())
    return;

  // Wipe out the junction box manager, notify anyone waiting on the state condition:
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

    switch (m_state) {
    case State::NotStarted:
    case State::CanRun:
      // User never initiated
      m_state = State::Abandoned;
      break;
    case State::Initiated:
    case State::Running:
      // Initiate called, move to the shutdown state
      m_state = State::Shutdown;
      break;
    case State::Shutdown:
    case State::Abandoned:
      // Already shut down, no further work need be done
      return;
    }

    UnregisterEventReceiversUnsafe();
    
    firstThreadToStop = m_threads.begin();
    if (m_beforeRunning)
      ++firstThreadToStop;
    
    m_stateBlock->m_stateChanged.notify_all();
  }

  // Teardown interleave assurance--all of these contexts will generally be destroyed
  // at the exit of this block, due to the behavior of SignalTerminate, unless exterior
  // context references (IE, related to snooping) exist.
  //
  // This is done in order to provide a stable collection that may be traversed during
  // teardown outside of a lock.
  std::vector<std::shared_ptr<CoreContext>> childrenInterleave;

  // Thread pool token and pool pointer
  std::shared_ptr<void> startToken;
  std::shared_ptr<ThreadPool> threadPool;

  // Tear down all the children, evict thread pool:
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

    startToken = std::move(m_startToken);
    m_startToken.reset();
    threadPool = std::move(m_threadPool);
    m_threadPool.reset();

    // Fill strong lock series in order to ensure proper teardown interleave:
    childrenInterleave.reserve(m_children.size());
    for(const auto& entry : m_children) {
      auto childContext = entry.lock();

      // Technically, it *is* possible for this weak pointer to be expired, even though
      // we're holding the lock.  This may happen if the context itself is exiting even
      // as we are processing SignalTerminate.  In that case, the child context in
      // question is blocking in its dtor lambda, waiting patiently until we're done,
      // at which point it will modify the m_children collection.
      if(!childContext)
        continue;

      // Add to the interleave so we can SignalTerminate in a controlled way.
      childrenInterleave.push_back(childContext);
    }
  }

  // Now that we have a locked-down, immutable series, begin termination signalling:
  for(size_t i = childrenInterleave.size(); i--; )
    childrenInterleave[i]->SignalShutdown(false, shutdownMode);

  // Pass notice to all child threads:
  bool graceful = (shutdownMode == ShutdownMode::Graceful);
  for (auto itr = firstThreadToStop; itr != m_threads.end(); ++itr)
    (*itr)->Stop(graceful);

  // Wait if requested
  if(wait)
    Wait();
}

void CoreContext::Quiescent(void) const {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  m_stateBlock->m_stateChanged.wait(lk, [this] { return m_stateBlock->m_outstanding.expired(); });
}

bool CoreContext::Quiescent(const std::chrono::nanoseconds duration) const {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  return m_stateBlock->m_stateChanged.wait_for(lk, duration, [this] { return m_stateBlock->m_outstanding.expired(); });
}

bool CoreContext::IsQuiescent(void) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  return m_stateBlock->m_outstanding.expired();
}

void CoreContext::Wait(void) {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  m_stateBlock->m_stateChanged.wait(lk, [this] {return IsShutdown() && m_stateBlock->m_outstanding.expired(); });
}

bool CoreContext::Wait(const std::chrono::nanoseconds duration) {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  return m_stateBlock->m_stateChanged.wait_for(lk, duration, [this] {return IsShutdown() && m_stateBlock->m_outstanding.expired(); });
}

bool CoreContext::DelayUntilInitiated(void) {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  m_stateBlock->m_stateChanged.wait(lk, [this] {return IsInitiated();});
  return !IsShutdown();
}

std::shared_ptr<CoreContext> CoreContext::CurrentContextOrNull(void) {
  auto retVal = autoCurrentContext.get();
  return retVal ? *retVal : nullptr;
}

std::shared_ptr<CoreContext> CoreContext::CurrentContext(void) {
  if(!autoCurrentContext.get())
    return std::static_pointer_cast<CoreContext, GlobalCoreContext>(GetGlobalContext());

  std::shared_ptr<CoreContext>* retVal = autoCurrentContext.get();
  assert(retVal);
  assert(*retVal);
  return *retVal;
}

void CoreContext::AddCoreRunnable(const std::shared_ptr<CoreRunnable>& ptr) {
  // Performing a double check.
  bool shouldRun;
  {
    std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
    
    // Insert into the linked list of threads first:
    m_threads.push_front(ptr.get());
    
    // Check if we're already running, this means we're late to the party and need to start _now_.
    shouldRun = IsRunning();
    
    // Signal that we are in the "running"
    m_beforeRunning = true;
  }

  // Run this thread without the lock
  if(shouldRun)
    ptr->Start(m_stateBlock->IncrementOutstandingThreadCount(shared_from_this()));
  

  // Check if the stop signal was sent between the time we started running until now.  If so, then
  // we will stop the thread manually here.
  bool shouldStopHere;
  {
    std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
    
    // Signal that we have stopped "running"
    m_beforeRunning = false;
    
    // If SignalShutdown() was invoked while we were "running", then we will need to stop this thread ourselves
    shouldStopHere = IsShutdown();
  }

  if(shouldStopHere)
    ptr->Stop(false);
}

void CoreContext::AddBolt(const std::shared_ptr<BoltBase>& pBase) {
  for(auto cur = pBase->GetContextSigils(); *cur; cur++){
    m_nameListeners[**cur].push_back(pBase.get());
  }

  if(!*pBase->GetContextSigils())
    m_nameListeners[typeid(void)].push_back(pBase.get());
}

JunctionBoxBase& CoreContext::All(const std::type_info& ti) const {
  auto jb = m_junctionBoxManager->Get(ti);
  if (!jb)
    throw autowiring_error("Attempted to obtain a junction box which has not been declared");
  return *jb;
}

void CoreContext::BuildCurrentState(void) {
  auto glbl = GlobalCoreContext::Get();
  glbl->Invoke(&AutowiringEvents::NewContext)(*this);
    
  // Enumerate objects injected into this context
  for(auto& object : m_concreteTypes) {
    GetGlobal()->Invoke(&AutowiringEvents::NewObject)(*this, object);
  }
  
  // Recurse on all children
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  for(const auto& c : m_children) {
    auto cur = c.lock();
    if(!cur)
      continue;

    // Recurse into the child instance:
    cur->BuildCurrentState();
  }
}

void CoreContext::SetThreadPool(const std::shared_ptr<ThreadPool>& threadPool) {
  if (!threadPool)
    throw std::invalid_argument("A context cannot be given a null thread pool");

  std::shared_ptr<ThreadPool> priorThreadPool;
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
    if (IsShutdown())
      // Nothing to do, context already down
      return;

    if (!IsRunning()) {
      // Just set up the forwarding thread pool
      auto nullPool = std::dynamic_pointer_cast<NullPool>(m_threadPool);
      if (!nullPool)
        throw autowiring_error("Internal error, null pool was deassigned even though the context has not been started");
      priorThreadPool = nullPool->GetSuccessor();
      nullPool->SetSuccessor(threadPool);
      return;
    }

    priorThreadPool = m_threadPool;
    m_threadPool = threadPool;
  }

  // We are presently running.  We need to start the pool, and then attempt to
  // update our token
  auto startToken = threadPool->Start();
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  if (m_threadPool != threadPool)
    // Thread pool was updated by someone else, let them complete their operation
    return;

  // Update our start token and return.  Swap, not move; we don't want to risk
  // calling destructors while synchronized.
  std::swap(m_startToken, startToken);
}

std::shared_ptr<ThreadPool> CoreContext::GetThreadPool(void) const {
  return (std::lock_guard<std::mutex>)m_stateBlock->m_lock, m_threadPool;
}

void CoreContext::CancelAutowiringNotification(DeferrableAutowiring* pDeferrable) {
  if (!pDeferrable)
    return;

  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  auto q = m_typeMemos.find(pDeferrable->GetType());
  if(q == m_typeMemos.end())
    return;

  // Always finalize this entry:
  auto strategy = pDeferrable->GetStrategy();
  if(strategy)
    strategy->Finalize();

  // Stores the immediate predecessor of the node we will linearly scan for in our
  // linked list.
  DeferrableAutowiring* prior = nullptr;

  // Now remove the entry from the list:
  // NOTE:  If a performance bottleneck is tracked to here, the solution is to use
  // a doubly-linked list.
  for(auto cur = q->second.pFirst; cur && cur != pDeferrable; prior = cur, cur = cur->GetFlink())
    if(!cur)
      // Ran off the end of the list, nothing we can do here
      return;

  if(prior)
    // Erase the entry by using link elision:
    prior->SetFlink(pDeferrable->GetFlink());

  if(pDeferrable == q->second.pFirst)
    // Current deferrable is at the head, update the flink:
    q->second.pFirst = pDeferrable->GetFlink();
}

void CoreContext::Dump(std::ostream& os) const {
  std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
  
  for(const auto& entry : m_typeMemos) {
    os << demangle(entry.first);
    const void* pObj = entry.second.m_value->ptr();
    if(pObj)
      os << " 0x" << std::hex << pObj;
    os << std::endl;
  }

  for(CoreRunnable* runnable : m_threads) {
    BasicThread* pThread = dynamic_cast<BasicThread*>(runnable);
    if (!pThread) continue;

    const char* name = pThread->GetName();
    os << "Thread " << pThread << " " << (name ? name : "(no name)") << std::endl;
  }
}

void ShutdownCurrentContext(void) {
  CoreContext::CurrentContext()->SignalShutdown();
}

void CoreContext::UnregisterEventReceiversUnsafe(void) {
  // Release all event receivers originating from this context:
  for(const auto& entry : m_eventReceivers)
    m_junctionBoxManager->RemoveEventReceiver(entry);

  // Notify our parent (if we have one) that our event receivers are going away:
  if(m_pParent)
    m_pParent->RemoveEventReceivers(m_eventReceivers);

  // Recursively unregister packet factory subscribers:
  AnySharedPointerT<AutoPacketFactory> pf;
  FindByTypeUnsafe(pf);

  // Wipe out all collections so we don't try to free these multiple times:
  m_eventReceivers.clear();
}

void CoreContext::BroadcastContextCreationNotice(const std::type_info& sigil) const {
  auto listeners = m_nameListeners.find(sigil);
  if(listeners != m_nameListeners.end()) {
    // Iterate through all listeners:
    for(BoltBase* bolt : listeners->second)
      bolt->ContextCreated();
  }

  // In the case of an anonymous sigil type, we do not notify the all-types
  // listeners a second time.
  if(sigil != typeid(void)) {
    listeners = m_nameListeners.find(typeid(void));
    if(listeners != m_nameListeners.end())
      for(BoltBase* bolt : listeners->second)
        bolt->ContextCreated();
  }

  // Notify the parent next:
  if(m_pParent)
    m_pParent->BroadcastContextCreationNotice(sigil);
}

void CoreContext::SatisfyAutowiring(std::unique_lock<std::mutex>& lk, MemoEntry& entry) {
  std::vector<DeferrableUnsynchronizedStrategy*> requiresFinalize;

  // Now we need to take on the responsibility of satisfying this deferral.  We will do this by
  // nullifying the flink, and by ensuring that the memo is satisfied at the point where we
  // release the lock.
  std::stack<DeferrableAutowiring*> stk;
  stk.push(entry.pFirst);
  entry.pFirst = nullptr;

  // Depth-first search
  while (!stk.empty()) {
    auto top = stk.top();
    stk.pop();

    for (DeferrableAutowiring* pCur = top; pCur; pCur = pCur->GetFlink()) {
      pCur->SatisfyAutowiring(entry.m_value);

      // See if there's another chain we need to process:
      auto child = pCur->ReleaseDependentChain();
      if (child)
        stk.push(child);

      // Not everyone needs to be finalized.  The entities that don't require finalization
      // are identified by an empty strategy, and we just skip them.
      auto strategy = pCur->GetStrategy();
      if (strategy)
        requiresFinalize.push_back(strategy);
    }
  }

  lk.unlock();

  // Run through everything else and finalize it all:
  for (const auto& cur : requiresFinalize)
    cur->Finalize();
}

void CoreContext::UpdateDeferredElement(std::unique_lock<std::mutex>&& lk, MemoEntry& entry) {
  // Satisfy what needs to be satisfied:
  SatisfyAutowiring(lk, entry);

  // Give children a chance to also update their deferred elements:
  lk.lock();
  for (const auto& weak_child : m_children) {
    // Hold reference to prevent this iterator from becoming invalidated:
    auto ctxt = weak_child.lock();
    if (!ctxt)
      continue;

    // Reverse lock before satisfying children:
    lk.unlock();
    ctxt->UpdateDeferredElement(
      std::unique_lock<std::mutex>(ctxt->m_stateBlock->m_lock),
      entry
    );
    lk.lock();
  }
  lk.unlock();
}

void CoreContext::UpdateDeferredElements(std::unique_lock<std::mutex>&& lk, const CoreObjectDescriptor& entry, bool local) {
  {
    // Collection of items needing finalization:
    std::vector<DeferrableUnsynchronizedStrategy*> delayedFinalize;

    // Notify any autowired field whose autowiring was deferred.  We do this by processing each entry
    // in the entire type memos collection.  These entries are keyed on the type of the memo, and the
    // value is a linked list of trees of deferred autowiring instances that will need to be called
    // if the corresponding memo type has been satisfied.
    //
    // A tree data structure is used, here, specifically because there are cases where child nodes
    // on a tree should only be called if and only if the root node is still present.  For instance,
    // creating an Autowired field adds a tree to this list with the root node referring to the
    // Autowired field itself, and then invoking Autowired::NotifyWhenAutowired attaches a child to
    // this tree.  If the Autowired instance is destroyed, the lambda registered for notification is
    // also removed at the same time.
    //
    // Each connected nonroot deferrable autowiring is referred to as a "dependant chain".
    std::stack<DeferrableAutowiring*> stk;
    for (auto& cur : m_typeMemos) {
      MemoEntry& value = cur.second;

      if (value.m_value && value.m_local)
        // This entry is already satisfied locally, no need to process it
        continue;

      // Determine whether the current candidate element satisfies the autowiring we are considering.
      // This is done internally via a dynamic cast on the interface type for which this polymorphic
      // base type was constructed.
      if (!value.m_value->try_assign(entry.pCoreObject))
        continue;

      // Success, assign the traits
      value.pObjTraits = &entry;

      // Store if it was injected from the local context or not
      value.m_local = local;

      // Now we need to take on the responsibility of satisfying this deferral.  We will do this by
      // nullifying the flink, and by ensuring that the memo is satisfied at the point where we
      // release the lock.
      stk.push(value.pFirst);
      value.pFirst = nullptr;

      // Finish satisfying the remainder of the chain while we hold the lock:
      while (!stk.empty()) {
        auto top = stk.top();
        stk.pop();

        for (DeferrableAutowiring* pNext = top; pNext; pNext = pNext->GetFlink()) {
          pNext->SatisfyAutowiring(value.m_value);

          // See if there's another chain we need to process:
          auto child = pNext->ReleaseDependentChain();
          if (child)
            stk.push(child);

          // Not everyone needs to be finalized.  The entities that don't require finalization
          // are identified by an empty strategy, and we just skip them.
          auto strategy = pNext->GetStrategy();
          if (strategy)
            delayedFinalize.push_back(strategy);
        }
      }
    }
    lk.unlock();

    // Run through everything else and finalize it all:
    for (const auto& cur : delayedFinalize)
      cur->Finalize();
  }

  // Give children a chance to also update their deferred elements:
  lk.lock();
  for (const auto& weak_child : m_children) {
    // Hold reference to prevent this iterator from becoming invalidated:
    auto ctxt = weak_child.lock();
    if(!ctxt)
      continue;

    // Reverse lock before handing off control:
    lk.unlock();
    ctxt->UpdateDeferredElements(
      std::unique_lock<std::mutex>(ctxt->m_stateBlock->m_lock),
      entry,
      false
    );
    lk.lock();
  }
}

void CoreContext::AddEventReceiver(const JunctionBoxEntry<CoreObject>& entry) {
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);

    if (!IsInitiated()) {
      // Delay adding receiver until context is initialized
      m_delayedEventReceivers.insert(entry);
      return;
    }
  }

  m_junctionBoxManager->AddEventReceiver(entry);

  // Delegate ascending resolution, where possible.  This ensures that the parent context links
  // this event receiver to compatible senders in the parent context itself.
  if(m_pParent)
    m_pParent->AddEventReceiver(entry);
}


void CoreContext::AddEventReceiversUnsafe(t_rcvrSet&& receivers) {
  if (IsInitiated()) {
    // Context is initiated, we can safely attach these receivers to our own junction box
    for (const auto& q : receivers)
      m_junctionBoxManager->AddEventReceiver(q);

    // Delegate ascending resolution, where possible.  This ensures that the parent context links
    // this event receiver to compatible senders in the parent context itself.
    if (m_pParent) {
      std::lock_guard<std::mutex> lk(m_pParent->m_stateBlock->m_lock);
      m_pParent->AddEventReceiversUnsafe(std::forward<t_rcvrSet&&>(receivers));
    }
  }
  else {
    // Context not initiated, we need to add the receivers to our own deferred collection
    if (m_delayedEventReceivers.empty())
      m_delayedEventReceivers = std::move(receivers);
    else
      m_delayedEventReceivers.insert(receivers.begin(), receivers.end());
  }
}

void CoreContext::RemoveEventReceivers(const t_rcvrSet& receivers) {
  for(const auto& q : receivers)
    m_junctionBoxManager->RemoveEventReceiver(q);

  // Detour to the parent collection (if necessary)
  if(m_pParent)
    m_pParent->RemoveEventReceivers(receivers);
}

void CoreContext::UnsnoopEvents(const AnySharedPointer& snooper, const JunctionBoxEntry<CoreObject>& receiver) {
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
    if(
      // If the passed value is currently a snooper, then the caller has snooped a context and also
      // one of its parents.  End here.
      m_snoopers.count(snooper) ||

      // If we are an event receiver in this context, then the caller has snooped a context and
      // is also a proper EventReceiver in one of that context's ancestors--this is a fairly
      // common case.  We return here, rather than incorrectly unregistering the caller.
      m_eventReceivers.count(receiver)
    )
      return;

    m_delayedEventReceivers.erase(receiver);
  }
  
  // Always remove from this junction box manager:
  m_junctionBoxManager->RemoveEventReceiver(receiver);

  // Handoff to parent type:
  if(m_pParent)
    m_pParent->UnsnoopEvents(snooper, receiver);
}

void CoreContext::FilterException(void) {
  bool handled = false;
  for(ExceptionFilter* filter : m_filters) {
    try {
      filter->Filter();
      handled = true;
    } catch(...) {
      // Do nothing
    }
  }

  // Pass to parent if one exists:
  if(m_pParent) {
    try {
      // See if the parent wants to handle this exception:
      m_pParent->FilterException();

      // Parent handled it, we're good to go
      return;
    } catch(...) {
      // Do nothing
    }
  }

  // Rethrow if unhandled:
  if(!handled)
    throw;
}

void CoreContext::FilterFiringException(const JunctionBoxBase* pProxy, CoreObject* pRecipient) {
  // Filter in order:
  for(CoreContext* pCur = this; pCur; pCur = pCur->GetParentContext().get())
    for(ExceptionFilter* filter : pCur->m_filters) {
      try {
        filter->Filter(pProxy, pRecipient);
      } catch(...) {
        // Do nothing, filter didn't want to filter this exception
      }
    }
}

void CoreContext::AddSnooper(const CoreObjectDescriptor& traits) {
  // Add to collections of snoopers
  InsertSnooper(traits.value);

  // Add EventReceiver
  if (traits.receivesEvents)
    AddEventReceiver(JunctionBoxEntry<CoreObject>(this, traits.pCoreObject));

  // Add PacketSubscriber;
  if (!traits.subscriber.empty())
    AddPacketSubscriber(traits.subscriber);
}

void CoreContext::RemoveSnooper(const CoreObjectDescriptor& traits) {
  RemoveSnooper(traits.value);

  // Cleanup if its an EventReceiver
  if (traits.receivesEvents)
    UnsnoopEvents(traits.value, JunctionBoxEntry<CoreObject>(this, traits.pCoreObject));

  // Cleanup if its a packet listener
  if (!traits.subscriber.empty())
    UnsnoopAutoPacket(traits);
}

void CoreContext::AddDeferredUnsafe(DeferrableAutowiring* deferrable) {
  // Determine whether a type memo exists right now for the thing we're trying to defer.  If it doesn't
  // exist, we need to inject one in order to allow deferred satisfaction to know what kind of type we
  // are trying to satisfy at this point.
  size_t found = m_typeMemos.count(deferrable->GetType());

  if(!found)
    // Slot not presently initialized, need to initialize it:
    m_typeMemos[deferrable->GetType()].m_value = deferrable->GetSharedPointer();

  // Obtain the entry (potentially a second time):
  MemoEntry& entry = m_typeMemos[deferrable->GetType()];

  // Chain forward the linked list:
  deferrable->SetFlink(entry.pFirst);
  entry.pFirst = deferrable;
}

void CoreContext::InsertSnooper(const AnySharedPointer& snooper) {
  (std::lock_guard<std::mutex>)m_stateBlock->m_lock,
  m_snoopers.insert(snooper);
}

void CoreContext::RemoveSnooper(const AnySharedPointer& snooper) {
  (std::lock_guard<std::mutex>)m_stateBlock->m_lock,
  m_snoopers.erase(snooper);
}

void CoreContext::AddContextMember(const std::shared_ptr<ContextMember>& ptr) {
  // Always add to the set of context members
  m_contextMembers.push_back(ptr.get());
}

void CoreContext::AddPacketSubscriber(const AutoFilterDescriptor& rhs) {
  Inject<AutoPacketFactory>()->AddSubscriber(rhs);
}

void CoreContext::TryTransitionChildrenState(void) {
  std::unique_lock<std::mutex> lk(m_stateBlock->m_lock);
  
  // We have to hold this to prevent dtors from running in a synchronized context
  std::shared_ptr<CoreContext> prior;
  for (auto childWeak : m_children) {
    // Obtain child pointer and lock it down so our iterator stays stable
    auto child = childWeak.lock();
    if (!child)
      // Expiration while we were attempting to dereference, circle around
      continue;
    
    // Cannot hold a lock safely if we hand off control to a child context
    lk.unlock();
    {
      // Get lock of child while we're modifying it's state
      std::unique_lock<std::mutex> childLk(child->m_stateBlock->m_lock);
      
      switch (child->m_state) {
        case State::Initiated:
          // Can transition to the running state now
          {
            auto q = child->m_threads.begin();
            child->m_state = State::Running;
            
            // Child had it's state changed
            child->m_stateBlock->m_stateChanged.notify_all();
            
            childLk.unlock();
            auto outstanding = child->m_stateBlock->IncrementOutstandingThreadCount(child);
            
            while (q != child->m_threads.end()) {
              (*q)->Start(outstanding);
              q++;
            }
          }
          
          // Recursivly try to transition children
          child->TryTransitionChildrenState();
          
          break;
        case State::NotStarted:
          // Children can run now that their parent has been initiated
          child->m_state = State::CanRun;
          
          // Child had it's state changed
          child->m_stateBlock->m_stateChanged.notify_all();
          break;
        case State::CanRun:
        case State::Running:
        case State::Shutdown:
        case State::Abandoned:
          // No action need be taken for these states
          break;
      }
    }
    
    // Permit a prior context to expire if needed
    prior.reset();
    
    // Need to preserve current child context pointer before it goes out of scope in order to
    // preserve our iterator.
    lk.lock();
    prior = child;
  }
  
  // Release our lock before letting `prior` expire, we can't hold a lock through such an event
  lk.unlock();
}

void CoreContext::UnsnoopAutoPacket(const CoreObjectDescriptor& traits) {
  {
    std::lock_guard<std::mutex> lk(m_stateBlock->m_lock);
    
    // If the passed value is currently a snooper, then the caller has snooped a context and also
    // one of its parents.  End here.
    if (m_snoopers.count(traits.value))
      return;
  }
  
  // Always remove from this context's PacketFactory:
  Inject<AutoPacketFactory>()->RemoveSubscriber(traits.subscriber);
}

std::ostream& operator<<(std::ostream& os, const CoreContext& rhs) {
  rhs.Dump(os);
  return os;
}

std::shared_ptr<CoreContext> CoreContext::SetCurrent(const std::shared_ptr<CoreContext>& ctxt) {
  std::shared_ptr<CoreContext> retVal = CoreContext::CurrentContextOrNull();
  if (ctxt)
    autoCurrentContext.reset(new std::shared_ptr<CoreContext>(ctxt));
  else
    autoCurrentContext.reset();
  return retVal;
}

std::shared_ptr<CoreContext> CoreContext::SetCurrent(void) {
  return CoreContext::SetCurrent(shared_from_this());
}

void CoreContext::EvictCurrent(void) {
  autoCurrentContext.reset();
}
