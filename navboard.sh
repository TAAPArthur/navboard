#!/bin/sh -e

NAVBOARD_CONFIG_HOME=${XDG_CONFIG_DIR:-$HOME/.config}/navboard
NAVBOARD_CACHE_HOME=${XDG_CACHE_DIR:-$HOME/.cache}/navboard
NAVBOARD_LOCAL="$NAVBOARD_CACHE_HOME/navboard"
recompile() {
    mkdir -p "$NAVBOARD_CONFIG_HOME"
    mkdir -p "$NAVBOARD_CACHE_HOME"
    cd "$NAVBOARD_CONFIG_HOME" || exit 1
    for f in *.sh; do
        [ -x "$f" ] && "./$f" "$NAVBOARD_CONFIG_HOME"
    done
    if [ -f "Makefile" ] || [ -f "makefile" ]; then
        make
    else
        ${CC:-cc} -o "$NAVBOARD_LOCAL" ./*.c -lnavboard -lm
    fi
}
if [ "$1" = "-r" ] || [ "$1" = "--recompile" ]; then
    recompile
else
    if [ -d "$NAVBOARD_CONFIG_HOME" ]; then
        [ -x "$NAVBOARD_LOCAL" ] || recompile
        exec "$NAVBOARD_LOCAL" "$@"
    else
        exec "/usr/libexec/navboard" "$@"
    fi
fi

