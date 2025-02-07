#include "renderer.h"
#include "gpu_device.h"

#include <std/mem.h>

typedef struct renderer_t
{
    struct gpu_device_t *gpu_device;
    int                  gpu_device_size;
} renderer_t;

fn_export void
renderer_create(renderer_info_t *info, struct renderer_t *renderer, int *renderer_size)
{
    *renderer_size = sizeof(renderer_t);
    if (!renderer) return;

    gpu_device_create(NULL, NULL, &renderer->gpu_device_size);
    renderer->gpu_device = SYS_ALLOC(renderer->gpu_device_size);

    gpu_device_info_t device_info = {
	    .software_name    = info->software_name,
		.software_version = info->software_version,
		.native_surface   = info->native_surface,
	};

    gpu_device_create(&device_info, renderer->gpu_device, &renderer->gpu_device_size);
}

fn_export void
renderer_destroy(struct renderer_t *renderer)
{
    gpu_device_destroy(renderer->gpu_device);
    SYS_FREE(renderer->gpu_device);
}

fn_export void
renderer_on_resize(struct renderer_t *renderer, u32 width, u32 height)
{
    gpu_device_on_resize(renderer->gpu_device, width, height);
}

fn_export void
renderer_on_render(struct renderer_t *renderer)
{
    gpu_device_begin_frame(renderer->gpu_device);
    gpu_device_end_frame(renderer->gpu_device);
}
