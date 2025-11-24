#include <cstdint>
#include <vector>
#include <stdexcept>
#include <unordered_map>


#include "./jpeg-io.hpp"
#include "./tiff-io.hpp"

#include "./png-io.hpp"
#include "./util.h"



int image_get_size(
    uint32_t    filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    // outputs
    int32_t*    width,
    int32_t*    height,
    // return code (because of wasm issues)
    int*        returncode
){
    if(returncode != NULL) *returncode = UNEXPECTED;

    int rc;
    rc = jpeg_get_size(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        width, 
        height, 
        returncode
    );
    if(rc == OK)
        return OK;
    
    
    const auto expect_tiff = TIFF_Handle::create(
        filesize, 
        (read_file_callback_ptr_t)read_file_callback_p, 
        read_file_handle
    );
    if(expect_tiff) {
        const std::shared_ptr<TIFF_Handle> tiff = expect_tiff.value();
        *width  = (int32_t)tiff->width;
        *height = (int32_t)tiff->height;
        if(returncode != NULL) *returncode = OK;
        return OK;
    }
    

    rc = png_get_size(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        width, 
        height, 
        returncode
    );
    if(rc == OK)
        return OK;

    if(returncode != NULL) *returncode = NOT_IMPLEMENTED;
    return NOT_IMPLEMENTED;
}


/** Read a sub-image patch without reading the full image, and resize it to
   dst_width x dst_height (currently only nearest neighbor interpolation). */
int image_read_patch(
    uint32_t    filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    int         src_x,
    int         src_y,
    int         src_width,
    int         src_height,
    int         dst_width,
    int         dst_height,
    void*       dst_buffer,
    uint32_t    dst_buffersize,
    // return code (because of wasm issues)
    int*        returncode
) {
    if(returncode != NULL) *returncode = UNEXPECTED;

    int rc;
    rc = jpeg_read_patch(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        src_x, 
        src_y, 
        src_width,
        src_height,
        dst_width,
        dst_height,
        dst_buffer,
        dst_buffersize,
        returncode
    );
    if(rc == OK)
        return OK;
    
    rc = tiff_read_patch(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        src_x, 
        src_y, 
        src_width,
        src_height,
        dst_width,
        dst_height,
        dst_buffer,
        dst_buffersize,
        returncode
    );
    if(rc == OK)
        return OK;

    rc = png_read_patch(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        src_x, 
        src_y, 
        src_width,
        src_height,
        dst_width,
        dst_height,
        dst_buffer,
        dst_buffersize,
        returncode
    );
    if(rc == OK)
        return OK;

    if(returncode != NULL) *returncode = UNKNOWN_FORMAT;
    return UNKNOWN_FORMAT;
}



std::unordered_map<uint8_t*, Buffer_p> _buffers;


/** Read a sub-image patch without reading the full image, and resize it to
   dst_width x dst_height (currently only nearest neighbor interpolation),
   then return it re-encoded as jpeg or png. */
int image_read_patch_and_encode(
    uint32_t    filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    int         src_x,
    int         src_y,
    int         src_width,
    int         src_height,
    int         dst_width,
    int         dst_height,
    // if lossless, encode as png, otherwise jpeg
    bool        lossless,
    // output: encoded data
    uint8_t**   output_buffer,
    uint64_t*   output_size,
    // return code (because of wasm issues)
    int*        returncode
) {
    if(returncode != NULL) *returncode = UNEXPECTED;

    const int dst_buffersize = dst_width * dst_height * 4;
    std::vector<uint8_t> dst_buffer(dst_buffersize);

    int rc = image_read_patch(
        filesize,
        read_file_callback_p,
        read_file_handle,
        src_x,
        src_y,
        src_width,
        src_height,
        dst_width,
        dst_height,
        dst_buffer.data(),
        dst_buffersize,
        returncode
    );
    if(rc != OK)
        return rc;
    
    if(lossless) {
        if(returncode != NULL) *returncode = NOT_IMPLEMENTED;
        return NOT_IMPLEMENTED;
    } else {
        const std::expected<Buffer_p, int> jpeg_buffer_x = 
            jpeg_compress(dst_buffer.data(), dst_width, dst_height);
        if(!jpeg_buffer_x) {
            if(returncode != NULL) *returncode = jpeg_buffer_x.error();
            return jpeg_buffer_x.error();
        }

        const Buffer_p jpeg_buffer_p = jpeg_buffer_x.value();
        _buffers[jpeg_buffer_p->data] = jpeg_buffer_p;

        *output_buffer = jpeg_buffer_p->data;
        *output_size = jpeg_buffer_p->size;
    }

    if(returncode != NULL) *returncode = OK;
    return OK;
}


int free_output_buffer(uint8_t* buffer_p) {
    if(_buffers.contains(buffer_p))
        _buffers.erase(buffer_p);
    return 0;
}



