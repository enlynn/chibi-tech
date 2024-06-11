cmake_minimum_required(VERSION 3.28)
project(
        Chibi-Tech
        VERSION 0.1
        DESCRIPTION "A little graphics engine using DX12"
        LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)

# Let's ensure -std=c++xx instead of -std=g++xx
set(CMAKE_CXX_EXTENSIONS OFF)

# Let's nicely support folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Dependency Libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

# Fetch GLFW
include(Cmake/FetchGLFW.cmake)
FetchGLFW()

# Allow for samples/engine to easily handle PCH files
include(Cmake/SetPCH.cmake)

# Engine Static Library
add_subdirectory(ChibiTech)

# Sample Projects
add_subdirectory(Samples)