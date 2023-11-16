#include "x11.h"

#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

x11_connection *x11_connection_open(void) {
    xcb_connection_t *const connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection)) {
        (void)fprintf(stderr, "error: could not connect to X server\n");
        return NULL;
    }

    return connection;
}

void x11_connection_close(xcb_connection_t *const connection) {
    xcb_disconnect(connection);
}

int x11_set_root_name(x11_connection *const connection, const char *name) {
    xcb_screen_t *const screen =
        xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    const xcb_window_t root_window = screen->root;

    const unsigned short name_format = 8;
    const xcb_void_cookie_t cookie = xcb_change_property(
        connection, XCB_PROP_MODE_REPLACE, root_window, XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING, name_format, strlen(name), name);

    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error != NULL) {
        (void)fprintf(stderr, "error: could not set X root name\n");
        return 1;
    }

    if (xcb_flush(connection) <= 0) {
        (void)fprintf(stderr, "error: could not flush X output buffer\n");
        return 1;
    }

    return 0;
}
