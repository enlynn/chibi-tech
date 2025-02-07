cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MODULE_NAME ct_engine)

SET(ALL_FILES
  # header files
  ${CMAKE_CURRENT_SOURCE_DIR}/vk.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_present_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_graphics_context.h

  # source files
  ${CMAKE_CURRENT_SOURCE_DIR}/vk.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_device.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_command_buffer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_present_context.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_graphics_context.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vma_inc.cpp
)

if (WIN32)
    target_compile_definitions(${MODULE_NAME} PRIVATE
        VK_NO_PROTOTYPES
        VK_USE_PLATFORM_WIN32_KHR
    )
elseif (UNIX)
    target_compile_definitions(${MODULE_NAME} PRIVATE
        VK_NO_PROTOTYPES
        VK_USE_PLATFORM_XLIB_KHR
    )
else ()
    message( FATAL_ERROR "No renderer backend for platform. CMake will exit." )
endif()

target_sources(${MODULE_NAME} PRIVATE ${ALL_FILES})

#-Wnullability-completeness disabled for vma
target_compile_options(${MODULE_NAME} PRIVATE -Wno-nullability-completeness)
