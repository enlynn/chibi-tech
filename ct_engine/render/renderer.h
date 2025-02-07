#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <std/types.h>

struct renderer_t;
struct os_window_surface;

typedef struct {
    u32                       software_version;
    const char               *software_name;
    struct os_window_surface *native_surface;
} renderer_info_t;

fn_export void renderer_create(renderer_info_t *info, struct renderer_t *renderer, int *renderer_size);
fn_export void renderer_destroy(struct renderer_t *renderer);
fn_export void renderer_on_resize(struct renderer_t *renderer, u32 width, u32 height);
fn_export void renderer_on_render(struct renderer_t *renderer);

#endif //_RENDERER_H_
