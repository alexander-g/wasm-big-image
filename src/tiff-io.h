/** TIFF image read functions */


enum Error { 
    OK = 0,

    TIFF_OPEN_FAILED             = -1, 
    TIFF_GET_IMAGE_WIDTH_FAILED  = -2, 
    TIFF_GET_IMAGE_HEIGHT_FAILED = -3, 
    BUFFER_TOO_SMALL      = -4,
    INVALID_SIZES         = -5, 
    TIFF_READ_FULL_FAILED = -6, 
    NEGATIVE_OFFSETS      = -7, 
    OFFSETS_OUT_OF_BOUNDS = -8,
    INVALID_SIZES2        = -9, 
    TIFF_GET_ROWS_PER_STRIP_FAILED = -10, 
    MALLOC_FAILED          = -11,
    TIFF_READ_STRIP_FAILED = -12, 

    NOT_IMPLEMENTED       = -999,
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


