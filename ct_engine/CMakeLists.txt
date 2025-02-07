cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MODULE_NAME ct_engine)

set(SOURCES
    # header files
    ${CMAKE_CURRENT_SOURCE_DIR}/ct_engine.h

    # source files
    ${CMAKE_CURRENT_SOURCE_DIR}/ct_engine.c
)

add_library(${MODULE_NAME} SHARED ${SOURCES})
set_property(TARGET ${MODULE_NAME} PROPERTY C_STANDARD 23)

target_include_directories(${MODULE_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/ct_engine
        ${PROJECT_SOURCE_DIR}/vendor
)

target_compile_definitions(${MODULE_NAME} PRIVATE CT_EXPORT)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${MODULE_NAME} PRIVATE CT_DEBUG)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${MODULE_NAME} PRIVATE CT_DIST)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(${MODULE_NAME} PRIVATE CT_RELEASE)
endif()

if (UNIX)
    find_package(X11 REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE X11)
endif ()

# Engine Modules
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/std)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/containers)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/window)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/render)
