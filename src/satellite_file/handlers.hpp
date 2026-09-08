#pragma once

// `satellite.file`'s rows, `satellite.variable.file`'s, and
// `satellite.system.delete`'s -- PLAN M19.
//
// SIXTEEN ROWS OVER THREE PARENTS, AND THE THIRD PARENT IS THE ONE WORTH
// EXPLAINING. `satellite.system.delete` `1 22 1` is a `satellite.system`
// number and `satellite_system/handlers.cpp` does not install it: v1's argument
// for the number is that unlink acts on a NAME, so one verb covers a file and
// an empty directory and belongs above both modules -- and v1's body accepts an
// open `satellite.variable.file` as well as a string, so the only module that
// can read its second argument shape is this one. PLAN M19 moved the row here
// for exactly that reason, and it is installed beside the code that can read it
// rather than beside the number it is spelled under.
//
// AND `1 6 2 1` IS DELIBERATELY NOT INSTALLED. `satellite.variable.file.new` is
// a number with nothing behind it: DESIGN §6.4 qualification 2 names
// `my_file.new()` as the confusing arity error the receiver tag EXISTS TO
// PREVENT, not as a spelling to build, and v1 has no `new` handle method at
// all. A program writes `satellite.file.new(path)` `1 8 1`. WORD_NUMBERS §4
// carries the confirmation the author settled on 2026-09-08; here, an
// uninstalled row is S0721's sentence, which is how a reserved number reads
// until its milestone lands -- `satellite.time.new` `1 9 2`'s situation exactly.

namespace satellite::file {

// Install every row this module owns. Idempotent: install() overwrites rather
// than appends.
void install_handlers();

} // namespace satellite::file
