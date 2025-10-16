cmake_minimum_required(VERSION 3.20)
project(bigimage LANGUAGES C CXX)

include(ExternalProject)



ExternalProject_Add(
    libjpeg-turbo
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libjpeg-turbo
    CMAKE_COMMAND ${CMAKE_COMMAND}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libjpeg-turbo-install
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DBUILD_SHARED_LIBS=OFF
        -DWITH_SIMD=TRUE
        #emscripten    
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    UPDATE_COMMAND ""  # No git pull etc
    INSTALL_DIR ${CMAKE_BINARY_DIR}/libjpeg-turbo-install
)



add_library(JPEG::JPEG STATIC IMPORTED)
set_target_properties(JPEG::JPEG PROPERTIES
  IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libjpeg.a"
  IMPORTED_LOCATION_RELEASE "${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libjpeg.a"
  IMPORTED_LOCATION_DEBUG   "${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libjpegd.a"
)

set(JPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/libjpeg-turbo-install/include)
set(JPEG_LIBRARY ${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libturbojpeg.a)
include_directories(${JPEG_INCLUDE_DIR})



set(tiff-docs OFF)
set(tiff-tools OFF)
set(tiff-tests OFF)
set(jpeg ON)
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
    COMMAND emcc 
      libwasm_big_image.a 
      libtiff/libtiff/libtiff.a  
      libjpeg-turbo-install/lib/libjpeg.a
        -sEXPORT_ES6 
        -sMODULARIZE=1 
        -sSINGLE_FILE=1 
        -sEXPORT_NAME="bigimage" 
        -sEXPORTED_RUNTIME_METHODS=addFunction,Asyncify
        -sEXPORTED_FUNCTIONS=_tiff_get_size,_tiff_read,_tiff_read_patch,_malloc,_free
        -sENVIRONMENT=web 
        -sALLOW_TABLE_GROWTH 
        -sALLOW_MEMORY_GROWTH=1
        -sWASM_BIGINT 
        -sASYNCIFY 
        -O3
        -o big-image.js
    #COMMAND 
    #    ../deno.sh task bundle
    DEPENDS wasm_big_image
  )
endif()

