// Copyright (c) 2010 - 2013 Leap Motion. All rights reserved. Proprietary and confidential.
#include "stdafx.h"
#include "CoreThreadTest.h"
#include "Autowired.h"
#include "TestFixtures/SimpleThreaded.h"
#include <boost/thread/thread.hpp>

class SpamguardTest:
  public CoreThread
{
public:
  SpamguardTest(void):
    m_hit(false),
    m_multiHit(false)
  {
  }

  bool m_hit;
  bool m_multiHit;

  void Run(void) override {
    if(m_hit) {
      m_multiHit = true;
      return;
    }

    m_hit = false;

    boost::unique_lock<boost::mutex> lk(m_lock);
    m_stateCondition.wait(lk, [this] () {return ShouldStop();});
  }
};

TEST_F(CoreThreadTest, VerifyStartSpam) {
  // Create our thread class:
  AutoRequired<SpamguardTest> instance;

  m_create->InitiateCoreThreads();

  // This shouldn't cause another thread to be created:
  instance->Start(std::shared_ptr<Object>(new Object));

  EXPECT_FALSE(instance->m_multiHit) << "Thread was run more than once unexpectedly";
}

class InvokesIndefiniteWait:
  public CoreThread
{
public:
  virtual void Run(void) override {
    AcceptDispatchDelivery();

    // Wait for one event using an indefinite timeout, then quit:
    WaitForEvent(boost::chrono::steady_clock::time_point::max());
  }
};

TEST_F(CoreThreadTest, VerifyIndefiniteDelay) {
  AutoRequired<InvokesIndefiniteWait> instance;
  m_create->InitiateCoreThreads();

  // Verify that the instance does not quit until we pend something:
  ASSERT_FALSE(instance->WaitFor(boost::chrono::milliseconds(10))) << "Thread instance exited prematurely, when it should have been waiting indefinitely";

  // Now we pend an arbitrary event and verify that we can wait:
  *instance += [] {};
  ASSERT_TRUE(instance->WaitFor(boost::chrono::milliseconds(10))) << "Instance did not exit after an event was pended which should have woken up its dispatch loop";
}

TEST_F(CoreThreadTest, VerifyNestedTermination) {
  std::shared_ptr<SimpleThreaded> st;

  // Insert a thread into a second-order subcontext:
  {
    AutoCreateContext outer;
    CurrentContextPusher outerPshr(outer);
    AutoRequired<SimpleThreaded>();
    outer->InitiateCoreThreads();

    {
      AutoCreateContext ctxt;
      CurrentContextPusher pshr(ctxt);
      ctxt->InitiateCoreThreads();
      st = AutoRequired<SimpleThreaded>();
    }
  }

  // Everything should be running by now:
  ASSERT_TRUE(st->IsRunning()) << "Child thread was not running as expected";

  // Shut down the enclosing context:
  m_create->SignalTerminate(true);

  // Verify that the child thread has stopped:
  ASSERT_FALSE(st->IsRunning()) << "Child thread was running even though the enclosing context was terminated";
}

class SleepEvent : public virtual EventReceiver
{
public:
  virtual Deferred SleepFor(int seconds) = 0;
  virtual Deferred SleepForThenThrow(int seconds) = 0;
};

class ListenThread :
  public CoreThread,
  public SleepEvent
{
public:
  ListenThread() : CoreThread("ListenThread") {}

  Deferred SleepFor(int seconds) override {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    if(ShouldStop())
      throw std::runtime_error("Execution aborted");

    return Deferred(this);
  }

  Deferred SleepForThenThrow(int seconds) override {
    return Deferred(this);
  }
};

TEST_F(CoreThreadTest, VerifyDispatchQueueShutdown) {
  AutoCreateContext ctxt;
  CurrentContextPusher pusher(ctxt);

  AutoRequired<ListenThread> listener;
  try
  {
    ctxt->InitiateCoreThreads();
    listener->DelayUntilCanAccept();

    AutoFired<SleepEvent> evt;

    // Spam in a bunch of events:
    for(size_t i = 100; i--;)
      evt(&SleepEvent::SleepFor)(0);

    // Graceful termination then enclosing context shutdown:
    listener->Stop(true);
    ctxt->SignalShutdown(true);
  }
  catch (...) {}

  ASSERT_EQ(listener->GetDispatchQueueLength(), 0);
}

TEST_F(CoreThreadTest, VerifyNoLeakOnExecptions) {
  AutoCreateContext ctxt;
  CurrentContextPusher pshr(ctxt);

  AutoRequired<ListenThread> listener;
  std::shared_ptr<std::string> value(new std::string("sentinal"));

  std::weak_ptr<std::string> watcher(value);
  try
  {
    ctxt->InitiateCoreThreads();
    listener->DelayUntilCanAccept();

    *listener += [value] { throw std::exception(); };
    value.reset();
    ctxt->SignalShutdown(true);
  }
  catch (...) {}

  ASSERT_TRUE(watcher.expired()) << "Leaked memory on exception in a dispatch event";
}

TEST_F(CoreThreadTest, VerifyDelayedDispatchQueueSimple) {
  // Run our threads immediately, no need to wait
  m_create->InitiateCoreThreads();

  // Create a thread which we'll use just to pend dispatch events:
  AutoRequired<CoreThread> t;

  // Thread should be running by now:
  ASSERT_TRUE(t->IsRunning()) << "Thread added to a running context was not marked running";

  // Delay until the dispatch loop is actually running, then wait an additional 1ms to let the
  // WaitForEvent call catch on:
  t->DelayUntilCanAccept();
  boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

  // These are flags--we'll set them to true as the test proceeds
  std::shared_ptr<bool> x(new bool(false));
  std::shared_ptr<bool> y(new bool(false));

  // Pend a delayed event first, and then an immediate event right afterwards:
  *t += boost::chrono::milliseconds(25), [x] { *x = true; };
  *t += [y] { *y = true; };

  // Verify that, after 1ms, the first event is called and the second event is NOT called:
  boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
  ASSERT_TRUE(*y) << "A simple ready call was not dispatched within 10ms of being pended";
  ASSERT_FALSE(*x) << "An event which should not have been executed for 25ms was executed early";

  // Now delay another 90ms and see if the second event got called:
  boost::this_thread::sleep_for(boost::chrono::milliseconds(90));
  ASSERT_TRUE(*x) << "A delayed event was not made ready and executed as expected";
}

TEST_F(CoreThreadTest, VerifyDoublePendedDispatchDelay) {
  // Immediately run threads
  m_create->InitiateCoreThreads();

  // Some variables that we will set to true as the test proceeds:
  std::shared_ptr<bool> x(new bool(false));
  std::shared_ptr<bool> y(new bool(false));

  // Create a thread as before, and pend a few events.  The order, here, is important.  We intentionally
  // pend an event that won't happen for awhile, in order to trick the dispatch queue into waiting for
  // a lot longer than it should for the next event.
  AutoRequired<CoreThread> t;
  t->DelayUntilCanAccept();
  *t += boost::chrono::hours(1), [x] { *x = true; };

  // Now pend an event that will be ready just about right away:
  *t += boost::chrono::nanoseconds(1), [y] { *y = true; };

  // Delay for a short period of time, then check our variable states:
  boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

  // This one shouldn't have been hit yet, it isn't scheduled to be hit for 10s
  ASSERT_FALSE(*x) << "A delayed dispatch was invoked extremely early";

  // This one should have been ready almost at the same time as it was pended
  ASSERT_TRUE(*y) << "An out-of-order delayed dispatch was not executed in time as expected";
}