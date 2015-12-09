cmake_minimum_required(VERSION 3.3)
project(render_engine C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

pkg_search_module(FREETYPE REQUIRED freetype2)

find_library(FOUNDATION_LIBRARY Foundation)

find_path(STDIO_INCLUDE_PATH stdio.h)

include_directories("${STDIO_INCLUDE_PATH}/dummy/../")
include_directories(".")
include_directories("gl3w/include")

include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_STATIC_LIBRARY_DIRS})

include_directories(${FREETYPE_INCLUDE_DIRS})
link_directories(${FREETYPE_STATIC_LIBRARY_DIRS})

set(SOURCE_FILES main.cpp gl3w/src/gl3w.c)
add_executable(render_engine ${SOURCE_FILES})
target_link_libraries(render_engine ${GLFW_LIBRARIES} ${FREETYPE_LIBRARIES} ${FOUNDATION_LIBRARY})

add_custom_command(TARGET render_engine PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:render_engine>/fonts)

add_custom_command(TARGET render_engine PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:render_engine>/images)
