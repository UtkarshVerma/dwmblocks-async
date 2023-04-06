#include "x11.h"

#include <X11/Xlib.h>

static Display *display;
static Window rootWindow;

int setupX() {
    display = XOpenDisplay(NULL);
    if (!display) {
        return 1;
    }

    rootWindow = DefaultRootWindow(display);
    return 0;
}

int closeX() {
    return XCloseDisplay(display);
}

void setXRootName(char *str) {
    XStoreName(display, rootWindow, str);
    XFlush(display);
}
