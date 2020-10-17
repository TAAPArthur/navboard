#ifndef MYVKBD_H
#define MYVKBD_H
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#define LEN(x)         (sizeof x / sizeof x[0])

#define SHIFT   (1<<0)
#define LOCK    (1<<1)
#define MOD     (1<<2)
typedef struct {
    KeySym keySym;
    KeySym keySymShift;
    unsigned int weight;
    char flags;
    const char* label;

    // should not be manually set
    char c;
    int index;
    KeyCode keyCode;
    char pressed;
} Key;
typedef struct Layout {
    Key* keys;
    Key* shiftKeys;
    int level;
} Layout;

typedef enum DockType {
    LEFT, RIGHT, TOP, BOTTOM
} DockType;
#endif
