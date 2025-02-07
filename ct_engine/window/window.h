#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <std/types.h>

struct os_window_o;

enum os_window_surface_type
{
    OS_SURFACE_UNKNOWN,
    OS_SURFACE_WIN32,
    OS_SURFACE_X11,
    OS_SURFACE_WAYLAND,
};

struct os_window_surface
{
    enum os_window_surface_type type;
    union {
        struct {
            void *win32_module;
            void *win32_window;
        };
        struct {
            unsigned long  x11_window;
            void          *x11_display;
        };
        struct {
            void *wayland_surface;
            void *wayland_display;
        };
    };
};

fn_export void os_window_create(size_t width, size_t height, const char* window_title, struct os_window_o *window, size_t *window_size);
fn_export void os_window_destroy(struct os_window_o *window);

typedef enum
{
	WINDOW_SHOW_FLAG_NORMAL = 0,
	WINDOW_SHOW_FLAG_HIDE,
	WINDOW_SHOW_FLAG_MINIMIZED,
	WINDOW_SHOW_FLAG_MAXIMIZED,
} os_window_show_flags_t;

fn_export void os_window_show(struct os_window_o *window, os_window_show_flags_t flags);
fn_export bool os_window_is_valid(struct os_window_o *window_i);
fn_export void os_window_get_dims(struct os_window_o *window, u32 *width, u32 *height);
fn_export void os_window_process_pending_messages(struct os_window_o *window_i);
fn_export struct os_window_surface os_window_get_surface(struct os_window_o *self);

#endif //_WINDOW_H_
