cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MODULE_NAME ct_engine)

SET(ALL_FILES
  # header files
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer.h

  # source files
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer.c
)

if (WIN32 OR UNIX)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/render_backend_vk)
else ()
    message( FATAL_ERROR "No renderer backend for platform. CMake will exit." )
endif()

target_sources(${MODULE_NAME} PRIVATE ${ALL_FILES})
