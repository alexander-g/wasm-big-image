cmake_minimum_required(VERSION 3.19)
project(example0 LANGUAGES C CXX)


set(tiff-docs OFF)
set(tiff-tools OFF)
set(tiff-tests OFF)
add_subdirectory(./libtiff EXCLUDE_FROM_ALL)
include_directories(${LIBTIFF_INCLUDE_DIRS})

#add_library(example0 STATIC example0.c)
#target_link_libraries(example0 PRIVATE tiff)
#set_target_properties(example0 PROPERTIES CXX_STANDARD 17)


add_library(cb_cookie STATIC src/cb_cookiefile.c)

add_executable(tests tests/tests.c)
target_link_libraries(tests PRIVATE cb_cookie)
set_target_properties(tests PROPERTIES CXX_STANDARD 17)
