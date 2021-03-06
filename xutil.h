#ifndef NAVBOARD_XUTIL_H
#define NAVBOARD_XUTIL_H
#include "common.h"

void initConnection(void);
void closeConnection();

void sendKeyEvent(char press, xcb_keycode_t keyCode);
void sendShiftKeyEvent(char press);
XDrawable* createWindow(uint32_t windowMasks);
void setRootDims(uint16_t width, uint16_t height);

char getKeyChar(xcb_keysym_t sym);

xcb_keycode_t getKeyCode(xcb_keysym_t targetSym, xcb_keysym_t** foundSym, char* symIndex);

void drawText(XDrawable* win, int numChars, const char*str, Color foreground, int x, int y, int w, int h);

void outlineRect(XDrawable* win, Color color,  int numRects, const xcb_rectangle_t* rects);

void setWindowProperties(XDrawable* win);

void updateDockProperties(XDrawable* drawable, DockProperties dockProperties);

void drawSlider(XDrawable* win, Color color, float percent, xcb_rectangle_t* rect, xcb_rectangle_t* dest);
void updateBackground(XDrawable* win, Color color, xcb_rectangle_t* rects);
int matchesWindow(XDrawable* drawable,xcb_window_t win);

int getXFD();

void processAllQueuedEvents();

void logError(xcb_generic_error_t* e);

void destroyWindow(XDrawable* drawable);
void mapWindow(XDrawable* drawable);
void onResize(XDrawable* drawable, uint32_t width, uint32_t height);
int xFlush();

void setFont(const char* fontName, int size);
void grabSelection();

void clear_drawable(XDrawable* drawable);
void clear_window(XDrawable* drawable);
#endif

