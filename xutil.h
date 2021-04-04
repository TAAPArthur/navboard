#ifndef NAVBOARD_XUTIL_H
#define NAVBOARD_XUTIL_H
#include "navboard.h"
void initConnection(void);

void sendKeyEvent(char press, KeyCode keyCode);
xcb_window_t createWindow(void);

char getKeyChar(xcb_keysym_t sym);

int getKeyCode(KeySym targetSym, xcb_keysym_t** foundSym);

void drawText(xcb_window_t win, int numChars, int x, int y, const char*str);

void outlineRect(xcb_window_t win, int numRects, const xcb_rectangle_t* rects);

void setWindowProperties(xcb_window_t win);

void updateDockProperties(xcb_window_t win, DockType dockType, int thicknessPercent, int start, int end);

void updateBackground(xcb_window_t win, int press, xcb_rectangle_t* rects);

int getXFD();

void processAllQueuedEvents();

void logError(xcb_generic_error_t* e);
void mapWindow(xcb_window_t win);
int xFlush();
#endif

