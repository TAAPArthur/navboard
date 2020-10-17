#include "myvkbd.h"
#include "layout.dvorak.h"
int fontsize = 20;
double overlay_delay = 1.0;
int heightfactor = 16; //one row of keys takes up 1/x of the screen height
const char* fonts[] = {
    "DejaVu Sans:bold:size=20"
};
/*
static const char *colors[SchemeLast][2] = {
    //     fg         bg
	[SchemeNorm] = { "#ffffff", "#14313d" },
	[SchemePress] = { "#ffffff", "#000000" },
	[SchemeHighlight] = { "#58a7c6", "#005577" },
};
*/
Layout layouts[] = {{keys_dvorak}};
