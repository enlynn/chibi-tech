#ifndef _VULKAN_COMMAND_BUFFER_H_
#define _VULKAN_COMMAND_BUFFER_H_

#include "vk.h"

struct vulkan_command_buffer_t;
struct allocated_buffer_t;
struct allocated_image_t;

void vulkan_command_buffer_init(struct vulkan_command_buffer_t *self, VkCommandBuffer handle);
void vulkan_command_buffer_begin_recording(struct vulkan_command_buffer_t *self);
void vulkan_command_buffer_end_recording(struct vulkan_command_buffer_t *self);
void vulkan_command_buffer_reset(struct vulkan_command_buffer_t *self);
VkCommandBufferSubmitInfo vulkan_command_buffer_get_submit_info(struct vulkan_command_buffer_t *self);
void vulkan_command_buffer_transition_image(struct vulkan_command_buffer_t *self, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);
void vulkan_command_buffer_clear_color_image(struct vulkan_command_buffer_t *self, VkImage image, VkClearColorValue *clear_value);
void vulkan_command_buffer_bind_pipeline(struct vulkan_command_buffer_t *self, VkPipeline pipeline, VkPipelineBindPoint bind_point);
void vulkan_command_buffer_dispatch(struct vulkan_command_buffer_t *self, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void vulkan_command_buffer_begin_rendering(struct vulkan_command_buffer_t *self, VkRenderingInfo render_info);
void vulkan_command_buffer_end_rendering(struct vulkan_command_buffer_t *self);
void vulkan_command_buffer_set_viewport(struct vulkan_command_buffer_t *self, u32 width, u32 height, u32 offset_x, u32 offset_y);
void vulkan_command_buffer_set_scissor(struct vulkan_command_buffer_t *self, u32 width, u32 height);
void vulkan_command_buffer_copy_buffer_to_image(struct vulkan_command_buffer_t *self, struct allocated_buffer_t *upload_buffer, struct allocated_image_t *dst_image, VkExtent3D size);
void vulkan_command_buffer_bind_index_buffer(struct vulkan_command_buffer_t *self, struct allocated_buffer_t *index_buffer);
void vulkan_command_buffer_draw(struct vulkan_command_buffer_t *self, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void vulkan_command_buffer_draw_indexed(struct vulkan_command_buffer_t *self, u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance);
void vulkan_command_buffer_generate_mipmaps(struct vulkan_command_buffer_t *self, struct allocated_image_t *image);

void vulkan_command_buffer_bind_descriptor_sets(struct vulkan_command_buffer_t *self,
    VkPipelineLayout pipeline_layout, VkPipelineBindPoint bind_point,
    u32 first_set, VkDescriptorSet *descriptor_sets, u32 descriptor_count);

// convienence wrapper for binding push constants
#define VK_CMD_BUFFER_BIND_PUSH_CONSTANTS(self, pipeline_layout, stage, p_push_consts, offset) \
    vulkan_command_buffer_bind_push_constants(self, pipeline_layout, stage, (void*)(p_push_consts), sizeof(typeof(*p_push_consts)), offset)

void vulkan_command_buffer_bind_push_constants(struct vulkan_command_buffer_t *self,
    VkPipelineLayout pipeline_layout, VkShaderStageFlagBits stage,
    void *push_consts, u32 push_consts_size, u32 offset);

void vulkan_command_buffer_copy_buffer(struct vulkan_command_buffer_t *self,
    struct allocated_buffer_t *dst_buffer, VkDeviceSize dst_offset,
    struct allocated_buffer_t *src_buffer, VkDeviceSize src_offset,
    VkDeviceSize copy_size);

void vulkan_command_buffer_copy_image_to_image(struct vulkan_command_buffer_t *self,
    VkImage source,      VkExtent2D src_size,
    VkImage destination, VkExtent2D dst_size);

#endif //_VULKAN_COMMAND_BUFFER_H_
