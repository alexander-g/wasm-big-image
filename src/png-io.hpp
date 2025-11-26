#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>

#include "./util.hpp"


enum Error_PNG_IO { 
    //OK = 0,

    PNG_INIT_LIB_FAILED      = -301, 
    INVALID_CHANNELS         = -350,
    
    //NOT_IMPLEMENTED = -999,
};

extern "C" {

    
int png_get_size(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    // outputs
    int32_t*    width,
    int32_t*    height,
    // return code (because of wasm issues)
    int*        rc
);


/** Read a sub-image patch without reading the full image, and resize it to
   dst_width x dst_height (currently only nearest neighbor interpolation). */
int png_read_patch(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    int         src_x,
    int         src_y,
    int         src_width,
    int         src_height,
    int         dst_width,
    int         dst_height,
    void*       dst_buffer,
    size_t      dst_buffersize,
    // return code (because of wasm issues)
    int*        rc
);



} // extern "C"





std::expected<Buffer_p, int> png_compress_image(
    const uint8_t* rgba, 
    int32_t width, 
    int32_t height,
    // 1: binary, 3: rgb
    int channels
);



