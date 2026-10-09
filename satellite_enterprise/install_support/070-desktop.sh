# satellite 004 -- the launcher, for $HOME/.satl only.
#
# The author, 2026-09-22: clicking the installed app "calls the 0027 build
# interpreter, but it's a different satl-term or something, settings is gone, and
# there's no line at the bottom of it that says idle/running". The launcher still
# named 003's satl-term, which ran 004's satl inside 003's window -- so the word
# was 004's and the window was not. That settles the rest of D0.5.1: 004's window
# keeps org.satellite.terminal, the id pins and the .satl association already
# carry (icons/org.satellite.terminal.desktop says why the id stays).
#
# icons/org.satellite.terminal.desktop IS WRITTEN WITH satl'S ABSOLUTE PATH. Its
# Exec and TryExec name `satl` bare, and ~/.local/bin is not on a desktop
# session's PATH -- only an interactive shell's -- so a TryExec that misses HIDES
# THE ENTRY with no word anywhere (the lesson 003's distribute installer learnt).
# A root whose path would need quoting in a desktop entry gets no launcher, and
# is told so, rather than one that does not start.
#
# A LAUNCHER THIS INSTALLER DID NOT WRITE -- 003's, which ran satl-term -- is kept
# beside it as org.satellite.terminal.desktop.bak-<date>, which no menu reads, the
# way 060 keeps 003's satl. One this installer wrote carries its first comment
# line, and is simply replaced.

if [ "$home_root" = yes ]; then
    _apps=${XDG_DATA_HOME:-$HOME/.local/share}/applications
    _launcher=$_apps/org.satellite.terminal.desktop
    _satl=$root_real/satl
    case $_satl in
        *[!A-Za-z0-9._/+-]*)
            step "no launcher: $(shown "$_satl") would need quoting in a desktop entry; run satl by its path" ;;
        *)
            if ! mkdir -p -- "$_apps" 2> /dev/null ||
                ! sed -e "s|^Exec=satl |Exec=$_satl |" -e "s|^TryExec=satl\$|TryExec=$_satl|" \
                    "$here/icons/org.satellite.terminal.desktop" > "$_launcher.installing" 2> /dev/null; then
                rm -f -- "$_launcher.installing"
                step "could not write the launcher in $_apps; satl still runs by its path"
            elif cmp -s -- "$_launcher.installing" "$_launcher"; then
                rm -f -- "$_launcher.installing"
            else
                if [ -f "$_launcher" ] &&
                    ! head -n 1 -- "$_launcher" | grep -qxF '# Desktop entry for satl, which opens its own console (GTK_AND_NO_DEPENDENCIES.md'; then
                    _kept=$_launcher.bak-$(date +%Y%m%d-%H%M%S)
                    cp -p -- "$_launcher" "$_kept" &&
                        step "kept the launcher that was there, which this installer did not write, as $(basename -- "$_kept")"
                fi
                if mv -fT -- "$_launcher.installing" "$_launcher"; then
                    step "wrote the launcher $(shown "$_launcher"): Satellite opens $(shown "$_satl") --console"
                    ! command -v update-desktop-database > /dev/null 2>&1 ||
                        update-desktop-database -q -- "$_apps" 2> /dev/null || :
                else
                    rm -f -- "$_launcher.installing"
                    step "could not put the launcher in place at $_launcher"
                fi
            fi
            ;;
    esac
    unset _apps _launcher _satl _kept
fi
