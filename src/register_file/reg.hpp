#pragma once

// One slot in the virtual machine's register file — §17.4.
//
// Status: the register TYPE and the stack that holds it. There is no VM yet,
// and nothing in the interpreter includes this file. The tree walker of §10 is
// still what runs, and stays the authority until a VM produces byte-identical
// output and identical error vectors.
//
// WHAT THIS IS FOR. §17 measured the tree walker and found two costs. One is
// dispatch — a recursive eval() call and a variant dispatch per node, 75% of an
// addition — and that is what bytecode removes. The other is the remaining 25%:
// `ValuePtr` is `shared_ptr<const Value>`, so every intermediate result is a
// make_shared, a malloc plus an ATOMIC refcount, to add two integers. No
// dispatch strategy touches that. A VM built on `ValuePtr` would remove the 75%,
// keep the 25%, land near 1.6x and read as evidence that bytecode was oversold.
// This type is how the 25% goes.
//
// WHAT IT IS NOT. `Value` does not change, and neither do `ValuePtr`, `Library`
// or `Object::fields`. A design pass measured a hybrid register — inline for the
// common case, `ValuePtr` for everything else — against a full 16-byte tagged
// union at 15.38 against 15.91 ns/iter, which is a wash. The hybrid buys the
// same speed without reopening the lock-free publish protocol that
// ThreadSanitizer currently verifies, and that trade is why `Value` is left
// alone. Do not rewrite it.
//
// A REGISTER IS NEVER ALLOCATED. §17.4: one array exists, and a register is a
// slot in it. Register 5 of the current frame is `base[5]`. A design where each
// register were a separately allocated object would put a malloc back on every
// value and land exactly where the tree walker already is — the same mistake
// under a new name.

// This header is the umbrella. Its parts live beside it and are included in an
// order that compiles — Reg first, since RegStack is an array of them. Include
// this file, not the parts: every file that included reg.hpp before still gets
// exactly what it got.

#include "register_file/reg_slot.hpp"
#include "register_file/reg_stack.hpp"
