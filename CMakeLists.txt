cmake_minimum_required(VERSION 3.3)
project(render_engine)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

pkg_search_module(FREETYPE REQUIRED freetype2)

find_library(FOUNDATION_LIBRARY Foundation)

include_directories("gl3w/include")

include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_STATIC_LIBRARY_DIRS})

include_directories(${FREETYPE_INCLUDE_DIRS})
link_directories(${FREETYPE_STATIC_LIBRARY_DIRS})

set(SOURCE_FILES main.cpp gl3w/src/gl3w.c)
add_executable(render_engine ${SOURCE_FILES})
target_link_libraries(render_engine ${GLFW_LIBRARIES} ${FREETYPE_LIBRARIES} ${FOUNDATION_LIBRARY})