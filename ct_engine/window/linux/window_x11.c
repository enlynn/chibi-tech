#include "../window.h"
#include <std/mem.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

typedef struct {
    Display *display;
    Window   handle;
    Atom     delete_atom;
    bool     is_live;
    bool     destroy_nodify_recieved;
} x11_window_t;

fn_internal void
x11_get_display_dims(Display *display, int *width, int *height)
{
    int screen = DefaultScreen(display);
    *width  = DisplayWidth(display, screen);
    *height = DisplayHeight(display, screen);
}

fn_export void
os_window_create(size_t requested_width, size_t requested_height, const char* window_title, struct os_window_o *window_i, size_t *window_size)
{
    *window_size = sizeof(x11_window_t);
    if (!window_i) return;

    Display* main_display = XOpenDisplay(0);
    Window   root_window  = XDefaultRootWindow(main_display);

    //Note(enlynn): this has no effect because window managers are allowed to ignore the window position parameters.
    //center window on the display
    int screen_width, screen_height;
    x11_get_display_dims(main_display, &screen_width, &screen_height);

    int width  = fast_min(screen_width,  (int)requested_width);
    int height = fast_min(screen_height, (int)requested_height);

    int window_x = (screen_width  - requested_width) / 2;
    int window_y = (screen_height - requested_height) / 2;

    if (window_x < 0) window_x = 0;
    if (window_y < 0) window_y = 0;
    if (width  < 0)   width    = screen_width  - window_x; // extend the width to the width of the screen
    if (height < 0)   height   = screen_height - window_y; // extend the height to the height of the screen

    int border_width      = 0;
    int window_depth      = CopyFromParent;
    int window_class      = CopyFromParent;
    Visual *window_visual = CopyFromParent;

    int attribute_value_mask = CWBackPixel | CWEventMask;

    XSetWindowAttributes window_attributes = {};
    window_attributes.background_pixel = 0xff000000;
    window_attributes.event_mask       = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;
    window_attributes.override_redirect = true; //override the position of the window, todo(enlynn): do i want to do this?

    Window main_window = XCreateWindow(main_display, root_window, window_x, window_y, width, height,
        border_width, window_depth, window_class, window_visual, attribute_value_mask, &window_attributes
    );

    // When a window is created, the window manager might add GUI components at the top of the window containing
    // the usual "Close, Minimize, Maximize" buttons. So if a user selects the "X" (close) button, the window manager
    // will destroy the window for us and send a DestroyNotify event. Registering for the WM_DELETE_WINDOW will
    // prevent (hopefully) the window manager from cleaning up the window before we can gracefully exit. Otherwise,
    // calling XDestroyWindow/XCloseDisplay will cause the client to crash.
    //
    // Todo(enlynn): Does the same need to happen for other title bar options?
    Atom delete = XInternAtom(main_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(main_display, main_window, &delete, 1);

    XStoreName(main_display, main_window, window_title);
    XMapWindow(main_display, main_window);
    //Note(enlynn): XNextEvent calls XFlush when the event queue is empty

    x11_window_t *window = (x11_window_t*)window_i;
    window->display                 = main_display;
    window->handle                  = main_window;
    window->delete_atom             = delete;
    window->is_live                 = true;
    window->destroy_nodify_recieved = false;
}

fn_export void
os_window_destroy(struct os_window_o *window_i)
{
    x11_window_t *window = (x11_window_t*)window_i;
    window->is_live = false;

    // In the case that we recieved the DestroyNotify event but did not catch the
    // delete atom, we should avoid destroying the window because the window manager
    // has likely done that already.
    //
    // Note(enlynn): I'm not sure if this is worth checking since I have registered
    //               the delete atom.
    if (window->destroy_nodify_recieved)
    {
        XDestroyWindow(window->display, window->handle);
        XCloseDisplay(window->display);

    }
    ZERO_STRUCT(window);
}

fn_export void
os_window_show(struct os_window_o *window, os_window_show_flags_t flags)
{
    //todo(enlynn): added for completeness, but not needed for now.
}

fn_export bool
os_window_is_valid(struct os_window_o *window_i)
{
    x11_window_t *window = (x11_window_t*)window_i;
    return window->is_live;
}

fn_export void
os_window_get_dims(struct os_window_o *window_i, u32 *width, u32 *height)
{
    x11_window_t *window = (x11_window_t*)window_i;

    XWindowAttributes window_attributes;
    Status res = XGetWindowAttributes(window->display, window->handle, &window_attributes);

    if (res != 0)
    {
        *width  = window_attributes.width;
        *height = window_attributes.height;
    }
    else
    {
        *width  = 0;
        *height = 0;
    }
}

fn_export void
os_window_process_pending_messages(struct os_window_o *window_i)
{
    x11_window_t *window = (x11_window_t*)window_i;

    while (window->is_live && XPending(window->display)) {
        XEvent event = {};
        XNextEvent(window->display, &event); //note: blocking, but checking for pending events should fix this

        if (event.xclient.data.l[0] == window->delete_atom)
        {
            // User pressesd the "X" button or window manager decided to close the window.
            window->is_live = false;
            break;
        }

        switch (event.type) {
            case UnmapNotify:
            {
                XMapWindow(window->display, window->handle); //todo: is this right?
            } break;

            case DestroyNotify:
            {
                window->is_live = false;
            } break;

            case KeyPress:
            case KeyRelease:
            {
                XKeyPressedEvent *key_event = (XKeyPressedEvent*)&event;
                if (key_event->keycode == XKeysymToKeycode(window->display, XK_Escape))
                {
                    window->is_live = false;
                }
            } break;
        }
    }
}
