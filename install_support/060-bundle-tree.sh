# satellite -- bundle mode: installing install_tree/ without a Makefile.
#
# THE FILE LIST IS DERIVED BY WALKING WHAT WAS SHIPPED, never restated. That is
# the promise at the top of install.sh, kept: the tree is the output of the
# Makefile's `install` target, staged by `make bundle`, and this copies it.
# Defines two functions; 070 calls them.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# ---------------------------------------------------------------------------
# Bundle mode: installing install_tree/ without a Makefile.
#
# The list of files is DERIVED BY WALKING WHAT WAS SHIPPED, never restated. That
# is what keeps the promise at the top of this file. install_tree/ is not a
# second copy of the install tree -- it is the RESULT of the one declaration in
# the Makefile's `install` target, produced by running it into a staging
# DESTDIR at an empty prefix. Add a data file to that target and it appears in
# the next bundle, and in these two functions, with no edit here.
#
# `install -D` per file rather than `cp -R`, for two reasons that both bite:
#
#   - cp -R applies the umask to what it creates. Under the 002 that is Ubuntu's
#     default for the primary user, share/ arrives group-writable -- the same
#     trap the Makefile's note on `install -d` describes at length.
#   - cp -Rp preserves the ownership of the unpacked tarball, which is whoever
#     unpacked it. `sudo ./install.sh` would then fill /usr/local with files
#     owned by that user. install(1) creates as the caller, which under sudo is
#     root, which is what /usr/local wants.
#
# The mode comes from the shipped file's own executable bit, so the binaries
# land 755 and the data lands 644, which is what the Makefile installed them as.
bundle_install() {
    # Directories first, and separately from the files, because
    # share/satellite/lib is shipped EMPTY and is load-bearing:
    # library_path()'s tier 2 accepts a candidate only if the directory exists,
    # and that empty directory is the entire reason a relocated tree resolves
    # to itself rather than falling through to the prefix it was built for
    # (DESIGN §9). A walk over files alone drops it without a word, and the
    # symptom is `satl --where` answering with this build machine's home
    # directory on somebody else's computer.
    #
    # -m755 stated on every one, for the umask reason above. $target itself is
    # not created here, for the same reason the Makefile does not create
    # $(prefix): install -D makes the leading directories it needs, and a
    # prefix the caller named is theirs to have made.
    (cd "$tree" && find . -type d -print) | while read -r d; do
        d=${d#.}
        d=${d#/}
        [ -n "$d" ] || continue
        run install -d -m755 "$target/$d" || exit 1
    done

    (cd "$tree" && find . -type f -print) | while read -r f; do
        f=${f#./}
        if [ -x "$tree/$f" ]; then _m=755; else _m=644; fi
        run install -D -m$_m "$tree/$f" "$target/$f" || exit 1
    done
}

# Symmetric with the Makefile's `uninstall`, and asymmetric with the install
# above in exactly the way that target is: share/satellite and
# share/doc/satellite are trees this install created and owns outright, so
# removing them wholesale is exact and also takes the empty lib/ with it.
# Everything else lives in a directory shared with the rest of the system --
# share/icons/hicolor, share/applications, share/mime/packages, share/man --
# where only the named files may go and the directory itself must stay, so
# those are removed one at a time by the same walk that installed them.
bundle_uninstall() {
    (cd "$tree" && find . -type f -print) | while read -r f; do
        f=${f#./}
        run rm -f "$target/$f" || exit 1
    done
    run rm -rf "$target/share/satellite" "$target/share/doc/satellite"
}
