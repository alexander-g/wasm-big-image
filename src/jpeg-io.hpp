#include <cstdint>



extern "C" {


enum Error_JPEG_IO { 
    //OK = 0,

    JPEG_READ_HEADER_FAILED      = -1, 
    JPEG_START_DECOMPRESS_FAILED = -2,
    JPEG_UNSUPPORTED_N_CHANNELS  = -3,
    
    //NOT_IMPLEMENTED = -999,
};


int jpeg_get_size(
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
int jpeg_read_patch(
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


} // end extern "C"
