# satellite -- source mode: build it before installing it.
#
# Nothing in here runs in bundle mode, which has the binaries already -- that
# is what a prebuilt download is for. Ends with the one banner line that both
# modes print.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# Source mode only: bundle mode has the binaries already, which is what it is
# for. Everything inside this block is about compiling, and in bundle mode
# there is no compiler, no sources and no Makefile to drive.
if [ "$action" = install ] && [ "$mode" = source ]; then
    # `make install` would build these itself, but doing it as a separate step
    # means a compile failure is reported as a compile failure, before anything
    # has been copied anywhere.
    #
    # The prefix goes to the BUILD, not only to the install: it is compiled in
    # as SATELLITE_LIB_DIR, so building without it and installing with it would
    # bake /usr/local into a binary going to $HOME/.local. Passing the same
    # prefix to both is also what stops make from compiling everything twice.
    #
    # SATELLITE_AUTOINSTALL=0 goes to both build invocations below, because
    # `make` at the top of the tree now installs what it built. That is right
    # for someone typing make; it is wrong here twice over. The build below runs
    # `make -C "$repo"`, which the Makefile cannot tell apart from a hand-typed
    # build in that directory, so without this it would install once during the
    # build and again at the `make install` further down -- and in the sudo
    # branch the first one is fatal, not merely redundant: that build runs as
    # the human, at a prefix only root can write, so it would refuse the install
    # and (worse, on any make that returned non-zero for it) take `set -e` and
    # the whole installer down before the privileged copy ever happened. This
    # script chooses the prefix and this script performs the install; the build
    # step it drives does neither.
    #
    # A command-line assignment rather than an exported variable, for the same
    # reason the prefix is one: sudo's env_reset strips the environment across
    # the -u boundary below, and the make command line is the only level that
    # outranks an assignment inside the makefile.
    if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then
        # Compile as the human, copy as root. Under sudo the compile would run
        # as root with HOME=/root, which fails to find a toolchain installed in
        # the user's home and -- worse when it succeeds -- leaves root-owned .o
        # files in a source tree the developer then cannot rebuild without
        # sudo forever after. Copying files into /usr/local is the only part of
        # this that actually needs privilege, so it is the only part that gets
        # it.
        printf 'install.sh: building as %s; root is used only for the copy\n' \
            "$SUDO_USER"
        run sudo -H -u "$SUDO_USER" "$MAKE" -C "$repo" "prefix=$prefix" \
            SATELLITE_AUTOINSTALL=0
    elif [ -x "$repo/satl" ] && [ -x "$repo/satl-term" ]; then
        # Binaries left over from a plain `make` carry that make's prefix, which
        # is now the one it installed itself to -- $HOME/.local for an ordinary
        # user, not /usr/local. The `make install` below is what corrects them:
        # the Makefile records the prefix in a stamp file that system.o depends
        # on, so a changed prefix recompiles the one object that was told the
        # old one and relinks. Nothing here has to force it.
        printf 'install.sh: using the binaries already built in %s\n' "$repo"
    else
        printf 'install.sh: building\n'
        run "$MAKE" -C "$repo" "prefix=$prefix" SATELLITE_AUTOINSTALL=0
    fi
fi

printf 'install.sh: %s prefix=%s%s\n' "$action" "$prefix" \
    "${destdir:+ (staged under $destdir)}"
