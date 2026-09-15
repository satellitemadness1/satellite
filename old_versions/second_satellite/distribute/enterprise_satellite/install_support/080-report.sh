# satellite -- what it says afterwards.
#
# THE REPORT IS FOR SOMEBODY WHO HAS NEVER INSTALLED ANYTHING FROM A TERMINAL.
# That is not a reason to say less; it is a reason to say what happened, where
# the files went, and what to type next, in that order, without a single word
# that has to be looked up first. Everything below is either a fact this script
# established or a command that can be pasted.
#
# THE INSTALL IS VERIFIED BY RUNNING IT, which is the only check worth printing.
# A list of copied files proves that cp exited 0. Running the installed
# interpreter proves that the binary chosen for this CPU actually executes on
# this CPU -- which is the one thing in this package that a build machine cannot
# know and this machine can.

if [ "$dry_run" = yes ]; then
    say ""
    say "$me: that was a dry run. Nothing above was done."
    say "    Run the same command without -n to do it."
    exit 0
fi

if [ "$action" = uninstall ]; then
    say ""
    say "$me: satellite has been removed from $prefix."
    say "    $removed_count paths were named by the install tree; the ones that were"
    say "    there are gone, and every directory this install created and then"
    say "    emptied has been removed."
    if [ "$shell_path_state" = removed ]; then
        say ""
        say "    The satellite block was removed from $shell_rc."
        say "    Open a new terminal for that to take effect in one."
    fi
    say ""
    say "    Left alone on purpose:"
    say "      $theme_index"
    say "        the hicolor index.theme, which every other application's icons"
    say "        in that directory need too."
    if [ -f "$shell_rc.satellite-backup" ]; then
        say "      $shell_rc.satellite-backup"
        say "        your shell startup file as it was before satellite first"
        say "        edited it. Delete it yourself when you no longer want it."
    fi
    if [ -n "${dependencies_missing:-}" ] || command -v dnf >/dev/null 2>&1; then
        say "      the system packages dnf installed"
        say "        gtk4, vte291-gtk4 and the rest are used by other programs"
        say "        on this machine, so removing them is not this script's"
        say "        call to make."
    fi
    exit 0
fi

# ---------------------------------------------------------------------------
# DOES IT RUN.
#
# SATL_NO_WINDOW=1 IS NOT DECORATION. satl hands itself to satl-term when it
# finds it has no console, and that check runs before any argument is read.
# This script's own output may be redirected to a file or a pipe by whoever ran
# it, and /dev/null is a character device that satl reads as "no console" -- so
# `satl --version >/dev/null` once handed a machine-readable query to the GUI
# binary and got back "satl-term: unknown option --version". --version is on the
# list of flags that only print and exit, which fixed that case, and the
# variable is set here anyway because this line must be right about a program it
# is checking rather than reasoning about.
runs=no
version_line=
if [ -x "$bindir/satl" ]; then
    version_line=$(SATL_NO_WINDOW=1 "$bindir/satl" --version 2>/dev/null | head -1 || :)
    [ -n "$version_line" ] && runs=yes
fi

say ""
if [ "$runs" = yes ]; then
    say "$me: satellite is installed."
else
    say "$me: the files are installed, but the interpreter did not run."
fi
say ""

# ---------------------------------------------------------------------------
# WHAT WENT WHERE.
say "    Programs           $bindir"
say "      satl             the language itself${version_line:+   [$version_line]}"
case $cpu_variant in
    haswell)
        say "                       built for x86-64-v3, which this CPU supports"
        ;;
    baseline)
        say "                       the baseline build, which every x86-64 CPU runs"
        ;;
    undetected)
        say "                       the baseline build (satl-cpu-level did not run)"
        ;;
esac
say "      satl-cpu-level   asks this CPU which build of satl it can run"
case $have_term in
    yes|unchecked)
        say "      satl-term        the satellite window"
        ;;
    no)
        say "      satl-term        NOT INSTALLED -- see below"
        ;;
    absent)
        say "      satl-term        not in this package"
        ;;
esac
say ""
say "    Everything else    $datadir"
say "      applications/    the launcher, which is the icon in the apps grid"
say "      icons/hicolor/   the artwork: 9 sizes each of the satellite icon"
say "                       and the .satl file icon"
say "      mime/packages/   the .satl file type"
say "      satellite/example/   programs to read and run"
say ""

# ---------------------------------------------------------------------------
# WHAT TO DO NEXT. Three things, in the order somebody would actually try them.
say "    To start:"
say ""
if [ "$have_term" = yes ] || [ "$have_term" = unchecked ]; then
    say "      Click the Satellite icon in your applications grid."
    say "      (Press the Super key, then type: satellite)"
    say ""
    say "      If it is not there yet, log out and back in once -- a desktop"
    say "      reads its list of applications when the session starts."
    say ""
fi
case $shell_path_state in
    written)
        say "      Or, in a terminal. In a NEW terminal window -- this install"
        say "      added $bindir to your PATH in $shell_rc,"
        say "      and a window that was already open has not read that yet:"
        ;;
    *)
        say "      Or, in a terminal:"
        ;;
esac
say "          satl                          start the satellite prompt"
say "          satl --help                   every command it knows"
say "          satl $datadir/satellite/example/hello_world.satl"
say ""
say "      The example programs are read-only where they are installed."
say "      To get your own copies you can change:"
say "          cp -r $datadir/satellite/example ~/satellite-examples"
say ""

# ---------------------------------------------------------------------------
# WHAT MIGHT STILL BE WRONG, and every one of these is a thing this script
# checked rather than a thing it worried about. Nothing is printed unless it
# actually applies.
_notes=

if [ "$runs" = no ]; then
    _notes="$_notes
  The interpreter did not answer --version.
      Try running it directly to see what it says:
          $bindir/satl --version"
fi

# THE PATH NOTE IS ONLY A PROBLEM WHEN NOTHING WAS DONE ABOUT IT. bin_on_path
# is what the CURRENT shell has, and this script cannot change that -- a child
# process cannot edit its parent's environment. So a fresh install always looks
# like a failure here and is not one: the block was written, and the next
# terminal window will have it. The two cases are reported differently for that
# reason, and getting this wrong would mean telling somebody their install was
# broken every single time it worked.
if [ "$bin_on_path" = no ] && [ "$shell_path_state" != written ]; then
    _notes="$_notes
  Typing \`satl\` will not find it: $bindir is not on your PATH,
      and this run did not add it (--no-path, or a prefix that is on
      PATH already). Clicking the icon still works -- the launcher
      holds the full path. For the command line, use the full path:
          $bindir/satl"
fi

if [ "$data_on_xdg" = no ]; then
    _notes="$_notes
  $datadir is not on XDG_DATA_DIRS, so the desktop will not look
      there for the launcher or the icons. The programs work; the icon
      in the apps grid will not appear. Installing into ~/.local (no
      --prefix at all) or /usr/local (--system) avoids this."
fi

case $dependencies_state in
    failed)
        _notes="$_notes
  dnf could not install the packages satl-term needs:
          $dependencies_failed
      No network, a disabled repository and a declined password all
      look like this. The interpreter is installed and works. To try
      the window again:
          sudo dnf install $dependencies_failed
          sh $(quoted "$self" | sed 's/^ //')"
        ;;
    no-sudo)
        _notes="$_notes
  Some packages satl-term needs are missing and sudo is not on this
      machine, so they could not be installed. As root:
          dnf install $dependencies_missing"
        ;;
    no-dnf|no-rpm)
        _notes="$_notes
  This machine has no dnf, so the packages satl-term needs were not
      checked or installed. If the window does not open, its libraries
      are the first thing to look at."
        ;;
esac

case $have_term in
    no)
        _notes="$_notes
  satl-term was NOT installed, because libraries it needs are missing:
$(printf '%s\n' "$missing_libs" | sed 's/^/          /')
      That is what draws the window, so there is no icon to click yet.
      On AlmaLinux this is one command:
          sudo dnf install vte291-gtk4
      Then run this installer again. The interpreter is unaffected and
      works now."
        ;;
    unchecked)
        _notes="$_notes
  satl-term was installed without checking its libraries, because ldd
      is not on this machine. If clicking the icon does nothing, run
      \`$bindir/satl-term\` in a terminal to see why."
        ;;
esac

if [ "$theme_index_state" = missing ]; then
    _notes="$_notes
  $datadir/icons/hicolor has no index.theme, and without one GTK does
      not read that directory at all -- all eighteen icons are installed
      and none will ever be drawn, with no error anywhere. The package
      that owns that file is:
          sudo dnf install hicolor-icon-theme
      Then run this installer again."
fi

if [ "$os_family_ok" = no ]; then
    _notes="$_notes
  This package was built on AlmaLinux 10.2 and this machine reports
      $os_name $os_version. The interpreter is a static binary and does
      not care. satl-term links against this machine's GTK4 and VTE, so
      it is the one that may not run."
fi

if [ -n "$_notes" ]; then
    say "    Worth knowing:"
    printf '%s\n' "$_notes" | sed 's/^  /      /'
    say ""
fi

# THE UNINSTALL LINE HAS TO CARRY THE PREFIX BACK, because --uninstall with no
# --prefix removes from the default one, which is not where this went. Only when
# it differs: the common case is the default, and printing "--prefix
# /home/mom/.local" on every install would teach that the flag is required.
_prefix_flag=
[ "$prefix" = "$HOME/.local" ] || _prefix_flag=" --prefix $(quoted "$prefix" | sed 's/^ //')"

say "    To remove it all again:"
say "        sh $(quoted "$self" | sed 's/^ //') --uninstall$_prefix_flag"
say ""
