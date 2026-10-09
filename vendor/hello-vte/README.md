# vendor/hello-vte -- the proof that the vendored, STATIC libvte works

Written 2026-09-22, the day VTE went into `vendor/new/` (GTK_AND_NO_DEPENDENCIES.md
GTK-17). `hello-vte.c` is a window holding a `VteTerminal`, a shell spawned in it
that prints one line and exits 7, the exit status read back through `child-exited`,
and the terminal's own text read back so what VTE drew is on stdout.

    XDG_RUNTIME_DIR=/run/user/1000 sh vendor/hello-vte/build-and-run.sh

It links against `vendor/stage/lib/libvte-2.91-gtk4.a` and the whole static stack
the way satl does, prints the binary's NEEDED (satl's seven, nothing more), starts a
headless mutter of its own (`satlbare`) and runs there with no session bus. Exit 0
means the line came back and the status was 7. Its output also shows the one thing
gnutls off costs -- the terminal's first line is VTE's own red warning (Q-VTE-1).

It SIGSEGVed the first time, for WIN-1's reason: the vendored GTK looks for xkb data
at /nonexistent unless told, so the script points XKB_CONFIG_ROOT at the staged
xkeyboard-config -- which is what satl's spill does before gtk_init.
