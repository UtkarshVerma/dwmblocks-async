#ifndef X11_H
#define X11_H

#include <xcb/xcb.h>

typedef xcb_connection_t x11_connection;

x11_connection* x11_connection_open(void);
void x11_connection_close(x11_connection* const connection);
int x11_set_root_name(x11_connection* const connection,
                      const char* const name);

#endif  // X11_H
