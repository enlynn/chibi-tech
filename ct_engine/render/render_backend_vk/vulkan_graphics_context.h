#ifndef _VULKAN_GRAPHICS_CONTEXT_H_
#define _VULKAN_GRAPHICS_CONTEXT_H_

struct vulkan_graphics_context_t;
struct gpu_device_t;
struct allocated_image_t;

void vulkan_graphics_context_init(struct vulkan_graphics_context_t *self, struct gpu_device_t *device);
void vulkan_graphics_context_deinit(struct vulkan_graphics_context_t *self, struct gpu_device_t *device);
void vulkan_graphics_context_submit(struct vulkan_graphics_context_t *self /* todo: wait on other queues */);
void vulkan_graphics_context_begin(struct vulkan_graphics_context_t *self, struct gpu_device_t *device);
void vulkan_graphics_context_clear_color_image(struct vulkan_graphics_context_t *self, struct allocated_image_t *image, float color[4]);

#endif //_VULKAN_GRAPHICS_CONTEXT_H_
