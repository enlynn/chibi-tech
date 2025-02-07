cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MODULE_NAME ct_engine)

SET(WIN32_FILES
  # header files

  # source files
 ${CMAKE_CURRENT_SOURCE_DIR}/win32/window_win32.c
)

SET(X11_FILES
  # header files

  # source files
 ${CMAKE_CURRENT_SOURCE_DIR}/linux/window_x11.c
)

SET(CORE_FILES
  # header files
 ${CMAKE_CURRENT_SOURCE_DIR}/window.h

  # source files
)

if (WIN32)
    set(ALL_FILES ${CORE_FILES} ${WIN32_FILES})
elseif (UNIX)
    set(ALL_FILES ${CORE_FILES} ${X11_FILES})
else ()
    set(ALL_FILES ${CORE_FILES})
endif()

target_sources(${MODULE_NAME} PRIVATE ${ALL_FILES})
