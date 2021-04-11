#ifndef NAVBOARD_XUTIL_H
#define NAVBOARD_XUTIL_H
#include "common.h"

void initConnection(void);

void sendKeyEvent(char press, KeyCode keyCode);
XDrawable* createWindow(uint32_t windowMasks);
void setRootDims(uint16_t width, uint16_t height);

char getKeyChar(xcb_keysym_t sym);

int getKeyCode(KeySym targetSym, xcb_keysym_t** foundSym);

void drawText(XDrawable* win, int numChars, const char*str, Color foreground, int x, int y, int w, int h);

void outlineRect(XDrawable* win, Color color,  int numRects, const xcb_rectangle_t* rects);

void setWindowProperties(XDrawable* win);

void updateDockProperties(XDrawable* drawable, DockProperties dockProperties);

void updateBackground(XDrawable* win, Color color, xcb_rectangle_t* rects);
int matchesWindow(XDrawable* drawable,xcb_window_t win);

int getXFD();

void processAllQueuedEvents();

void logError(xcb_generic_error_t* e);

void destroyWindow(XDrawable* drawable);
void mapWindow(XDrawable* drawable);
int xFlush();

void setFont(const char* fontName);
void grabSelection();
#endif

