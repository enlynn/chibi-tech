#include "vulkan_command_buffer.h"
#include "vulkan_types.h"

#include <std/cassert.h>

#include <math.h>

fn_inline VkCommandBufferBeginInfo
make_command_buffer_begin_info(VkCommandBufferUsageFlags usage_flags)
{
    VkCommandBufferBeginInfo result = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    result.flags = usage_flags;
    return result;
}

fn_inline VkCommandBufferSubmitInfo
make_command_buffer_submit_info(VkCommandBuffer cmd_buffer)
{
    VkCommandBufferSubmitInfo result = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    result.commandBuffer = cmd_buffer;
    return result;
}

fn_inline VkImageSubresourceRange
make_image_subresource_range(VkImageAspectFlags aspect_mask)
{
    VkImageSubresourceRange result = {};
    result.aspectMask     = aspect_mask;
    result.baseMipLevel   = 0;
    result.levelCount     = VK_REMAINING_MIP_LEVELS;
    result.baseArrayLayer = 0;
    result.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    return result;
}

void
vulkan_command_buffer_init(struct vulkan_command_buffer_t *self, VkCommandBuffer handle)
{
    self->handle         = handle;
    self->state          = CBS_CLOSED;
    self->bound_pipeline = NULL;
}

void
vulkan_command_buffer_begin_recording(struct vulkan_command_buffer_t *self)
{
    ASSERT(self->state == CBS_RESET);

    VkCommandBufferBeginInfo cmd_begin_info = make_command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK_RESULT(vkBeginCommandBuffer(self->handle, &cmd_begin_info),
        "Failed to begin command buffer");

    self->state = CBS_OPEN;
}

void
vulkan_command_buffer_end_recording(struct vulkan_command_buffer_t *self)
{
    ASSERT(self->state == CBS_OPEN);

    vkEndCommandBuffer(self->handle);
    self->state = CBS_CLOSED;
}

void
vulkan_command_buffer_reset(struct vulkan_command_buffer_t *self)
{
    ASSERT(self->state == CBS_CLOSED);

    vkResetCommandBuffer(self->handle, 0);

    self->state          = CBS_RESET;
    self->bound_pipeline = NULL;
}

VkCommandBufferSubmitInfo
vulkan_command_buffer_get_submit_info(struct vulkan_command_buffer_t *self)
{
    ASSERT(self->state == CBS_CLOSED);
    return make_command_buffer_submit_info(self->handle);
}

void
vulkan_command_buffer_transition_image(struct vulkan_command_buffer_t *self, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout)
{
    ASSERT(self->state == CBS_OPEN);

    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    barrier.srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    barrier.oldLayout        = current_layout;
    barrier.newLayout        = new_layout;
    barrier.subresourceRange = make_image_subresource_range(aspect_mask);
    barrier.image            = image;


    // note: can send multiple image barriers at once to improve performance
    VkDependencyInfo dep_info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2(self->handle, &dep_info);
}

void
vulkan_command_buffer_clear_color_image(struct vulkan_command_buffer_t *self, VkImage image, VkClearColorValue *clear_value)
{
    ASSERT(self->state == CBS_OPEN);

    VkImageSubresourceRange clear_range = make_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(self->handle, image, VK_IMAGE_LAYOUT_GENERAL, clear_value, 1, &clear_range);
}

void
vulkan_command_buffer_bind_pipeline(struct vulkan_command_buffer_t *self, VkPipeline pipeline, VkPipelineBindPoint bind_point)
{
    ASSERT(self->state == CBS_OPEN);

    if( self->bound_pipeline != pipeline)
    {
        self->bound_pipeline = pipeline;
    }

    vkCmdBindPipeline(self->handle, bind_point, pipeline);
}

void
vulkan_command_buffer_bind_descriptor_sets(struct vulkan_command_buffer_t *self,
    VkPipelineLayout pipeline_layout, VkPipelineBindPoint bind_point,
    u32 first_set, VkDescriptorSet *descriptor_sets, u32 descriptor_count)
{
    ASSERT(self->state == CBS_OPEN);

    //todo: dynamic descriptor sets
    vkCmdBindDescriptorSets(self->handle, bind_point, pipeline_layout, first_set, descriptor_count, descriptor_sets, 0, NULL);
}

void
vulkan_command_buffer_bind_push_constants(struct vulkan_command_buffer_t *self, VkPipelineLayout pipeline_layout, VkShaderStageFlagBits stage,
    void *push_consts, u32 push_consts_size, u32 offset)
{
    ASSERT(self->state == CBS_OPEN);
    vkCmdPushConstants(self->handle, pipeline_layout, stage, offset, push_consts_size, push_consts);
}

void
vulkan_command_buffer_dispatch(struct vulkan_command_buffer_t *self, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
    ASSERT(self->state == CBS_OPEN);
    vkCmdDispatch(self->handle, group_count_x, group_count_y, group_count_z);
}

void
vulkan_command_buffer_begin_rendering(struct vulkan_command_buffer_t *self, VkRenderingInfo render_info)
{
    ASSERT(self->state == CBS_OPEN);
    vkCmdBeginRendering(self->handle, &render_info);
}

void
vulkan_command_buffer_end_rendering(struct vulkan_command_buffer_t *self)
{
    ASSERT(self->state == CBS_OPEN);
    vkCmdEndRendering(self->handle);
}

void
vulkan_command_buffer_set_viewport(struct vulkan_command_buffer_t *self, u32 width, u32 height, u32 offset_x, u32 offset_y)
{
    ASSERT(self->state == CBS_OPEN);

    VkViewport viewport = {};
    viewport.x        = (f32)offset_x;
    viewport.y        = (f32)offset_y;
    viewport.width    = (f32)width;
    viewport.height   = (f32)height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( self->handle, 0, 1, &viewport);
}

void
vulkan_command_buffer_set_scissor(struct vulkan_command_buffer_t *self, u32 width, u32 height)
{
    ASSERT(self->state == CBS_OPEN);

    VkRect2D scissor = {};
   	scissor.offset.x      = 0;
   	scissor.offset.y      = 0;
   	scissor.extent.width  = width;
   	scissor.extent.height = height;

    vkCmdSetScissor(self->handle, 0, 1, &scissor);
}

void
vulkan_command_buffer_draw(struct vulkan_command_buffer_t *self, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    ASSERT(self->state == CBS_OPEN);
    vkCmdDraw(self->handle, vertex_count, instance_count, first_vertex, first_instance);
}

void
vulkan_command_buffer_copy_buffer(struct vulkan_command_buffer_t *self,
    allocated_buffer_t *dst_buffer, VkDeviceSize dst_offset,
    allocated_buffer_t *src_buffer, VkDeviceSize src_offset,
    VkDeviceSize copy_size)
{
    ASSERT(self->state == CBS_OPEN);

    VkBufferCopy copy_info = {};
    copy_info.srcOffset = src_offset;
    copy_info.dstOffset = dst_offset;
    copy_info.size      = copy_size;

    vkCmdCopyBuffer(self->handle, src_buffer->buffer, dst_buffer->buffer, 1, &copy_info);
}

void
vulkan_command_buffer_copy_image_to_image(struct vulkan_command_buffer_t *self,
    VkImage source,      VkExtent2D src_size,
    VkImage destination, VkExtent2D dst_size)
{
    ASSERT(self->state == CBS_OPEN);

    VkImageBlit2 blit_region = { VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
    blit_region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel       = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount     = 1;
    blit_region.srcOffsets[0]                 = (VkOffset3D){ 0, 0, 0 },
    blit_region.srcOffsets[1]                 = (VkOffset3D){ (s32)src_size.width, (s32)src_size.height, 1 }; //@fixme: is z = 1 a typo?
    blit_region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel       = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount     = 1;
    blit_region.dstOffsets[0]                 = (VkOffset3D){ 0, 0, 0 },
    blit_region.dstOffsets[1]                 = (VkOffset3D){ (s32)dst_size.width, (s32)dst_size.height, 1 }; //@fixme: is z = 1 a typo?

    VkBlitImageInfo2 blit_info = { VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
    blit_info.srcImage       = source;
    blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blit_info.dstImage       = destination;
    blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_info.regionCount    = 1;
    blit_info.pRegions       = &blit_region;
    blit_info.filter         = VK_FILTER_LINEAR;

    vkCmdBlitImage2(self->handle, &blit_info);
}

void
vulkan_command_buffer_copy_buffer_to_image(struct vulkan_command_buffer_t *self, allocated_buffer_t *upload_buffer, allocated_image_t *dst_image, VkExtent3D size)
{
    ASSERT(self->state == CBS_OPEN);

    VkBufferImageCopy copy_region = {};
    copy_region.bufferOffset                    = 0;
    copy_region.bufferRowLength                 = 0;
    copy_region.bufferImageHeight               = 0;
    copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel       = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount     = 1;
    copy_region.imageOffset                     = (VkOffset3D){ 0, 0, 0 };
    copy_region.imageExtent                     = size;

    vkCmdCopyBufferToImage(self->handle, upload_buffer->buffer, dst_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
}

void
vulkan_command_buffer_bind_index_buffer(struct vulkan_command_buffer_t *self, allocated_buffer_t *index_buffer)
{
    vkCmdBindIndexBuffer(self->handle, index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
}

void
vulkan_command_buffer_draw_indexed(struct vulkan_command_buffer_t *self, u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance)
{
    vkCmdDrawIndexed(self->handle, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void
vulkan_command_buffer_generate_mipmaps(struct vulkan_command_buffer_t *self, allocated_image_t *image)
{
    ASSERT(self->state == CBS_OPEN);

    VkExtent2D image_size = { image->dims.width, image->dims.height };
    u32 mip_count = (u32)floorf(log2f((f32)fast_max(image_size.width, image_size.height))) + 1;

    FOR_RANGE(u32, i, mip_count)
    {
        VkExtent2D half_size = { image_size.width / 2, image_size.height / 2 };

        VkImageSubresourceRange subresource_range = make_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
        subresource_range.levelCount   = 1;
        subresource_range.baseMipLevel = i;

        VkImageMemoryBarrier2 image_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        image_barrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        image_barrier.srcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT;
        image_barrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        image_barrier.dstAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
        image_barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_barrier.srcQueueFamilyIndex = 0;
        image_barrier.dstQueueFamilyIndex = 0;
        image_barrier.image               = image->image,
        image_barrier.subresourceRange    = subresource_range;

        VkDependencyInfo dep_info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dep_info.imageMemoryBarrierCount = 1;
        dep_info.pImageMemoryBarriers    = &image_barrier;

        vkCmdPipelineBarrier2(self->handle, &dep_info);

        if (i < mip_count - 1)
        {
            VkImageBlit2 blit_region = { VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
            blit_region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit_region.srcSubresource.mipLevel       = i;
            blit_region.srcSubresource.baseArrayLayer = 0;
            blit_region.srcSubresource.layerCount     = 1;
            blit_region.srcOffsets[0]                 = (VkOffset3D){ 0, 0, 0 },
            blit_region.srcOffsets[1]                 = (VkOffset3D){ (s32)image_size.width, (s32)image_size.height, 1 }; //@fixme: is z = 1 a typo?
            blit_region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit_region.dstSubresource.mipLevel       = i + 1;
            blit_region.dstSubresource.baseArrayLayer = 0;
            blit_region.dstSubresource.layerCount     = 1;
            blit_region.dstOffsets[0]                 = (VkOffset3D){ 0, 0, 0 },
            blit_region.dstOffsets[1]                 = (VkOffset3D){ (s32)half_size.width, (s32)half_size.height, 1 }; //@fixme: is z = 1 a typo?

            VkBlitImageInfo2 blit_info = { VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
            blit_info.srcImage       = image->image;
            blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blit_info.dstImage       = image->image;
            blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blit_info.regionCount    = 1;
            blit_info.pRegions       = &blit_region;
            blit_info.filter         = VK_FILTER_LINEAR;

            vkCmdBlitImage2(self->handle, &blit_info);

            image_size = half_size;
        }
    }

    // transition all mip levels into the final read_only layout
    vulkan_command_buffer_transition_image(self, image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
