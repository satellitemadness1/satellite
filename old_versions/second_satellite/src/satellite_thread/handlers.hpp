#pragma once

// `satellite.thread`'s rows and `satellite.variable.thread`'s -- PLAN M23.
//
// THREE ROWS FOR SIX NUMBERED PATHS, AND THE OTHER THREE ARE RIGHT TO HAVE
// NONE. PLAN §8's M23 entry names six and its own ledger caught that "M23 names
// only `satellite.variable.thread`" was true of five of them:
//
//     satellite.thread                  1 23       a namespace -- nothing to call
//     satellite.thread()                1 23 0     the bare shape, reserved
//     satellite.thread.new(f(x))        1 23 1     INSTALLED
//     satellite.variable.thread         1 6 13     a type name in a declaration
//     satellite.variable.thread()       1 6 13 0   the bare shape, reserved
//     satellite.variable.thread.start() 1 6 13 1   INSTALLED
//     satellite.variable.thread.join()  1 6 13 2   INSTALLED
//
// That is seven lines for six paths because `1 23` and `1 6 13` are a namespace
// and a type, and satellite_help/built.cpp derives both from the children below
// them in one backward pass -- so neither needs a row here and neither needs
// one in words.def's front-end list either. `satellite.variable.capsule`
// `1 6 16` is the exception and words.def carries why: it has no children at
// all, so there is nothing to derive it from.
//
// AND `1 6 16` GETS NO HANDLER FOR THE SAME REASON `1 6 2 1` GETS NONE ONE
// MODULE OVER. A deferred call is made by `satellite.thread.new` and asked
// nothing; it is a value with no methods, which is `satellite.variable.time`'s
// position at M13 and `satellite.variable.float`'s from M15 to M21. An
// uninstalled row would be S0721's sentence, and there is no row to uninstall.
//
// THE ORDER THE ROWS ARE INSTALLED IN DOES NOT MATTER AND THE ORDER THEY RUN IN
// IS THE LANGUAGE. `new` packages, `start()` launches, `join()` waits -- and
// every wrong ordering of those three is an S14xx row rather than a silence.

namespace satellite::thread {

// Install every row this module owns. Idempotent: install() overwrites rather
// than appends.
void install_handlers();

} // namespace satellite::thread
