#pragma once
// satellite/satellite_variable_thread/satellite_thread_handle.hpp -- the one name
// satellite_object.hpp needs for arm 18: what a thread IS held through.
//
// ITS OWN HEADER, ON PURPOSE. A thread holds values -- the arguments it was handed and
// the answer it gives back -- so the class in satellite_thread.hpp needs satelliteObject
// whole, and satellite_object.hpp needs only this: a handle to a class it never looks
// inside. Two headers is what keeps the two from including each other.
//
// A SHARED HANDLE, as a window's and a file's are: `b = a` is a second name for the same
// thread, so starting it through one and joining it through the other is one thread (003's
// M23 §6, "two names, one thread").

#include <memory>

namespace satellite004 {

class satellite_thread;
using ThreadHandle = std::shared_ptr<satellite_thread>;

} // namespace satellite004
