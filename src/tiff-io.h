/** TIFF image read functions */

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


