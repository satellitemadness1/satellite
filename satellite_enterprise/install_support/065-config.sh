# satellite 004 -- $HOME/.satl/config.ini, the person's lasting settings.
#
# The author, 2026-09-18: *"let's put it into $(HOME)/.satl/config.ini then, and
# we'll have to build that into the installer, to install config.ini at that
# location"*.
#
# IT IS NEVER OVERWRITTEN, AND THAT IS THE WHOLE OF THIS FILE'S CARE. Everything
# else the installer puts down is REPLACED on purpose -- a new satl, new
# libraries, a rewritten record -- because those are satellite's. This one is the
# PERSON'S: it holds what their programs wrote with
# `satellite.library.main.arguments.access = satellite.bool.false`, and an
# install that reset it would throw away a setting somebody chose, silently, as a
# side effect of upgrading. So: created when absent, left exactly as it is when
# present, and no attempt is made to merge new keys into an old file.
#
# A MISSING KEY IS NOT A PROBLEM, which is what makes leaving it alone safe. Every
# setting carries its own default in its own library (satellite/config/
# config_file.hpp, and each satellite-numbers/ word's kDefault), so a config.ini
# written by an older install is read by a newer satl with the new settings simply
# taking their defaults. There is no version in the file and it needs none.
#
# $HOME AND NOT $root. The binaries may go to $HOME/.satl or, with --system, to
# /usr/local -- but the settings are per-PERSON either way, so they are always
# under a home directory and never beside the binary.
#
# AND UNDER sudo THAT HOME IS $SUDO_USER'S. A --system install runs as root, where
# $HOME is /root; writing there would create a config for a person who will never
# run satl and leave the one who ran the installer without one. $SUDO_USER is the
# account that asked, and its home is where this belongs.

_config_home=$HOME
if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ]; then
    _sudo_home=$(getent passwd -- "$SUDO_USER" 2> /dev/null | cut -d: -f6)
    [ -n "$_sudo_home" ] && _config_home=$_sudo_home
fi

if [ -z "$_config_home" ]; then
    step "no home directory to write config.ini into; satl will use its built-in defaults"
else
    _config_dir=$_config_home/.satl
    _config=$_config_dir/config.ini
    if [ -f "$_config" ]; then
        step "kept the config.ini already at $_config"
    elif mkdir -p -- "$_config_dir" 2> /dev/null &&
        cat > "$_config.installing" << 'SATELLITE_CONFIG_INI'
# satellite 004 -- your lasting settings.
#
# One `key = value` a line. `#` starts a comment, and anything this file does not
# name takes satl's built-in default, so a key you delete is not a key you break.
#
# A PROGRAM WRITES THIS FILE TOO. `satellite.library.main.arguments.access =
# satellite.bool.false` rewrites the row below and leaves every other line --
# these comments included -- exactly where it found them.

# access -- keep the last known name, type and value of everything, so that
# satellite.access(object_name) can still answer after a program has stopped.
access = true
SATELLITE_CONFIG_INI
    then
        if mv -fT -- "$_config.installing" "$_config" 2> /dev/null; then
            step "wrote $_config"
        else
            rm -f -- "$_config.installing"
            step "could not put config.ini in place at $_config; satl will use its built-in defaults"
        fi
    else
        rm -f -- "$_config.installing" 2> /dev/null
        step "could not write $_config; satl will use its built-in defaults"
    fi
    # AND THEN COMPOSE THE REGISTER, by running the satl just installed.
    #
    # THE TEMPLATE ABOVE DELIBERATELY DOES NOT CARRY A `features =` LINE. It is
    # fixed text and the register's width grows every time a feature is added, so
    # a template that held one would go stale the first time somebody added a bit
    # -- and a stale register is exactly what S011 exists to complain about. So
    # the installer does not write the value, it asks the binary to write it, and
    # the binary is the thing that knows how wide it is.
    #
    # RUN AS THE PERSON WHOSE CONFIG IT IS, with HOME pointed at their home, so a
    # --system install under sudo composes $SUDO_USER's register and not root's.
    if [ -x "$root/satl" ]; then
        if HOME=$_config_home "$root/satl" --rebuild > /dev/null 2>&1; then
            step "composed the feature register with $root/satl --rebuild"
        else
            step "could not run $root/satl --rebuild; run it yourself to compose the feature register"
        fi
    fi
    unset _config_dir _config
fi
unset _config_home _sudo_home
