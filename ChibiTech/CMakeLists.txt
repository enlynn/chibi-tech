cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all of the source files
file(GLOB UTIL_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Util/*.cpp")
file(GLOB UTIL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Util/*.h")

file(GLOB MATH_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Math/*.cpp")
file(GLOB MATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Math/*.h")

file(GLOB SYSTEMS_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Systems/*.cpp")
file(GLOB SYSTEMS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Systems/*.h")

file(GLOB PLATFORM_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/*.cpp")
file(GLOB PLATFORM_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/*.h")

IF (WIN32)
    file(GLOB PLATFORM_EXTRA_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/Win32/*.cpp")
    file(GLOB PLATFORM_EXTRA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/Win32/*.h")
endif (WIN32)

if (UNIX)
    file(GLOB PLATFORM_EXTRA_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/Nix/*.cpp")
    file(GLOB PLATFORM_EXTRA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/Nix/*.h")
endif (UNIX)

file(GLOB GPU_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Gpu/*.cpp")
file(GLOB GPU_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Gpu/*.h")

set(ALL_SOURCES ${UTIL_SRC} ${PLATFORM_SRC} ${PLATFORM_EXTRA_SRC} ${GPU_SRC} ${MATH_SRC} ${SYSTEMS_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/Source/EngineEntry.cpp")
set(ALL_HEADERS ${UTIL_HEADERS} ${PLATFORM_HEADERS} ${PLATFORM_EXTRA_HEADERS} ${GPU_HEADERS} ${MATH_HEADERS} ${SYSTEMS_HEADERS}
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Game.h"
)

set(ALL_FILES ${ALL_SOURCES} ${ALL_HEADERS})

# TODO: figure out precompiled headers with msvc
#ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "${CMAKE_SOURCE_DIR}/ChibiTech/stdafx.cpp" ALL_FILES)

# IDEs should put the headers in a nice place
source_group(
        TREE "${PROJECT_SOURCE_DIR}/ChibiTech"
        PREFIX "Chibi Tech Files"
        FILES ${ALL_HEADERS})

# Setup the library
add_library(chibi-tech ${ALL_FILES})
target_compile_features(chibi-tech PUBLIC cxx_std_20)

target_include_directories(chibi-tech
        PRIVATE
        "." "Source/" "../Vendor" "../Vendor/AgilitySDK/" "../Vendor/DXC/"
)

IF (WIN32)
    target_link_directories(chibi-tech PRIVATE
            "../Vendor/AgilitySDK/bin/x64"
            "../Vendor/DXC/Bin/x64"
    )

    # GLFW_EXPOSE_NATIVE_WIN32 is required to fetch the Win32 Window handle when creating a SwapChain
    add_compile_definitions(chibi-tech PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX CT_PLATFORM_WINDOWS)
    target_link_libraries(chibi-tech PRIVATE
            Winmm.lib
            dxgi.lib
            d3d12.lib
            dxguid.lib
            d3dcompiler.lib
            dxcompiler.lib
    )
endif (WIN32)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(chibi-tech PRIVATE CT_DEBUG)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(chibi-tech PRIVATE CT_DIST)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(chibi-tech PRIVATE CT_RELEASE)
endif()

target_link_libraries(chibi-tech PRIVATE glfw)
