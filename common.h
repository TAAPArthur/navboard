#ifndef NAVBOARD_COMMON_H
#define NAVBOARD_COMMON_H

#define LEN(x)         (sizeof x / sizeof x[0])

typedef enum DockType {
    LEFT, RIGHT, TOP, BOTTOM
} DockType;

typedef struct {
    DockType type;
    short start;
    short end;
    int thicknessPercent;
} DockProperties;

typedef struct xdrawable XDrawable;

typedef uint32_t Color;


extern void(*xEventHandlers[])();

#endif
