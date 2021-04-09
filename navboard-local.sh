#!/bin/sh -e

NAVBOARD_CONFIG_HOME=${XDG_CONFIG_DIR:-$HOME/.config}/navboard
NAVBOARD_LOCAL_LIB="$NAVBOARD_CONFIG_HOME/libnavboard.so"
if [ "$1" = "-r" ] || [ "$1" = "--recompile" ]; then
    mkdir -p "$NAVBOARD_CONFIG_HOME"
    ${CC:-cc} -fPIC -shared -o "$NAVBOARD_LOCAL_LIB" "$NAVBOARD_CONFIG_HOME"/*.c
else
    LD_PRELOAD=$NAVBOARD_LOCAL_LIB:$LD_PRELOAD navboard "$@"
fi

