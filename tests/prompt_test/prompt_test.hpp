#pragma once

// The harness, and the four sections. Three functions and a counter, no
// framework -- the shape every suite in this tree uses.
//
// WHAT IS BEING PROVED. PLAN M22 makes four claims and each is invisible in the
// others:
//
//   1. A KEY IS A KEY HOWEVER ITS BYTES ARRIVE. An arrow key is `ESC [ A` and
//      nothing promises those three bytes come from one read(), so the decoder
//      is fed one byte at a time and must answer Key::None until it has a whole
//      one. A test that fed it the three together would pass against code that
//      cannot work on a slow link.
//   2. THE EDITOR IS THE LINE AND NOTHING ELSE. Keys in, buffer out, with no
//      terminal anywhere -- which is what lets these clauses run in a build
//      with no tty at all.
//   3. A BRACE INSIDE A STRING IS NOT A BRACE. block.cpp counts through the
//      language's own lexer rather than over characters, so `display("{")` does
//      not open a block. This is the clause that would fail against the obvious
//      implementation, and it is why the lexer is linked into this binary.
//   4. THE PROMPT RUNS THE REAL satl. §4 forkpty(3)s it, types, and asserts on
//      what a person would see -- the same rule console_test's terminal section
//      keeps, and for the same reason: Ctrl-C at a prompt is the BYTE 0x03 and
//      a test that wrote it to a pipe would be testing nothing, because with no
//      terminal there is no raw mode and ISIG was never turned off.
//
// IT LINKS THE LEXER, THE PARSER, THE RESOLVER AND THE EVALUATOR, which is more
// than the first three sections need and exactly what §4's subject is: a typed
// line becomes a program through all four passes, and session.cpp is the file
// that says how.

#include <string>

namespace prompt_test {

extern int failures;

void check(bool ok, const std::string &what);

bool holds(const std::string &text, const std::string &needle);

void section_keys();     // bytes to keys, split arrivals, UTF-8, 0x03
void section_editing();  // the buffer, the cursor, history browsing
void section_blocks();   // depth through the lexer, placement, the owed body
void section_session();  // the real satl --repl under a pty

} // namespace prompt_test
