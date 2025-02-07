#include "vulkan_device.h"
#include "vulkan_types.h"
#include "vulkan_present_context.h"
#include "vulkan_graphics_context.h"

#include "../gpu_device.h"

#include <ct_engine.h>
#include <std/log.h>
#include <std/mem.h>
#include <std/str.h>
#include <window/window.h>

#include <math.h>

#if GPU_ENABLE_DEBUG_LAYER
    var_global const bool c_enable_debug_layer = true;
#else
    var_global const bool c_enable_debug_layer = false;
#endif

var_global const char *c_portability_extension_name = "VK_KHR_portability_subset";

var_global const char *c_validation_layers[1] = {
        "VK_LAYER_KHRONOS_validation"
};
#define c_validation_layers_count ARRAY_COUNT(c_validation_layers)

var_global const char *c_device_extensions[3] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};
#define c_device_extensions_count ARRAY_COUNT(c_device_extensions)

var_global const char *c_instance_extensions[] = {
#if CT_PLATFORM_WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif CT_PLATFORM_LINUX
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    //VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#else
#  error Vulkan Surface extension not supported for platform.
#endif
#if GPU_ENABLE_DEBUG_LAYER
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};
#define c_instance_extensions_count ARRAY_COUNT(c_instance_extensions)

var_global struct allocated_image_t g_scene = {};

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      MessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData)
{
    if ((MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
    {
        LOG_INFO("%s", pCallbackData->pMessage);
    }
    else if ((MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
    {
        LOG_INFO("%s", pCallbackData->pMessage);
    }
    else if ((MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
    {
        LOG_WARN("%s", pCallbackData->pMessage);
    }
    if ((MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
    {
        LOG_ERROR("%s", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

fn_inline void
populate_debug_messenger_ci(VkDebugUtilsMessengerCreateInfoEXT *ci)
{
    ZERO_STRUCT(ci);

    ci->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci->messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci->pfnUserCallback = debug_callback;
}

fn_internal void
enumerate_instance_layer_properties(VkLayerProperties **layers, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkEnumerateInstanceLayerProperties(count, NULL);
	*layers = CT_ALLOC_ARRAY(allocator, VkLayerProperties, *count);

    vkEnumerateInstanceLayerProperties(count, *layers);
}

fn_internal void
enumerate_instance_extensions(VkExtensionProperties **layers, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkEnumerateInstanceExtensionProperties(NULL, count, NULL);
    *layers = CT_ALLOC_ARRAY(allocator, VkExtensionProperties, *count);

    vkEnumerateInstanceExtensionProperties(NULL, count, *layers);
}


fn_internal void
enumerate_gpu_queue_family_properties(VkPhysicalDevice gpu, VkQueueFamilyProperties **properties, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, count, NULL);
    *properties = CT_ALLOC_ARRAY(allocator, VkQueueFamilyProperties, *count);

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, count, *properties);
}

fn_internal void
enumerate_gpu_present_modes(VkPhysicalDevice gpu, VkSurfaceKHR surface, VkPresentModeKHR **present_modes, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, count, NULL);
    *present_modes = CT_ALLOC_ARRAY(allocator, VkPresentModeKHR, *count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, count, *present_modes);
}

fn_internal void
enumerate_gpu_surface_formats(VkPhysicalDevice gpu, VkSurfaceKHR surface, VkSurfaceFormatKHR **surface_formats, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, count, NULL);
    *surface_formats = CT_ALLOC_ARRAY(allocator, VkSurfaceFormatKHR, *count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, count, *surface_formats);
}

fn_internal void
enumerate_device_extensions(VkPhysicalDevice gpu, VkExtensionProperties **extensions, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkEnumerateDeviceExtensionProperties(gpu, NULL, count, NULL);
    *extensions = CT_ALLOC_ARRAY(allocator, VkExtensionProperties, *count);

    vkEnumerateDeviceExtensionProperties(gpu, NULL, count, *extensions);
}

fn_internal void
enumerate_gpus(VkInstance instance, VkPhysicalDevice **gpus, u32 *count)
{
    allocator_t *allocator = get_frame_allocator();

    vkEnumeratePhysicalDevices(instance, count, NULL);
    *gpus = CT_ALLOC_ARRAY(allocator, VkPhysicalDevice, *count);

    vkEnumeratePhysicalDevices(instance, count, *gpus);
}

fn_internal bool
check_validation_layers_support()
{
    VkLayerProperties *available_layers = NULL;
    u32 layers_count = 0;
    enumerate_instance_layer_properties(&available_layers, &layers_count);

    FOR_RANGE(u32, i, c_validation_layers_count)
    {
        const char *expected_layer = c_validation_layers[i];

        bool layer_found = false;
        FOR_RANGE(u32, j, layers_count)
        {
            VkLayerProperties *vk_layer = available_layers + j;
            if (str_cmp(expected_layer, vk_layer->layerName))
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            LOG_ERROR("Failed to find validation layer: %s", expected_layer);
            return false;
        }
    }

    return true;
}

fn_internal bool
check_instance_extensions_support()
{
    VkExtensionProperties *available_extensions = NULL;
    u32 layers_count = 0;
    enumerate_instance_extensions(&available_extensions, &layers_count);

    FOR_RANGE(u32, i, c_instance_extensions_count)
    {
        const char *expected_extension = c_instance_extensions[i];

        bool layer_found = false;
        FOR_RANGE(u32, j, layers_count)
        {
            VkExtensionProperties *vk_extension = available_extensions + j;
            if (str_cmp(expected_extension, vk_extension->extensionName))
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            LOG_ERROR("Failed to find instance extension: %s", expected_extension);
            return false;
        }
    }

    return true;
}

fn_internal void
create_vk_instance(struct gpu_device_t *self, gpu_device_info_t *info)
{
    if (c_enable_debug_layer && !check_validation_layers_support())
    {
        ASSERT_CUSTOM(false, "Requested validation layers, but a layer was not found.");
    }

    ASSERT_CUSTOM(check_instance_extensions_support(), "Failed to find a required Vulkan Instance Extension");

    void *p_next = NULL;
    if (c_enable_debug_layer)
    {
        var_persist VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = {};
        populate_debug_messenger_ci(&debug_messenger_ci);
        p_next = (void*)&debug_messenger_ci;
    }

    VkApplicationInfo app_info  = {0};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = info->software_name;
    app_info.applicationVersion = info->software_version;
    app_info.pEngineName        = c_engine_name;
    app_info.engineVersion      = c_engine_version;
    app_info.apiVersion         = VK_API_VERSION_1_4;

    VkInstanceCreateInfo instance_ci = {0};
    instance_ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pNext                   = p_next;
    instance_ci.pApplicationInfo        = &app_info;
    instance_ci.enabledExtensionCount   = c_instance_extensions_count;
    instance_ci.ppEnabledExtensionNames = c_instance_extensions;
    instance_ci.enabledLayerCount       = c_enable_debug_layer ? c_validation_layers_count : 0;
    instance_ci.ppEnabledLayerNames     = c_enable_debug_layer ? c_validation_layers       : NULL;

    VK_CHECK_RESULT(vkCreateInstance(&instance_ci, NULL, &self->instance),
        "Failed to create instance!");
}

fn_internal void
setup_debug_messenger(struct gpu_device_t *self)
{
#if GPU_ENABLE_DEBUG_LAYER
    VkDebugUtilsMessengerCreateInfoEXT messenger_ci = {0};
    populate_debug_messenger_ci(&messenger_ci);

    const PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        self->instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != NULL)
    {
        VK_CHECK_RESULT(func(self->instance, &messenger_ci, NULL, &self->debug_messenger), "Failed to set up debug messenger!");
        LOG_DEBUG("Vulkan debug messenger initialized.");
    }
    else
    {
        LOG_ERROR("Debug messenger not present. The messenger will not be initialized.");
    }
#endif
}

fn_internal void
destroy_debug_messenger(struct gpu_device_t *self)
{
#if GPU_ENABLE_DEBUG_LAYER
    if (self->debug_messenger)
    {
        const PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            self->instance, "vkDestroyDebugUtilsMessengerEXT");

        if (func != NULL)
        {
            func(self->instance, self->debug_messenger, NULL);
            LOG_DEBUG("Vulkan debug messenger destroy.");
        }
    }
#endif
}

fn_internal void
create_vulkan_surface(struct gpu_device_t *self, struct os_window_surface *surface)
{
#if CT_PLATFORM_WIN32
    ASSERT(surface->type == OS_SURFACE_WIN32);

    VkWin32SurfaceCreateInfoKHR surface_ci = {};
    surface_ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_ci.pNext     = NULL;
    surface_ci.flags     = 0;
    surface_ci.hinstance = surface->win32_module;
    surface_ci.hwnd      = surface->win32_window;

    VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(self->instance, &surface_ci, NULL, &self->surface),
        "Failed to create Win32 vulkan surface"
    );
#elif CT_PLATFORM_LINUX
    ASSERT(surface->type == OS_SURFACE_X11); //wayland not currently supported

    VkXlibSurfaceCreateInfoKHR surface_ci = {};
    surface_ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surface_ci.pNext  = NULL;
    surface_ci.flags  = 0;
    surface_ci.dpy    = surface->x11_display;
    surface_ci.window = surface->x11_window;

    VK_CHECK_RESULT(vkCreateXlibSurfaceKHR(self->instance, &surface_ci, NULL, &self->surface),
        "Failed to create XLib vulkan surface"
    );
#else
#  error create_vulkan_surface not supported for platform.
#endif
}

fn_internal vulkan_queue_families_t
get_queue_families(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice gpu)
{
    vulkan_queue_families_t result = {};
    result.present  = c_invalid_queue_family;
    result.graphics = c_invalid_queue_family;
    result.compute  = c_invalid_queue_family;
    result.transfer = c_invalid_queue_family;

    VkQueueFamilyProperties *queues = NULL;
    u32 queues_count = 0;
    enumerate_gpu_queue_family_properties(gpu, &queues, &queues_count);

    // Iterate  over each queue family and select each queue of based on a score to determine if the queue
    // is a *unique* queue. If no unique queue is found, a duplicate is selected.
    u8 min_transfer_score = 255;
    FOR_RANGE(u32, i, queues_count)
    {
        VkQueueFamilyProperties *family = queues + i;
        u8 current_transfer_score = 0;

        // Graphics queue?
        if (family->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            result.graphics = i;
            current_transfer_score += 1;
        }

        // Compute queue?
        if (family->queueFlags & VK_QUEUE_COMPUTE_BIT) {
            result.compute = i;
            current_transfer_score += 1;
        }

        // Transfer queue?
        if (family->queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // Take the index if it is the current lowest. This increases the likelihood that it is a dedicated transfer queue.
            if (current_transfer_score <= min_transfer_score)
            {
                min_transfer_score = current_transfer_score;
                result.transfer = i;
            }
        }

        // Does this queue family support the present queue? If so, yoink it.
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supports_present),
                        "Failed to query vkGetPhysicalDeviceSurfaceSupportKHR");

        if (supports_present)
        {
            result.present = i;
        }
    }

    // prefer for the present queue to be on the graphics queue, if possible
    if (result.graphics != result.present)
    {
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, result.graphics, surface, &supports_present),
                        "Failed to query vkGetPhysicalDeviceSurfaceSupportKHR");

        if (supports_present)
        {
            result.present = result.graphics;
        }
    }

    return result;
}

fn_internal swapchain_support_info_t
query_swapchain_support_info(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    swapchain_support_info_t result = {};

    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &result.capabilities),
        "Failed to query vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    enumerate_gpu_surface_formats(gpu, surface, &result.formats, &result.formats_count);
    enumerate_gpu_present_modes(gpu, surface, &result.present_modes, &result.present_modes_count);

    return result;
}

fn_internal bool
does_gpu_meet_requirements(struct gpu_device_t *self, VkPhysicalDevice gpu, VkPhysicalDeviceProperties properties, VkPhysicalDeviceFeatures features)
{
    vulkan_queue_families_t queues = get_queue_families(self->instance, self->surface, gpu);

    // missing a desired queue
    if (queues.present  == c_invalid_queue_family) return false;
    if (queues.graphics == c_invalid_queue_family) return false;
    if (queues.compute  == c_invalid_queue_family) return false;
    if (queues.transfer == c_invalid_queue_family) return false;

    swapchain_support_info_t swapchain_info = query_swapchain_support_info(gpu, self->surface);
    if (swapchain_info.formats_count == 0 || swapchain_info.present_modes_count == 0)
    { // Missing a presentable surface.
        return false;
    }

    VkExtensionProperties *extensions;
    u32 extensions_count = 0;
    enumerate_device_extensions(gpu, &extensions, &extensions_count);

    if (extensions_count == 0) return false;

    FOR_RANGE(u32, j, c_device_extensions_count)
    {
        const char *expected_extension = c_device_extensions[j];

        bool found = false;
        FOR_RANGE(u32, i, extensions_count)
        {
            VkExtensionProperties *vk_extension = extensions + i;
            if (str_cmp(expected_extension, vk_extension->extensionName))
            {
                found = true;
                break;
            }
        }

        // could not find a necessary extension
        if (!found) return false;
    }


    // Sampler anisotropy
    static const bool enable_sampler_anisotropy = true;
    if (enable_sampler_anisotropy && !features.samplerAnisotropy)
    {
        return false;
    }

    return true;
}

fn_internal bool
has_format(const VkSurfaceFormatKHR *formats, const u32 formats_count, const VkSurfaceFormatKHR desired_format)
{
    FOR_RANGE(u32, i, formats_count) {
        if (formats[i].format == desired_format.format && formats[i].colorSpace == desired_format.colorSpace) {
            return true;
        }
    }
    return false;
}

fn_internal void
select_surface_formats(VkPhysicalDevice gpu, const VkSurfaceFormatKHR *formats, const u32 formats_count, const bool prefer_hdr,
    VkSurfaceFormatKHR *out_surface_format, VkFormat *out_depth_format)
{
    ASSERT_CUSTOM(!prefer_hdr, "HDR path not supported yet.");

    var_persist const VkSurfaceFormatKHR c_sdr_format = { .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    var_persist const VkSurfaceFormatKHR c_hdr_format = { .format = VK_FORMAT_UNDEFINED,      .colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR       };

    bool format_found = false;
    // If we want HDR, check if HDR is a valid format for this adapter.
    // NOTE(enlynn): Will have to also check the display!
    if (prefer_hdr)
    {
        format_found = has_format(formats, formats_count, c_hdr_format);
        if (format_found) *out_surface_format = c_hdr_format;
    }

    if (!format_found)
    {
        format_found = has_format(formats, formats_count, c_sdr_format);
        if (format_found) *out_surface_format = c_sdr_format;
    }

    // If failed to find the desired SDR and HDR format, choose the first available instead.
    if (!format_found) {
        LOG_WARN("Failed to find the desired SDR or HDR surface format. Choosing first available instead.");

        ASSERT(formats_count > 0);
        *out_surface_format = formats[0];
    }

    // Set the depth format
    *out_depth_format = VK_FORMAT_UNDEFINED;

    var_persist const VkFormat depth_candidates[3] = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
    };

    var_persist const u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    FOR_RANGE(u32, i, ARRAY_COUNT(depth_candidates))
    {
        VkFormat candidate = depth_candidates[i];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(gpu, candidate, &properties);

        const bool has_linear_tiling  = (properties.linearTilingFeatures  & flags) != 0;
        const bool has_optimal_tiling = (properties.optimalTilingFeatures & flags) != 0;
        if (has_linear_tiling || has_optimal_tiling)
        {
            *out_depth_format = candidate;
            break;
        }
    }

    ASSERT_CUSTOM(*out_depth_format != VK_FORMAT_UNDEFINED, "Unable to find valid depth format for swapchain.");
}

fn_internal void
select_gpu(struct gpu_device_t *self)
{
    VkPhysicalDevice *gpus = NULL;
    u32 gpus_count = 0;
    enumerate_gpus(self->instance, &gpus, &gpus_count);

    FOR_RANGE(u32, i, gpus_count)
    {
        VkPhysicalDevice gpu = gpus[i];

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);

        if ((properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) == 0)
        { // only support discrete gpus (for now)
            continue;
        }

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(gpu, &features);

        if (!does_gpu_meet_requirements(self, gpu, properties, features))
        {
            continue;
        }

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memory);

        // Check if device supports local/host visible combo
        bool supports_device_local_host_visible = false;
        FOR_RANGE(u32, i, memory.memoryTypeCount)
        {
            if (((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0))
            {
                supports_device_local_host_visible = true;
                break;
            }
        }

        self->gpu.handle                             = gpu;
        self->gpu.swapchain_info                     = query_swapchain_support_info(gpu, self->surface); //@fixme: right now this is only in temp memory
        self->gpu.properties                         = properties;
        self->gpu.features                           = features;
        self->gpu.memory_properties                  = memory;
        self->gpu.queues                             = get_queue_families(self->instance, self->surface, gpu);
        self->gpu.supports_device_local_host_visible = supports_device_local_host_visible;
        select_surface_formats(gpu, self->gpu.swapchain_info.formats, self->gpu.swapchain_info.formats_count, false,
            &self->gpu.surface_format, &self->gpu.depth_format);

        LOG_INFO("Selecting GPU: %s", properties.deviceName);

        return;
    }

    ASSERT_CUSTOM(false, "Failed to find a valid gpu");
}

fn_internal bool
does_gpu_require_portability(vulkan_gpu_t *gpu)
{
    VkExtensionProperties *extensions = NULL;
    u32 count = 0;

    enumerate_device_extensions(gpu->handle, &extensions, &count);

    FOR_RANGE(u32, i, count)
    {
        VkExtensionProperties *extension = extensions + i;
        if (str_cmp(extension->extensionName, c_portability_extension_name))
        {
            return true;
        }
    }

    return false;
}

fn_internal void
create_device(struct gpu_device_t *self)
{
    //
    // Build a list of unique queues

    #define c_max_queues 4
    u32 queue_indices[c_max_queues] = {
        self->gpu.queues.present, self->gpu.queues.graphics,
        self->gpu.queues.compute, self->gpu.queues.transfer,
    };

    u32 uniques_queues[c_max_queues];
    u32 uniques_queues_count = 0;

    FOR_RANGE(u32, queue_indice, c_max_queues)
    {
        bool found = false;

        FOR_RANGE(u32, unique_indice, uniques_queues_count)
        {
            if (queue_indices[queue_indice] == uniques_queues[unique_indice])
            {
                found = true;
                break;
            }
        }

        // only insert unique queues
        if (!found) uniques_queues[uniques_queues_count++] = queue_indices[queue_indice];
    }

    const f32 queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_infos[c_max_queues];
    FOR_RANGE(u32, i, uniques_queues_count)
    {
        VkDeviceQueueCreateInfo *info = queue_infos + i;
        ZERO_STRUCT(info);

        info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info->queueFamilyIndex = uniques_queues[i];
        info->queueCount       = 1;
        info->flags            = 0;
        info->pNext            = NULL;
        info->pQueuePriorities = &queue_priority;
    }

    //
    // Enable required device features

    // enable Device Address Features (allows for directly address GPU memory)
    VkPhysicalDeviceBufferDeviceAddressFeatures enabled_device_address_features = {};
    enabled_device_address_features.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabled_device_address_features.bufferDeviceAddress              = VK_TRUE;
    enabled_device_address_features.bufferDeviceAddressCaptureReplay = VK_FALSE;
    enabled_device_address_features.bufferDeviceAddressMultiDevice   = VK_FALSE;

    // enable synchronization2
    VkPhysicalDeviceSynchronization2Features enabled_synchronization2_features = {};
    enabled_synchronization2_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    enabled_synchronization2_features.synchronization2 = VK_TRUE;
    enabled_synchronization2_features.pNext            = &enabled_device_address_features;

    // enable timeline semaphores
    VkPhysicalDeviceTimelineSemaphoreFeatures enabled_timeline_semaphore_features = {};
    enabled_timeline_semaphore_features.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    enabled_timeline_semaphore_features.timelineSemaphore = VK_TRUE;
    enabled_timeline_semaphore_features.pNext             = &enabled_synchronization2_features;

    // enable dynamic rendering
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {};
    dynamic_rendering_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;
    dynamic_rendering_features.pNext            = &enabled_timeline_semaphore_features;

    VkPhysicalDeviceFeatures enabled_features = {};

    VkPhysicalDeviceFeatures2 physical_device_features2 = {};
    physical_device_features2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physical_device_features2.features = enabled_features;
    physical_device_features2.pNext    = &dynamic_rendering_features;

    //
    // Build list of extensions

    // if the portability subset extension is present, then need to request it too
    #define c_max_extensions c_device_extensions_count + 1
    const char *extensions[c_max_extensions];
    u32 extensions_count = 0;

    FOR_RANGE(u32, i, c_device_extensions_count)
    {
        extensions[extensions_count++] = c_device_extensions[i];
    }

    if (does_gpu_require_portability(&self->gpu))
    {
        extensions[extensions_count++] = c_portability_extension_name;
    }

    //
    // Create the logical device

    VkDeviceCreateInfo device_ci = {};
    device_ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.flags                   = 0;
    device_ci.queueCreateInfoCount    = uniques_queues_count;
    device_ci.pQueueCreateInfos       = queue_infos;
    device_ci.enabledExtensionCount   = extensions_count;
    device_ci.ppEnabledExtensionNames = extensions;
    device_ci.pEnabledFeatures        = NULL;
    device_ci.pNext                   = &physical_device_features2;

    VK_CHECK_RESULT(vkCreateDevice(self->gpu.handle, &device_ci, NULL, &self->handle),
        "Failed to create the GPU device.");

    load_device_functions(self->handle, extensions, extensions_count);
}

VkSemaphore
create_semaphore(struct gpu_device_t *self)
{
    VkSemaphoreCreateInfo semaphore_ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore result = NULL;
    VK_CHECK_RESULT(vkCreateSemaphore(self->handle, &semaphore_ci, NULL, &result),
        "Failed to create semaphore");

    return result;
}

VkSemaphore
create_timeline_semaphore(struct gpu_device_t *self, u64 initial_value)
{
    VkSemaphoreTypeCreateInfo timeline_ci = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR };
    timeline_ci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
    timeline_ci.initialValue  = initial_value;

    VkSemaphoreCreateInfo semaphore_ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_ci.pNext = &timeline_ci;

    VkSemaphore result = NULL;
    VK_CHECK_RESULT(vkCreateSemaphore(self->handle, &semaphore_ci, NULL, &result),
        "Failed to create timeline semaphore");

    return result;
}

void
destroy_semaphore(struct gpu_device_t *self, VkSemaphore semaphore)
{
    vkDestroySemaphore(self->handle, semaphore, NULL);
}

VkFence
create_fence(struct gpu_device_t *self, bool set_signaled)
{
    VkFenceCreateInfo fence_ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_ci.flags = set_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkFence result = NULL;
    VK_CHECK_RESULT(vkCreateFence(self->handle, &fence_ci, NULL, &result),
        "Failed to create fence");

    return result;
}

void
destroy_fence(struct gpu_device_t *self, VkFence fence)
{
    vkDestroyFence(self->handle, fence, NULL);
}

fn_internal void
create_swapchain(vulkan_swapchain_t *self, struct gpu_device_t *device)
{
    vulkan_gpu_t *gpu = &device->gpu;

    // note: the client has to call on_resize after initially creating the swapchain
    self->cached_width  = fast_max(8, self->cached_width);
    self->cached_height = fast_max(8, self->cached_height);

    // Select the present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; //worst-case fallback if mailbox is not present
    FOR_RANGE(u32, i, gpu->swapchain_info.present_modes_count)
    {
        if (gpu->swapchain_info.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // For the docs on the surface capabilities:
    // > currentExtent is the current width and height of the surface, or the special value (0xFFFFFFFF, 0xFFFFFFFF) indicating
    //   that the surface size will be determined by the extent of a swapchain targeting the surface.
    //
    // We'll use the cached dimensions as the fallback. This will either be set by the device recreation or by onResize()
    VkExtent2D swapchain_extent = { .width = self->cached_width, .height = self->cached_height };
    if (gpu->swapchain_info.capabilities.currentExtent.width != U32_MAX && gpu->swapchain_info.capabilities.currentExtent.height != U32_MAX) {
        swapchain_extent = gpu->swapchain_info.capabilities.currentExtent;
    }

    // Clamp to the value allowed by the GPU.
    VkExtent2D image_min = gpu->swapchain_info.capabilities.minImageExtent;
    VkExtent2D image_max = gpu->swapchain_info.capabilities.maxImageExtent;
    swapchain_extent.width  = CLAMP(swapchain_extent.width,  image_min.width,  image_max.width);
    swapchain_extent.height = CLAMP(swapchain_extent.height, image_min.height, image_max.height);

    u32 image_count = gpu->swapchain_info.capabilities.minImageCount + 1;
    if (gpu->swapchain_info.capabilities.maxImageCount > 0)
    {
        image_count = fast_min(gpu->swapchain_info.capabilities.maxImageCount, image_count);
    }

    ASSERT(image_count > 0); // double check that we aren't about to accidentally allow UINT32_MAX images
    u32 max_images_in_flight = image_count - 1;

    // Create the Swapchain
    //

    VkSwapchainKHR old_handle = self->handle;

    VkSwapchainCreateInfoKHR swapchain_ci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_ci.surface          = device->surface;
    swapchain_ci.minImageCount    = image_count;
    swapchain_ci.imageFormat      = gpu->surface_format.format;
    swapchain_ci.imageColorSpace  = gpu->surface_format.colorSpace;
    swapchain_ci.imageExtent      = swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_ci.preTransform     = gpu->swapchain_info.capabilities.currentTransform;
    swapchain_ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode      = present_mode;
    swapchain_ci.clipped          = VK_TRUE;
    swapchain_ci.oldSwapchain     = old_handle;

    // We expect to have a present and graphics queue.
    u32 present_queue_index  = gpu->queues.present;
    u32 graphics_queue_index = gpu->queues.graphics;

    // let's triple check to make sure these are valid indices
    ASSERT(present_queue_index  != c_invalid_queue_family);
    ASSERT(graphics_queue_index != c_invalid_queue_family);

    u32 queue_family_indices[2] = { present_queue_index, graphics_queue_index };

    if (present_queue_index != graphics_queue_index)
    {
        swapchain_ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        swapchain_ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices   = NULL;
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(device->handle, &swapchain_ci, NULL, &self->handle),
        "Failed to create swapchain");

    // We requested the image count before, now let's query in case the driver didn't like our request
    image_count = 0;
    vkGetSwapchainImagesKHR(device->handle, self->handle, &image_count, NULL);

    // destroy previous image views (if any)
    FOR_RANGE(u32, i, self->images_count)
    {
        vkDestroyImageView(device->handle, self->image_views[i], NULL);
    }

    self->images_count = image_count;

    // fetch the new swapchain images
    vkGetSwapchainImagesKHR(device->handle, self->handle, &self->images_count, self->images);

    // ...and create their views
    FOR_RANGE(u32, i, self->images_count)
    {
        VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image                           = self->images[i];
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = gpu->surface_format.format;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        VK_CHECK_RESULT(vkCreateImageView(device->handle, &view_info, NULL, &self->image_views[i]),
            "Failed to create a swapchain image view.");
    }

    // clean up old swapchain
    if (old_handle)
    {
        vkDestroySwapchainKHR(device->handle, old_handle, NULL);
    }
}

fn_internal void
destroy_swapchain(vulkan_swapchain_t *self, struct gpu_device_t *device)
{
    //note: assume wait idle has been called

    FOR_RANGE(u32, i, self->images_count)
    {
        vkDestroyImageView(device->handle, self->image_views[i], NULL);
    }

    vkDestroySwapchainKHR(device->handle, self->handle, NULL);
}

fn_inline void
swapchain_validate(vulkan_swapchain_t *self)
{
    self->known_generation = self->current_generation;
}

fn_inline bool
swapchain_is_valid(vulkan_swapchain_t *self)
{
    return self->known_generation == self->current_generation;
}

fn_inline void
swapchain_on_resize(vulkan_swapchain_t *self, u32 width, u32 height)
{
    self->cached_width  = width;
    self->cached_height = height;
    swapchain_invalidate(self);
}

fn_inline VkImageCreateInfo
make_image_ci(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent)
{
    VkImageCreateInfo result = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    result.imageType             = VK_IMAGE_TYPE_2D;
    result.format                = format;
    result.extent                = extent;
    result.mipLevels             = 1;                       //TODO: this should be configurable
    result.arrayLayers           = 1;
    result.samples               = VK_SAMPLE_COUNT_1_BIT;   //for MSAA. not using it by default, so default it to 1 sample per pixel.
    result.tiling                = VK_IMAGE_TILING_OPTIMAL; //optimal tiling, so image is stored on the best gpu format
    result.usage                 = usage_flags;
    result.flags                 = 0;
    result.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    result.queueFamilyIndexCount = 0;
    result.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    // Tiling
    //   - If we want to read the image data from cpu, we would need to use tiling LINEAR (simple 2D array)

    return result;
}

fn_inline VkImageViewCreateInfo
make_image_view_ci(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags)
{
    // build an image-view for the depth image to use for rendering
    VkImageViewCreateInfo result = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    result.image                           = image;
    result.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    result.format                          = format;
    result.subresourceRange.aspectMask     = aspect_flags;
    result.subresourceRange.baseMipLevel   = 0;
    result.subresourceRange.levelCount     = 1;
    result.subresourceRange.baseArrayLayer = 0;
    result.subresourceRange.layerCount     = 1;
    return result;
}

fn_internal void
allocate_image_memory(struct gpu_device_t *self,
    VkExtent3D               extent,
    VkFormat                 format,
    VkImageUsageFlags        image_usage,
    VmaMemoryUsage           memory_usage,
    VkMemoryPropertyFlagBits memory_props,
    bool                     mipmapped,
    struct allocated_image_t *out_image)
{
    allocated_image_t result = {};

    // select the aspect flags
    VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
   	if (format == VK_FORMAT_D32_SFLOAT)
    {
  		aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
   	}

    //hardcoding the draw format to 32 bit float
    result.format = format;
    result.dims   = extent;

    VkImageCreateInfo image_ci = make_image_ci(format, image_usage, extent);
    if (mipmapped)
    {
        image_ci.mipLevels = (u32)floorf(log2f((f32)fast_max(extent.width, extent.height))) + 1;
    }

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo image_alloc_info = {};
    image_alloc_info.usage          = memory_usage;
    image_alloc_info.preferredFlags = memory_props;

    //allocate and create the image
    VK_CHECK_RESULT(vmaCreateImage(self->vma_allocator, &image_ci, &image_alloc_info, &result.image, &result.memory, NULL),
        "Failed to create vmaImage");

    //build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo image_view_ci = make_image_view_ci(result.format, result.image, aspect_flag);
    image_view_ci.subresourceRange.levelCount = image_ci.mipLevels;

    VK_CHECK_RESULT(vkCreateImageView(self->handle, &image_view_ci, NULL, &result.view),
        "Failed to create image view");

    *out_image = result;
}

fn_internal void
destroy_image_memory(struct gpu_device_t *self, struct allocated_image_t *image)
{
    vkDestroyImageView(self->handle, image->view, NULL);
    vmaDestroyImage(self->vma_allocator, image->image, image->memory);

    image->image  = NULL;
    image->memory = NULL;
}

fn_export void
gpu_device_create(gpu_device_info_t *device_info, struct gpu_device_t *self, int *device_size)
{
    *device_size = sizeof(struct gpu_device_t);
    if (!self) return;

    load_global_functions();

    // create the vulkan instance
    create_vk_instance(self, device_info);
    setup_debug_messenger(self);
    load_instance_functions(self->instance, c_instance_extensions, c_instance_extensions_count);

    // create the vulkan surface
    create_vulkan_surface(self, device_info->native_surface);

    // select the best GPU
    select_gpu(self);

    // create the logical device
    create_device(self);

    // setup command queues
    vkGetDeviceQueue(self->handle, self->gpu.queues.present,  0, &self->present_queue);
    vkGetDeviceQueue(self->handle, self->gpu.queues.present,  0, &self->graphics_queue);
    vkGetDeviceQueue(self->handle, self->gpu.queues.present,  0, &self->compute_queue);
    vkGetDeviceQueue(self->handle, self->gpu.queues.present,  0, &self->transfer_queue);

    // create the swapchain
    ZERO_STRUCT(&self->swapchain);
    create_swapchain(&self->swapchain, self);

    // create the gpu_contexts
    vulkan_present_context_init(&self->present_context, self);
    vulkan_graphics_context_init(&self->graphics_context, self);

    // initialize the Vulkan Memory Allocator
    const VmaAllocatorCreateFlags vma_flags =
        VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | // app is single threaded at the moment.
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    // need to enable: Found as available and enabled device feature `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`.

    const VmaVulkanFunctions vma_functions =
    {
        .vkGetInstanceProcAddr                   = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2,
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2,
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
#endif
#if VMA_KHR_MAINTENANCE4 || VMA_VULKAN_VERSION >= 1003000
        .vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements,
#endif
    };

    VmaAllocatorCreateInfo allocator_ci = {};
    allocator_ci.physicalDevice   = self->gpu.handle;
    allocator_ci.device           = self->handle;
    allocator_ci.instance         = self->instance;
    allocator_ci.flags            = vma_flags;
    allocator_ci.pVulkanFunctions = &vma_functions;
    vmaCreateAllocator(&allocator_ci, &self->vma_allocator);

    VkImageUsageFlags image_usages =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT      |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    allocate_image_memory(self, swapchain_get_extent(&self->swapchain), self->gpu.surface_format.format,
        image_usages, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, &g_scene);

	self->gpu_buffer_allocator            = create_pool_allocator(500, sizeof(vulkan_buffer_t));
	self->gpu_image_allocator             = create_pool_allocator(500, sizeof(vulkan_image_t));
	self->gpu_descriptor_pool_allocator   = create_pool_allocator(500, sizeof(vulkan_descriptor_pool_t));
	self->paged_descriptor_pool_allocator = create_pool_allocator(500, sizeof(vulkan_paged_descriptor_pool_t));
}

fn_export void
gpu_device_destroy(struct gpu_device_t *self)
{
    vkDeviceWaitIdle(self->handle);

    destroy_image_memory(self, &g_scene);

    vulkan_graphics_context_deinit(&self->graphics_context, self);
    vulkan_present_context_deinit(&self->present_context, self);

    destroy_swapchain(&self->swapchain, self);

    destroy_debug_messenger(self);

    vkDestroyDevice(self->handle, NULL);
    vkDestroySurfaceKHR(self->instance, self->surface, NULL);
    vkDestroyInstance(self->instance, NULL);
}

fn_export void
gpu_device_on_resize(struct gpu_device_t *self, u32 width, u32 height)
{
    swapchain_on_resize(&self->swapchain, width, height);
}

fn_export void
gpu_device_begin_frame(struct gpu_device_t *self)
{
    if (!swapchain_is_valid(&self->swapchain))
    {
        vkDeviceWaitIdle(self->handle);

        self->gpu.swapchain_info = query_swapchain_support_info(self->gpu.handle, self->surface); //@fixme: right now this is only in temp memory
        create_swapchain(&self->swapchain, self);

        swapchain_validate(&self->swapchain);
    }
}

fn_export void
gpu_device_end_frame(struct gpu_device_t *self /* todo: image to be copied into swapchain */)
{
    // submit any pending commands
    // note: example draw code (for now)
    vulkan_graphics_context_begin(&self->graphics_context, self);
    {
        var_persist u64 frame_number = 0;
        frame_number += 1;

        f32 flash = fabs(sinf(frame_number / 120.0f));
        f32 clear_color[4] = { 0.0f, 0.0f, flash, 1.0f };
        vulkan_graphics_context_clear_color_image(&self->graphics_context, &g_scene, clear_color);
    }
    vulkan_graphics_context_submit(&self->graphics_context);

    vulkan_present_context_present(&self->present_context, &self->graphics_context, self, &self->swapchain, &g_scene);
}

fn_internal VkDescriptorPool
paged_descriptor_pool_get_pool(vulkan_paged_descriptor_pool_t *paged_pool)
{
	VkDescriptorPool pool = NULL;

	if (paged_pool->ready_pools_count > 0)
	{
		return paged_pool->ready_pools[paged_pool->ready_pools_count - 1];
	}

	return pool;
}

fn_internal void
paged_descriptor_pool_add_ready_pool(vulkan_paged_descriptor_pool_t *paged_pool, VkDescriptorPool pool)
{
	ASSERT(paged_pool->ready_pools_count + 1 <= 100);
	paged_pool->ready_pools[paged_pool->ready_pools_count++] = pool;
}

fn_internal void
paged_descriptor_pool_add_full_pool(vulkan_paged_descriptor_pool_t *paged_pool, VkDescriptorPool pool)
{
	ASSERT(paged_pool->full_pools_count + 1 <= 100);
	paged_pool->full_pools[paged_pool->full_pools_count++] = pool;
}

fn_internal VkDescriptorType
gpu_descriptor_type_to_vulkan(gpu_descriptor_type_t type)
{
	VkDescriptorType result;

	switch (type)
	{
		case DESCRIPTOR_TYPE_SAMPLER:          result = VK_DESCRIPTOR_TYPE_SAMPLER;                break;
		case DESCRIPTOR_TYPE_COMBINED_SAMPLER: result = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:    result = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;          break;
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:    result = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;          break;
		case DESCRIPTOR_TYPE_UNIFORM_BUFFER:   result = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;         break; 
		case DESCRIPTOR_TYPE_STORAGE_BUFFER:   result = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;         break; 
		default: ASSERT(false);
	}

	return result;
}

fn_internal VkDescriptorPool
vulkan_device_create_descriptor_pool(vulkan_device_t *self, u32 max_sets, gpu_descriptor_pool_flags_t flags, gpu_descriptor_pool_size_ratio_t *ratios, u32 ratio_count)
{
	VkDescriptorPoolSize pool_sizes[DESCRIPTOR_TYPE_COUNT];
	
	FOR_RANGE(u32, i, ratio_count)
	{
		pool_sizes[i].type            = gpu_descriptor_type_to_vulkan(ratios[i].type);
		pool_sizes[i].descriptorCount = (u32)(max_sets * ratios[i].ratio);
	}

	VkDescriptorPoolCreateFlags vk_flags = (flags & DESCRIPTOR_POOL_FLAG_ALLOW_FREE) ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;
	
	VkDescriptorPoolCreateInfo pool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_ci.flags         = vk_flags;
	pool_ci.maxSets       = max_sets;
	pool_ci.poolSizeCount = ratio_count;
	pool_ci.pPoolSizes    = pool_sizes;

	VkDescriptorPool result = NULL;
	VK_CHECK_RESULT(vkCreateDescriptorPool(self->handle, &pool_ci, NULL, &result),
		"Failed to create VkDescriptorPool");

	return result;
}

fn_internal void
vulkan_device_destroy_descriptor_pool(vulkan_device_t *self, VkDescriptorPool pool)
{
	vkDestroyDescriptorPool(self->handle, pool, NULL);
}

fn_export struct gpu_descriptor_pool_t* gpu_device_create_descriptor_pool(
	struct gpu_device_t              *self, 
	gpu_descriptor_pool_flags_t        flags, 
	gpu_descriptor_pool_size_ratio_t *ratios,
	u32                               ratio_count,
	u32                               max_descriptor_sets)
{
	struct gpu_descriptor_pool_t *result = CT_ALLOC_STRUCT(self->gpu_descriptor_pool_allocator, struct gpu_descriptor_pool_t);
	result->flags  = flags;
	result->handle = vulkan_device_create_descriptor_pool(self, max_descriptor_sets, flags, ratios, ratio_count);
	return result;
}

fn_export void 
gpu_device_destroy_descriptor_pool(struct gpu_device_t *self, struct gpu_descriptor_pool_t *pool)
{
	vulkan_device_destroy_descriptor_pool(self, pool->handle);
	ZERO_STRUCT(pool);

	allocator_free(self->gpu_descriptor_pool_allocator, pool);
}

fn_export struct gpu_paged_descriptor_pool_t*
gpu_device_create_paged_descriptor_pool(struct gpu_device_t *self, gpu_descriptor_pool_size_ratio_t *ratios, u32 ratios_count, u32 sets_per_pool)
{
	ASSERT(ratios_count < DESCRIPTOR_TYPE_COUNT);

	vulkan_paged_descriptor_pool_t *result = CT_ALLOC_STRUCT(self->paged_descriptor_pool_allocator, vulkan_paged_descriptor_pool_t);
	result->descriptors_per_pool = sets_per_pool;
	result->ratios_count         = ratios_count;
	result->full_pools_count     = 0;
	result->ready_pools_count    = 1;
	mem_copy(result->ratios, ratios, ratios_count * sizeof(gpu_descriptor_pool_size_ratio_t));
	
	result->ready_pools[0] = vulkan_device_create_descriptor_pool(self, sets_per_pool, DESCRIPTOR_POOL_FLAG_NONE, result->ratios, result->ratios_count);

	return result;
}

fn_export void 
gpu_device_destroy_paged_descriptor_pool(struct gpu_device_t *self, struct gpu_paged_descriptor_pool_t *descriptor_pool)
{
	FOR_RANGE(u32, i, descriptor_pool->full_pools_count)
	{
		vulkan_device_destroy_descriptor_pool(self, descriptor_pool->full_pools[i]);
	}

	FOR_RANGE(u32, i, descriptor_pool->ready_pools_count)
	{
		vulkan_device_destroy_descriptor_pool(self, descriptor_pool->ready_pools[i]);
	}

	ZERO_STRUCT(descriptor_pool);
	allocator_free(self->paged_descriptor_pool_allocator, descriptor_pool);
}

fn_export void 
gpu_device_reset_descriptor_pool(struct gpu_device_t *self, struct gpu_descriptor_pool_t *pool)
{
	VK_CHECK_RESULT(vkResetDescriptorPool(self->handle, pool->handle, 0), "Failed to reset descriptor pool");
}

fn_export struct gpu_descriptor_set_t* 
gpu_device_allocate_descriptor_sets(
	struct gpu_device_t                  *self,
	struct gpu_descriptor_layout_t       *gpu_layout,
	struct gpu_paged_descriptor_pool_t         *descriptor_pool)
{
	VkDescriptorSetLayout vk_layout = (VkDescriptorSetLayout)gpu_layout;

	VkDescriptorPool pool = paged_descriptor_pool_get_pool(descriptor_pool);

	// TODO:
	UNIMPLEMENTED;

	return NULL;
}

fn_internal VkShaderStageFlagBits
gpu_stage_flags_to_vulkan(gpu_shader_stage_flags_t flags)
{
	VkShaderStageFlagBits result = 0;

	if (flags & GPU_SHADER_STAGE_ALL)
	{
		result |= VK_SHADER_STAGE_ALL;
	}

	if (flags & GPU_SHADER_STAGE_ALL_GRAPHICS)
	{
		result |= VK_SHADER_STAGE_ALL_GRAPHICS;
	}

	if (flags & GPU_SHADER_STAGE_VERTEX)
	{
		result |= VK_SHADER_STAGE_VERTEX_BIT;
	}

	if (flags & GPU_SHADER_STAGE_FRAGMENT)
	{
		result |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	if (flags & GPU_SHADER_STAGE_COMPUTE)
	{
		result |= VK_SHADER_STAGE_COMPUTE_BIT;
	}

	return result;
}

fn_internal VkDescriptorSetLayoutCreateFlags
gpu_descriptor_layout_type_to_vulkan(gpu_descriptor_layout_flags_t type)
{
	VkDescriptorSetLayoutCreateFlags result = 0;

	if (type & DESCRIPTOR_LAYOUT_FLAG_UPDATE_AFTER_BIND)
	{
		result = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	}
	if (type & DESCRIPTOR_LAYOUT_FLAG_PUSH_DESCRIPTOR)
	{
		result = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
	}
	if (type & DESCRIPTOR_LAYOUT_FLAG_DESCRIPTOR_BUFFER)
	{
		result = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
	}
		
	return result;
}

fn_export struct gpu_descriptor_layout_t* 
gpu_device_build_descriptor_layout(struct gpu_device_t *self, gpu_descriptor_layout_flags_t type, gpu_shader_stage_flags_t flags,
	gpu_descriptor_binding_t *bindings, u32 bindings_count)
{
	ASSERT(bindings_count <= MAX_DESCRIPTOR_BINDINGS);

	VkDescriptorSetLayoutBinding vk_layout_bindings[MAX_DESCRIPTOR_BINDINGS];
	ZERO_ARRAY(vk_layout_bindings, ARRAY_COUNT(vk_layout_bindings));

	FOR_RANGE(u32, i, bindings_count)
	{
		vk_layout_bindings[i].binding         = bindings[i].binding;
		vk_layout_bindings[i].descriptorType  = gpu_descriptor_type_to_vulkan(bindings[i].type);
		vk_layout_bindings[i].descriptorCount = 1;
		vk_layout_bindings[i].stageFlags      = gpu_stage_flags_to_vulkan(flags);
	}

	VkDescriptorSetLayoutCreateInfo layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layout_ci.flags        = gpu_descriptor_layout_type_to_vulkan(type);
	layout_ci.bindingCount = bindings_count;
	layout_ci.pBindings    = vk_layout_bindings;

	VkDescriptorSetLayout result = VK_NULL_HANDLE;
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(self->handle, &layout_ci, NULL, &result),
		"Failed to create descriptor layout");

	return (struct gpu_descriptor_layout_t*)result;
}

fn_export void 
gpu_device_destroy_descriptor_layout(struct gpu_device_t *self, struct gpu_descriptor_layout_t *layout)
{
	vkDestroyDescriptorSetLayout(self->handle, (VkDescriptorSetLayout)layout, NULL);
}