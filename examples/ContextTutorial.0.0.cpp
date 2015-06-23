// ContextTutorial.0.0.cpp created by Victor Dods on 2015/06/15
// Copyright (C) Leap Motion, Inc. All rights reserved.
/// @page ContextTutorial_0_0 Context Tutorial 0.0
/// This tutorial will introduce the reader to the concept of context within Autowiring and how to
/// implement a basic context hierarchy.
///
/// A context is the basic unit of organization within an autowired application. The scope of a context determines:
/// - How `Autowired` dependencies are resolved.
/// - Thread ownership (`BasicThread`, `CoreThread`).
/// - The scope of an `AutoFilter`-based filter network.
///
/// An autowired application has a context hierarchy whose root node is `GlobalCoreContext`, which is automatically created.
/// Things called 'context members' can be injected into any context, thereby defining the context hierarchy.
///
/// This tutorial will define and instantiate a simple context hierarchy.
/// @code
#include <autowiring/Autowired.h> // Needed for Autowiring classes.
#include <iostream>               // Needed for std::cout.
#include <string>                 // Needed for std::string.
/// @endcode 
///
/// @code
int main () {
/// @endcode
/// As mentioned before, the root node of the context hierarchy is created by default, having type `std::shared_ptr<GlobalCoreContext>`.
/// We must `Initiate` that context (and any other that we wish to use) before using it.  From `CoreContext::Initiate`, this call
/// starts all registered threads and enables events and the flow of filter graph packets.
/// @code
  std::shared_ptr<GlobalCoreContext> global_ctxt = GlobalCoreContext::Get();
  global_ctxt->Initiate();
/// @endcode
/// 
/// @code
  struct InputDeviceContext { };
  struct WebCam { std::string name; float res_x, res_y; WebCam (const std::string &n) : name(n), res_x(1.0f), res_y(1.0f) { } };
  struct Keyboard { std::string name; std::string input_buffer; Keyboard (const std::string &n) : name(n) { } };
  struct Mouse { std::string name; float pos_x, pos_y; Mouse (const std::string &n) : name(n), pos_x(0), pos_y(0) { } };
  struct Microphone { std::string name; float signal; Microphone (const std::string &n) : name(n), signal(0.0f) { } };

  std::shared_ptr<CoreContextT<InputDeviceContext>> idc = global_ctxt->Create<InputDeviceContext>();
  auto wc1 = idc->Inject<WebCam>("webcam1");
  auto wc2 = idc->Inject<WebCam>("webcam2");
  auto kb1 = idc->Inject<Keyboard>("keyboard1");
  auto m1 = idc->Inject<Mouse>("mouse1");

  //std::cout << "Enumerating all context members of type WebCam in InputDeviceContext.\n";
  //for (auto wc : ContextEnumeratorT<WebCam>(idc)) {
  //  std::cout << "    " << wc->name << '\n';
  //}
  
  #define FMT(x) #x << " = " << (x)
  
  std::cout << FMT(wc1->name) << ", " << FMT(wc2->name) << ", " << FMT(kb1->name) << ", " << FMT(m1->name) << '\n';
  
  
  return 0; // Return with no error.
}
/// @endcode
/// The output of this program is:
///
///     XXX
