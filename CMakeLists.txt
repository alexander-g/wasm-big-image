cmake_minimum_required(VERSION 3.19)
project(bigimage LANGUAGES C CXX)


set(tiff-docs OFF)
set(tiff-tools OFF)
set(tiff-tests OFF)
add_subdirectory(./libtiff EXCLUDE_FROM_ALL)
include_directories(${LIBTIFF_INCLUDE_DIRS})



add_library(wasm_big_image STATIC 
    src/cb_cookiefile.c
    src/tiff-io.c
)
target_link_libraries(wasm_big_image PRIVATE tiff)

add_executable(tests tests/tests.c)
target_link_libraries(tests PRIVATE wasm_big_image)
set_target_properties(tests PROPERTIES CXX_STANDARD 17)




if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "emscripten" OR DEFINED ENV{EMSCRIPTEN})
  set(RUNNING_EMSCRIPTEN TRUE)
else()
  set(RUNNING_EMSCRIPTEN FALSE)
endif()

if(RUNNING_EMSCRIPTEN)
  add_custom_target(wasm-js ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running emcc"
    BYPRODUCTS ${CMAKE_BINARY_DIR}/big-image.js
    COMMAND emcc libwasm_big_image.a libtiff/libtiff/libtiff.a 
        -sEXPORT_ES6 
        -sMODULARIZE=1 
        -sSINGLE_FILE=1 
        -sEXPORT_NAME="bigimage" 
        -sEXPORTED_RUNTIME_METHODS=addFunction,Asyncify
        -sEXPORTED_FUNCTIONS=_tiff_get_size,_tiff_read,_malloc,_free
        -sENVIRONMENT=web 
        -sALLOW_TABLE_GROWTH 
        -sWASM_BIGINT 
        -sASYNCIFY 
        -O3
        -o big-image.js
    #COMMAND 
    #    ../deno.sh task bundle
    DEPENDS wasm_big_image
  )
endif()

