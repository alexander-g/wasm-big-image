/** TIFF image read functions */

#include <stddef.h>

#include "./cb_cookiefile.h"


enum Error_TIFF_IO { 

    TIFF_OPEN_FAILED             = -201, 
    TIFF_GET_IMAGE_WIDTH_FAILED  = -202, 
    TIFF_GET_IMAGE_HEIGHT_FAILED = -203, 
    TIFF_READ_FULL_FAILED        = -204, 
    INVALID_SIZES2               = -205, 
    TIFF_GET_ROWS_PER_STRIP_FAILED = -206, 
    TIFF_READ_STRIP_FAILED       = -207, 
    TIFF_GET_TILE_SIZES_FAILED   = -208, 
    TIFF_READ_TILE_FAILED        = -209,

};


/** Read and decode a full tiff image (RGBA) */
int tiff_read(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    void*       buffer,
    size_t      buffersize
);

/** Read a sub-image patch without reading the full image, and resize it to
   dst_width x dst_height (currently only nearest neighbor interpolation). */
int tiff_read_patch(
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


int tiff_get_size(
    size_t      filesize,
    const read_file_callback_ptr_t read_file_callback_p,
    const void* read_file_handle,
    size_t*     width,
    size_t*     height,
    // TODO: return code
    void**      tif_p
);

