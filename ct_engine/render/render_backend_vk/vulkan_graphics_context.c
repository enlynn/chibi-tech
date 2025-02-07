#include "vulkan_graphics_context.h"
#include "vulkan_types.h"
#include "vulkan_device.h"
#include "vulkan_command_buffer.h"

#include <std/mem.h>

void
vulkan_graphics_context_init(struct vulkan_graphics_context_t *self, struct gpu_device_t *device)
{
    self->queue           = device->graphics_queue;
    self->frame_index     = 0;
    self->semaphore_value = 0;

    VkCommandPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = device->gpu.queues.graphics;

    VK_CHECK_RESULT(vkCreateCommandPool(device->handle, &pool_ci, NULL, &self->command_pool),
        "Failed to create vulkan command pool");

    VkCommandBufferAllocateInfo cmd_buffer_ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmd_buffer_ai.commandPool        = self->command_pool;
    cmd_buffer_ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_ai.commandBufferCount = c_max_buffered_frames;

    VkCommandBuffer command_buffers[c_max_buffered_frames];
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device->handle, &cmd_buffer_ai, &command_buffers[0]),
        "Failed to allocated command buffers");

    FOR_RANGE(u32, i, c_max_buffered_frames)
    {
        vulkan_command_buffer_init(&self->command_buffers[i], command_buffers[i]);
        self->fence_values[i] = 0;
    }

    self->timeline_semaphore = create_timeline_semaphore(device, self->semaphore_value);
}

void
vulkan_graphics_context_deinit(struct vulkan_graphics_context_t *self, struct gpu_device_t *device)
{
    destroy_semaphore(device, self->timeline_semaphore);
    vkDestroyCommandPool(device->handle, self->command_pool, NULL);
}

void
vulkan_graphics_context_submit(struct vulkan_graphics_context_t *self /* todo: wait on other queues */)
{
    vulkan_command_buffer_t *cmd_buffer = &self->command_buffers[self->frame_index];

    // only bother submitting if we've opened the command list
    if (cmd_buffer->state != CBS_OPEN) return;

    vulkan_command_buffer_end_recording(cmd_buffer);
    VkCommandBufferSubmitInfo cmd_buffer_si = vulkan_command_buffer_get_submit_info(cmd_buffer);

    const u64 wait_value   = self->semaphore_value;
    const u64 signal_value = self->semaphore_value + 1;

    const VkPipelineStageFlags wait_stages[1] = {
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
    };

    VkTimelineSemaphoreSubmitInfo timeline_info = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timeline_info.waitSemaphoreValueCount   = 1;
    timeline_info.pWaitSemaphoreValues      = &wait_value;
    timeline_info.signalSemaphoreValueCount = 1;
    timeline_info.pSignalSemaphoreValues    = &signal_value;

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.pNext                 = &timeline_info;
    submit_info.waitSemaphoreCount    = 1;
    submit_info.pWaitSemaphores       = &self->timeline_semaphore;
    submit_info.pWaitDstStageMask     = wait_stages; //todo: determine if there is a more optimal stage to wait on
    submit_info.signalSemaphoreCount  = 1;
    submit_info.pSignalSemaphores     = &self->timeline_semaphore;
    submit_info.commandBufferCount    = 1;
    submit_info.pCommandBuffers       = &cmd_buffer->handle;

    VK_CHECK_RESULT(vkQueueSubmit(self->queue, 1, &submit_info, NULL),
        "Failed to submit command buffer on present queue");

    self->semaphore_value = signal_value;
    self->fence_values[self->frame_index] = signal_value;

    self->frame_index = (self->frame_index + 1) % c_max_buffered_frames;
}

void
vulkan_graphics_context_begin(struct vulkan_graphics_context_t *self, struct gpu_device_t *device)
{
    vulkan_command_buffer_t *cmd_buffer = &self->command_buffers[self->frame_index];

    // wait for the command buffer to finish executing (if not done already)
    VkSemaphoreWaitInfo wait_info = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
    wait_info.flags          = 0;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores    = &self->timeline_semaphore;
    wait_info.pValues        = &self->fence_values[self->frame_index];
    vkWaitSemaphores(device->handle, &wait_info, UINT64_MAX);

    vulkan_command_buffer_reset(cmd_buffer);
    vulkan_command_buffer_begin_recording(cmd_buffer);
}

void
vulkan_graphics_context_clear_color_image(struct vulkan_graphics_context_t *self, struct allocated_image_t *image, f32 color[4])
{
    VkClearColorValue clear_value = {};
    mem_copy(&clear_value.float32, color, sizeof(f32[4]));

    vulkan_command_buffer_t *cmd_buffer = &self->command_buffers[self->frame_index];
    vulkan_command_buffer_transition_image(cmd_buffer, image->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vulkan_command_buffer_clear_color_image(cmd_buffer, image->image, &clear_value);
    vulkan_command_buffer_transition_image(cmd_buffer, image->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}
