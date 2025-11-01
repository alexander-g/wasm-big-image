#include <cstdint>
#include <stdexcept>



extern "C" {

#include "./tiff-io.h"
#include "./jpeg-io.hpp"
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
    
    size_t width_sz, height_sz;
    rc = tiff_get_size(
        filesize, 
        (read_file_callback_ptr_t) read_file_callback_p, 
        read_file_handle, 
        &width_sz, 
        &height_sz, 
        //returncode,  //TODO
        NULL
    );
    if(rc == OK){
        *width  = (int32_t)width_sz;
        *height = (int32_t)height_sz;
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

    if(returncode != NULL) *returncode = NOT_IMPLEMENTED;
    return NOT_IMPLEMENTED;
}

} // extern "C"