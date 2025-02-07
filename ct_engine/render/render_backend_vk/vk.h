#ifndef _VK_H_
#define _VK_H_

#ifndef VK_NO_PROTOTYPES
#  define VK_NO_PROTOTYPES
#endif

#include <std/types.h>
#if CT_PLATFORM_WIN32
#  include <std/win32/win32.h>
#endif

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#define VK_EXPORTED_FUNCTION(fun)                                 extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                             extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                           extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun)                             extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION(fun, extension)   extern PFN_##fun fun;
#include "vk_functions.inl"

#include <std/cassert.h>
#define VK_CHECK_RESULT(f, msg)                   \
    do {                                          \
        VkResult res = (f);                       \
        ASSERT_CUSTOM((res) == VK_SUCCESS, msg);  \
    } while(0)

void load_global_functions();
void load_instance_functions(VkInstance tInstance, const char** extensions, size_t extenion_count);
void load_device_functions(VkDevice Device, const char** extensions, size_t extenion_count);

#endif //_VK_H_
