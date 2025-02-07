cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(SOURCE_FILES
  # header files
  ${CMAKE_CURRENT_SOURCE_DIR}/array.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hash.h

  # source files
  ${CMAKE_CURRENT_SOURCE_DIR}/array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hash.c
)

set(ALL_FILES ${SOURCE_FILES})
target_sources(ct_engine PRIVATE ${ALL_FILES})
