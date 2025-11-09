cmake_minimum_required(VERSION 3.20)
project(bigimage LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_BUILD_TYPE Release)



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
        -DWITH_TESTS=OFF
        -DWITH_TOOLS=OFF
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


set(ZLIB_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
add_subdirectory(${ZLIB_DIRECTORY} EXCLUDE_FROM_ALL)
GET_DIRECTORY_PROPERTY(
	ZLIB_BUILD_DIRECTORY
	DIRECTORY
	${ZLIB_DIRECTORY}
	DEFINITION
	CMAKE_CURRENT_BINARY_DIR
)
set(ZLIB_INCLUDE_DIR ${ZLIB_DIRECTORY} ${ZLIB_BUILD_DIRECTORY})
set(ZLIB_LIBRARY zlibstatic)
include_directories(
	${ZLIB_DIRECTORY}
	${ZLIB_BUILD_DIRECTORY}
)



set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
add_subdirectory(./libpng EXCLUDE_FROM_ALL)


add_library(wasm_big_image STATIC 
    src/common.cpp
    src/cb_cookiefile.c
    src/tiff-io.c
    src/jpeg-io.cpp
    src/png-io.cpp
    src/util.c
)
target_include_directories(wasm_big_image PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/libpng
  ${CMAKE_CURRENT_BINARY_DIR}/zlib
)
target_link_libraries(wasm_big_image PRIVATE tiff)
target_link_libraries(wasm_big_image PRIVATE png_static)
target_link_libraries(wasm_big_image PRIVATE zlibstatic)
target_link_libraries(wasm_big_image PRIVATE ${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libturbojpeg.a)
target_compile_options(wasm_big_image PRIVATE -fexceptions)
target_compile_options(wasm_big_image PRIVATE -O3)


if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "emscripten" OR DEFINED ENV{EMSCRIPTEN})
  set(RUNNING_EMSCRIPTEN TRUE)
else()
  set(RUNNING_EMSCRIPTEN FALSE)
endif()

if(NOT RUNNING_EMSCRIPTEN)
add_executable(tests tests/tests.cpp tests/util.cpp)
target_link_libraries(tests PRIVATE wasm_big_image)
endif()




if(RUNNING_EMSCRIPTEN)
  target_compile_options(wasm_big_image PRIVATE -msimd128)
  target_compile_options(tiff PRIVATE -msimd128)
  target_compile_options(png_static PRIVATE -msimd128)
  target_compile_options(zlibstatic PRIVATE -msimd128)
  # TODO:
  #target_compile_options(${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libturbojpeg.a PRIVATE -msimd128)

  add_custom_target(wasm-js ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running emcc"
    BYPRODUCTS ${CMAKE_BINARY_DIR}/big-image.js
    COMMAND emcc 
      libwasm_big_image.a 
      libtiff/libtiff/libtiff.a  
      libjpeg-turbo-install/lib/libjpeg.a
      libjpeg-turbo-install/lib/libturbojpeg.a
      libpng/libpng.a
      zlib/libz.a
        -sEXPORT_ES6 
        -sMODULARIZE=1 
        -sSINGLE_FILE=1 
        -sEXPORT_NAME="bigimage" 
        -sEXPORTED_RUNTIME_METHODS=addFunction,Asyncify
        -sEXPORTED_FUNCTIONS=_image_get_size,_image_read_patch,_image_read_patch_and_encode,_free_output_buffer,_malloc,_free
        -sENVIRONMENT=web 
        -sALLOW_TABLE_GROWTH 
        -sALLOW_MEMORY_GROWTH=1
        -sWASM_BIGINT 
        -fexceptions
        -sASYNCIFY 
        -O3
        -o big-image.js
    #COMMAND 
    #    ../deno.sh task bundle
    DEPENDS wasm_big_image
  )
endif()

