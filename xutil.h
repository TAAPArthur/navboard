#ifndef NAVBOARD_XUTIL_H
#define NAVBOARD_XUTIL_H
#include "navboard.h"

void initConnection(void);

void sendKeyEvent(char press, KeyCode keyCode);
XDrawable* createWindow(void);

char getKeyChar(xcb_keysym_t sym);

int getKeyCode(KeySym targetSym, xcb_keysym_t** foundSym);

void drawText(XDrawable* win, int numChars, Color foreground, int x, int y, const char*str);

void outlineRect(XDrawable* win, Color color,  int numRects, const xcb_rectangle_t* rects);

void setWindowProperties(XDrawable* win);

void updateDockProperties(XDrawable* win, DockType dockType, int thicknessPercent, int start, int end);

void updateBackground(XDrawable* win, Color color, xcb_rectangle_t* rects);
int matchesWindow(XDrawable* drawable,xcb_window_t win);

int getXFD();

void processAllQueuedEvents();

void logError(xcb_generic_error_t* e);

void mapWindow(XDrawable* drawable);
int xFlush();

void setFont(const char* fontName);
#endif

