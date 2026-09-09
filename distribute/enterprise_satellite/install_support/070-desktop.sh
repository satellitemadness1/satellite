# satellite -- the file that makes the artwork visible, and the three indexes
# that make the rest of it findable.
#
# EIGHTEEN CORRECT PNGs IN THE RIGHT DIRECTORIES DRAW NOTHING WITHOUT THIS
# FRAGMENT, and that is the whole reason it is a separate one. 060 put every
# file where the specifications say it goes; none of the specifications say the
# desktop will look.

# ---------------------------------------------------------------------------
# index.theme -- the file whose absence is invisible.
#
# AN ICON DIRECTORY IS ONLY A THEME IF IT CONTAINS AN index.theme, and without
# one GTK does not look inside it AT ALL: has_icon() answers false for every
# icon in it, at every size, and the desktop draws the generic fallback. So a
# prefix can hold all eighteen correct PNGs and a .satl file still shows a blank
# page, with nothing anywhere reporting an error.
#
# gtk-update-icon-cache DOES NOT REVEAL IT EITHER, because the -t below is
# --ignore-theme-index: the cache builds happily over a directory that is not
# yet a theme. The only symptom is artwork that never appears.
#
# THE FILE BELONGS TO THE hicolor-icon-theme PACKAGE, WHICH INSTALLS IT UNDER
# /usr AND NOWHERE ELSE. So every prefix except /usr starts without one --
# ~/.local/share, which is this script's default, and /usr/local, which is
# --system. Both of the other installers in this tree learned this the same way
# and it is written up in satellite_enterprise/install_support/075-system.sh.
#
# THREE SOURCES, IN THIS ORDER, AND THE ORDER IS A PREFERENCE FOR THE REAL FILE
# OVER OUR COPY OF IT:
#
#   1. one that is already there   -- left alone. It describes hicolor for
#                                     somebody else's icons too, and this
#                                     install does not own it.
#   2. /usr/share/icons/hicolor/   -- copied. This is the machine's own, it is
#                                     complete, and it is the one every other
#                                     application on this desktop is being
#                                     drawn through.
#   3. the copy in this package    -- copied, as the fallback for a machine
#                                     with no hicolor-icon-theme installed.
#
# COPIED AND NOT GENERATED, in all three cases, because the file describes
# hicolor ITSELF -- its full directory list, their sizes and their contexts --
# and not the nine sizes of it that satellite happens to use. A generated one
# listing only our sizes would be valid and would quietly break the next
# application to install a 96x96 icon into the same directory.
theme_index=$datadir/icons/hicolor/index.theme
theme_index_state=skipped

if [ "$action" = install ]; then
    if [ -f "$theme_index" ]; then
        theme_index_state=present
    elif [ -f /usr/share/icons/hicolor/index.theme ]; then
        run mkdir -p "$datadir/icons/hicolor"
        run cp /usr/share/icons/hicolor/index.theme "$theme_index"
        run chmod 644 "$theme_index"
        theme_index_state=system
    elif [ -f "$payload_share/icons/hicolor/index.theme" ]; then
        run mkdir -p "$datadir/icons/hicolor"
        run cp "$payload_share/icons/hicolor/index.theme" "$theme_index"
        run chmod 644 "$theme_index"
        theme_index_state=packaged
    else
        # NOT FATAL. The programs are installed and correct; what is missing is
        # the file that makes a desktop read the icons, and the package that
        # owns it is named in 080-report.sh rather than guessed at.
        theme_index_state=missing
    fi
fi

# index.theme IS DELIBERATELY NOT REMOVED ON AN UNINSTALL, which both of the
# other installers do too and for the same reason: every other application that
# installs an icon into this same hicolor directory depends on it, and removing
# it would break their artwork on the way out of ours. It is the one file this
# script may create and will not destroy.

# ---------------------------------------------------------------------------
# THE THREE INDEXES.
#
# ALL THREE ARE BEST-EFFORT IN BOTH SENSES: a machine with no desktop has none
# of these tools, and a tool that fails does not fail the install. The files are
# correct either way; what an index does is make them found without a rescan,
# and a desktop that is restarted finds most of them anyway.
#
# EACH TOOL IS ASKED WHETHER ITS DIRECTORY IS STILL THERE, which matters only on
# the way out: 060 has already removed the tree and rmdir'd every directory it
# emptied, so update-mime-database would exit 1 with "Directory does not exist"
# and `set -e` would take the whole script down BEFORE 080 reports -- which is
# how a clean uninstall comes to look like a crash.
rebuild_data_indexes() {
    _data=$1

    # mime/packages IS RECREATED RATHER THAN SKIPPED when the mime directory
    # itself survived. That is not the uninstall putting something back: the
    # COMPILED database in $_data/mime is still there and still lists
    # application/x-satellite, and update-mime-database is the only thing that
    # can take it back out again -- and it will not run without a packages/
    # directory to read. An empty packages/ is its normal state.
    if [ -d "$_data/mime" ] && command -v update-mime-database >/dev/null 2>&1; then
        [ -d "$_data/mime/packages" ] || run mkdir -p "$_data/mime/packages"
        run update-mime-database "$_data/mime" || :
    fi

    if [ -d "$_data/icons/hicolor" ] &&
       command -v gtk-update-icon-cache >/dev/null 2>&1; then
        run gtk-update-icon-cache -qtf "$_data/icons/hicolor" || :
    fi

    # THE THIRD INDEX IS THE ONE THAT IS EASY TO LEAVE OUT. A .desktop file
    # dropped into applications/ is found by the menu on its own -- so the icon
    # appears in the apps grid without this -- but the MimeType= line inside it
    # is only consulted through mimeinfo.cache, which this builds. Without it
    # the entry appears, the file type is registered, the icon is drawn, and
    # double-clicking a .satl file still offers nothing to open it with. That is
    # the half of the install that would look like it worked.
    if [ -d "$_data/applications" ] &&
       command -v update-desktop-database >/dev/null 2>&1; then
        run update-desktop-database "$_data/applications" || :
    fi
}

rebuild_data_indexes "$datadir"

# ---------------------------------------------------------------------------
# IS THE PREFIX SOMEWHERE THE DESKTOP ACTUALLY LOOKS, and is bin/ on PATH.
#
# BOTH ARE OBSERVATIONS AND NEITHER CHANGES ANYTHING. 080-report.sh prints what
# they found. Nothing here edits .bashrc, .profile or any other file the person
# running it owns: a child process cannot change its parent's environment, so an
# installer that "exports PATH" exports it into a shell that exits one line
# later, and editing a login file is a permanent change made by a program
# somebody ran once that --uninstall could never reliably undo.
#
# THE TWO ARE SEPARATE QUESTIONS WITH SEPARATE ANSWERS, which is the thing worth
# knowing about this operating system. `satl` at a prompt needs bin/ on the
# shell's PATH, which AlmaLinux's stock ~/.bashrc provides for ~/.local/bin. The
# icon in the apps grid needs share/ on XDG_DATA_DIRS, which is a different
# variable set in a different place. One can work while the other does not, and
# an install that checked only one of them would report success for half a job.

bin_on_path=no
case ":${PATH:-}:" in
    *":$bindir:"*) bin_on_path=yes ;;
esac

# XDG_DATA_DIRS AND ITS SPECIFIED DEFAULT. The variable is very often unset, and
# unset does NOT mean "no directories" -- the XDG base directory specification
# says it then means /usr/local/share:/usr/share. Treating unset as empty would
# report a correct --system install as broken.
#
# XDG_DATA_HOME IS THE SEPARATE ONE, and is why the default prefix needs no
# check at all: ~/.local/share is the user data directory by specification and
# is searched ahead of everything in XDG_DATA_DIRS.
data_on_xdg=no
_xdg_dirs=${XDG_DATA_DIRS:-/usr/local/share:/usr/share}
_xdg_home=${XDG_DATA_HOME:-$HOME/.local/share}
case ":$_xdg_dirs:" in
    *":$datadir:"*) data_on_xdg=yes ;;
esac
[ "$datadir" = "$_xdg_home" ] && data_on_xdg=yes
