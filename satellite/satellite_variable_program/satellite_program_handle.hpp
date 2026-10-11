#pragma once
// satellite/satellite_variable_program/satellite_program_handle.hpp -- the one name
// satellite_object.hpp needs for arm 19: what a program IS held through.
//
// ITS OWN HEADER, for the reason satellite_thread_handle.hpp gives: the class behind it holds
// a mutex, a condition and a running process, and satellite_object.hpp needs only a handle to
// a class it never looks inside.
//
// A SHARED HANDLE, as a thread's, a window's and a file's are: `b = a` is a second name for the
// same program, so starting it through one and joining it through the other is one program.

#include <memory>

namespace satellite004 {

class satellite_program;
using ProgramHandle = std::shared_ptr<satellite_program>;

} // namespace satellite004
