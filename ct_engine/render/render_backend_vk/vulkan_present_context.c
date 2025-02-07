#include "vulkan_present_context.h"
#include "vulkan_graphics_context.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_types.h"

#include <std/log.h>

void
vulkan_present_context_init(struct vulkan_present_context_t *self, struct gpu_device_t *device)
{
    self->queue_index = device->gpu.queues.present;
    self->queue       = device->present_queue;
    self->frame_index = 0;

    VkCommandPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = self->queue_index;

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

        self->present_semaphores[i] = create_semaphore(device);
        self->render_semaphores[i]  = create_semaphore(device);

        // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
        // This will prevent the application from waiting indefinitely for the first frame to render since it
        // cannot be rendered until a frame is "rendered" before it.
        self->render_fences[i] = create_fence(device, true);
    }
}

void
vulkan_present_context_deinit(struct vulkan_present_context_t *self, struct gpu_device_t *device)
{
    FOR_RANGE(u32, i, c_max_buffered_frames)
    {
        if (self->present_semaphores[i])
        {
            destroy_semaphore(device, self->present_semaphores[i]);
        }

        if (self->render_semaphores[i])
        {
            destroy_semaphore(device, self->render_semaphores[i]);
        }

        if (self->render_fences[i])
        {
            destroy_fence(device, self->render_fences[i]);
        }
    }

    vkDestroyCommandPool(device->handle, self->command_pool, NULL);
}

void
vulkan_present_context_present(struct vulkan_present_context_t *self, struct vulkan_graphics_context_t *graphics_context,
    struct gpu_device_t       *device,
    struct vulkan_swapchain_t *swapchain,
    struct allocated_image_t  *presented_image)
{
    //
    // Acquire Next Swapchain Image
    //

    // Wait for the execution of the current frame to complete. The fence being free will allow this one to move on.
    //   Timeout of 1s
    VkResult result = vkWaitForFences(device->handle, 1, &self->render_fences[self->frame_index], VK_TRUE, 1000000000);
    if (result != VK_SUCCESS)
    {
        LOG_WARN("swapchain :: begin_frame :: In-flight fence wait failure!");
        return;
    }

    // Reset the fence for use on the next frame
    vkResetFences(device->handle, 1, &self->render_fences[self->frame_index]);

    // Acquire the next swapchain image. Timeout of 1s
    // present_semaphore will be signaled when we are ready to render into the swapchain image.
    result = vkAcquireNextImageKHR(device->handle, swapchain->handle, 1000000000,
        self->present_semaphores[self->frame_index], NULL, &swapchain->swapchain_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        swapchain_invalidate(swapchain);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("swapchain :: begin_frame :: Failed to acquire swapchain image!");
        return;
    }

    //
    // Copy Final Scene Image to the Swapchain
    //

    //todo: insert semaphore to make sure render and compute contexts have finished executing
    //      before the present command buffer can be run.

    vulkan_command_buffer_t *cmd_buffer = &self->command_buffers[self->frame_index];
    vulkan_command_buffer_reset(cmd_buffer);
    vulkan_command_buffer_begin_recording(cmd_buffer);

    VkImage swapchain_image = get_swapchain_image(swapchain);

    // todo: copy final framebuffer into the swapchain
    {
        VkExtent3D swapchain_extent = swapchain_get_extent(swapchain);

        VkExtent2D src_extent = { presented_image->dims.width, presented_image->dims.height };
        VkExtent2D dst_extent = { swapchain_extent.width,      swapchain_extent.height      };

        vulkan_command_buffer_transition_image(cmd_buffer, presented_image->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vulkan_command_buffer_transition_image(cmd_buffer, swapchain_image,        VK_IMAGE_LAYOUT_UNDEFINED,                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vulkan_command_buffer_copy_image_to_image(cmd_buffer, presented_image->image, src_extent, swapchain_image, dst_extent);
    }

    vulkan_command_buffer_transition_image(cmd_buffer, swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vulkan_command_buffer_end_recording(cmd_buffer);
    VkCommandBufferSubmitInfo cmd_buffer_si = vulkan_command_buffer_get_submit_info(cmd_buffer);

    VkSemaphore render_sem  = self->render_semaphores[self->frame_index];
    VkSemaphore present_sem = self->present_semaphores[self->frame_index];

    u64 graphics_wait_value   = graphics_context->semaphore_value;
    u64 graphics_signal_value = graphics_context->semaphore_value + 1;
    graphics_context->semaphore_value = graphics_signal_value;

    const u64 wait_semaphore_values[2] = {
        graphics_wait_value, // value for graphics "timeline" semaphore
        0                    // ignored, swapchhain "binary" semaphore.
    };

    const VkSemaphore wait_semaphores[2] = {
        graphics_context->timeline_semaphore, // Track timeline semaphore work completion
        present_sem,                          // Unblock presentation
    };

    //todo: determine if there is a more optimal stage to wait on
    const VkPipelineStageFlags wait_stages[2] = {
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    };

    const u64 signal_semaphore_values[2] = {
        graphics_signal_value, // value for graphics "timeline" semaphore
        0                      // ignored, swapchhain "binary" semaphore.
    };

    //todo: i don't think we need to signal the timeline_semaphore here!
    const VkSemaphore signal_semaphores[2] = {
        graphics_context->timeline_semaphore, // Track timeline semaphore work completion
        render_sem,                           // Unblock presentation
    };

    VkTimelineSemaphoreSubmitInfo timeline_info = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timeline_info.waitSemaphoreValueCount   = ARRAY_COUNT(wait_semaphore_values);
    timeline_info.pWaitSemaphoreValues      = wait_semaphore_values;
    timeline_info.signalSemaphoreValueCount = ARRAY_COUNT(signal_semaphore_values);
    timeline_info.pSignalSemaphoreValues    = signal_semaphore_values;

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.pNext                = &timeline_info;
    submit_info.waitSemaphoreCount   = ARRAY_COUNT(wait_semaphores);
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd_buffer->handle;
    submit_info.signalSemaphoreCount = ARRAY_COUNT(signal_semaphores);
    submit_info.pSignalSemaphores    = signal_semaphores;

    VK_CHECK_RESULT(vkQueueSubmit(self->queue, 1, &submit_info, self->render_fences[self->frame_index]),
        "Failed to submit command buffer on present queue");

    //
    // Present Swapchain Image
    //

    // Return the image to the swapchain for presentation.
    VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.pNext              = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &self->render_semaphores[self->frame_index]; // Do not present until this semaphore has been signaled
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain->handle;
    present_info.pImageIndices      = &swapchain->swapchain_index;
    present_info.pResults           = NULL;

    result = vkQueuePresentKHR(self->queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
        LOG_WARN("swapchain :: present_frame :: vkQueuePresentKHR returned out of date or suboptimal.");
        swapchain_invalidate(swapchain);
    }
    else if (result != VK_SUCCESS)
    {
        LOG_FATAL("Failed to present swap chain image!");
    }

    self->frame_index = (self->frame_index + 1) % c_max_buffered_frames;
}
