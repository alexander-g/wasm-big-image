/** TIFF image read functions */

/** Read and decode a full tiff image (RGBA) */
int tiff_read(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    void*       buffer,
    size_t      buffersize
);

