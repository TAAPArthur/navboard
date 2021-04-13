#!/bin/sh -e

NAVBOARD_CONFIG_HOME=${XDG_CONFIG_DIR:-$HOME/.config}/navboard
NAVBOARD_LOCAL="$NAVBOARD_CONFIG_HOME/navboard-local"
if [ "$1" = "-r" ] || [ "$1" = "--recompile" ]; then
    mkdir -p "$NAVBOARD_CONFIG_HOME"
    ${CC:-cc} -o "$NAVBOARD_LOCAL" -lnavboard "$NAVBOARD_CONFIG_HOME"/*.c -lX11 -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-xtest -lX11-xcb -lXft -lm
else
    exec "$NAVBOARD_LOCAL" "$@"
fi

