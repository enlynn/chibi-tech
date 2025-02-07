#include "vk.h"

#include <std/types.h>
#include <std/str.h>
#include <std/dll.h>

#define VK_EXPORTED_FUNCTION(fun)                                 PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                             PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                           PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun)                             PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)   PFN_##fun fun;
#include "vk_functions.inl"

#if CT_PLATFORM_LINUX
const char *c_vk_dll_name0 = "libvulkan.so";
const char *c_vk_dll_name1 = "libvulkan.so.1";
#elif CT_PLATFORM_WIN32
const char *c_vk_dll_name0 = "vulkan.dll";
const char *c_vk_dll_name1 = "vulkan-1.dll";
#else
#  error Vulkan loader not supported on platform
#endif

var_global os_dll_t g_vk_dll = {};

void
load_global_functions()
{
    g_vk_dll = dll_load(c_vk_dll_name0);
    if (!dll_is_loaded(&g_vk_dll))
    {
        g_vk_dll = dll_load(c_vk_dll_name1);
        ASSERT(dll_is_loaded(&g_vk_dll));
    }

#define VK_EXPORTED_FUNCTION(fun)                                  \
        fun = (PFN_##fun)dll_get_fn(&g_vk_dll, #fun);              \
        ASSERT(fun != NULL && "Could not load exported function.");

#define VK_GLOBAL_LEVEL_FUNCTION(fun)                             \
        fun = (PFN_##fun)vkGetInstanceProcAddr(NULL, #fun);       \
        ASSERT(fun != NULL && "Could not load global function.");

    #include "vk_functions.inl"
}

void
load_instance_functions(VkInstance instance, const char** extensions, size_t extenion_count)
{
    #define VK_INSTANCE_LEVEL_FUNCTION(fun)                          \
        fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun);      \
        ASSERT(fun != NULL && "Could not load instance function.");

    #define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)                 \
        for (size_t i = 0; i < extenion_count; ++i) {                                 \
            if (str_cmp(extensions[i], extension)) {                                  \
                fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun);               \
                ASSERT(fun != NULL && "Could not load instance extension function."); \
            }                                                                         \
        }

    #include "vk_functions.inl"
}

void
load_device_functions(VkDevice device, const char** extensions, size_t extenion_count)
{
    #define VK_DEVICE_LEVEL_FUNCTION(fun)                          \
        fun = (PFN_##fun)vkGetDeviceProcAddr(device, #fun);        \
        ASSERT(fun != NULL && "Could not load instance function.");

    #define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)                   \
        for (size_t i = 0; i < extenion_count; ++i) {                                 \
            if (str_cmp(extensions[i], extension)) {                                  \
                fun = (PFN_##fun)vkGetDeviceProcAddr(device, #fun);                   \
                ASSERT(fun != NULL && "Could not load instance extension function."); \
            }                                                                         \
        }

    #include "vk_functions.inl"
}
