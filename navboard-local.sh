#!/bin/sh -e

NAVBOARD_CONFIG_HOME=${XDG_CONFIG_DIR:-$HOME/.config}/navboard
NAVBOARD_LOCAL="$NAVBOARD_CONFIG_HOME/navboard-local"
if [ "$1" = "-r" ] || [ "$1" = "--recompile" ]; then
    mkdir -p "$NAVBOARD_CONFIG_HOME"
    cd "$NAVBOARD_CONFIG_HOME" || exit 1
    for f in *.sh; do
        [ -x "$f" ] && "./$f" "$NAVBOARD_CONFIG_HOME"
    done
    ${CC:-cc} -o "$NAVBOARD_LOCAL" ./*.c -lnavboard -lX11 -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-xtest -lX11-xcb -lXft -lm
else
    exec "$NAVBOARD_LOCAL" "$@"
fi

