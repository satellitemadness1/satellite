# nested_include -- a cwd for every file, with a decoy

(the author, 2026-09-16) "does it take includes that are in other .satl files
yet? We have to keep a cwd for each file."

It does, and this tree is the proof. `main.satl` includes `sub/one`, which
itself includes the bare name `two`. There are TWO files called `two.satl`:

    two.satl        at the top -- the DECOY
    sub/two.satl    beside the file that wrote the include -- the right one

A bare name resolves against the directory of THE FILE THAT WROTE THE INCLUDE,
so `sub/one.satl` must reach `sub/two.satl`. If the per-file directory were ever
lost -- resolved against the working directory, or against the main file -- the
decoy would win and the program would print the wrong line, loudly.

Run it:

    experiments/program_run.cpp  experiments/nested_include/main.satl

Expected: three files loaded (not four -- the decoy is never read), and

    the RIGHT two.satl -- sub/two.satl, beside the file that included it
