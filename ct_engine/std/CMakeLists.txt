cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(FDN_WIN32_FILES
  # header files
 ${CMAKE_CURRENT_SOURCE_DIR}/win32/win32.h

  # source files
 ${CMAKE_CURRENT_SOURCE_DIR}/win32/win32.c
 ${CMAKE_CURRENT_SOURCE_DIR}/win32/win32_console.c
)

SET(FDN_UNIX_FILES
  # header files

  # source files
  ${CMAKE_CURRENT_SOURCE_DIR}/unix/unix_dll.c
)

SET(FDN_CORE_FILES
  # header files
  ${CMAKE_CURRENT_SOURCE_DIR}/types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/str.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dll.h
  ${CMAKE_CURRENT_SOURCE_DIR}/log.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cassert.h

  # source files
  ${CMAKE_CURRENT_SOURCE_DIR}/mem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/str.c
  ${CMAKE_CURRENT_SOURCE_DIR}/log.c
)

if (WIN32)
    set(FDN_FILES ${FDN_CORE_FILES} ${FDN_WIN32_FILES})
elseif (UNIX)
    set(FDN_FILES ${FDN_CORE_FILES} ${FDN_UNIX_FILES})
else ()
    set(FDN_FILES ${FDN_CORE_FILES})
endif()

target_sources(ct_engine PRIVATE ${FDN_FILES})
