/** Emulate a readonly file stream by requesting data from a callback */

#include <stdint.h>


typedef int (*read_file_callback_ptr_t)(
    /** Callback-specific data handle */
    const void* handle,
    /** Destination data buffer */
    void* dstbuf, 
    /** Starting read position */
    uint64_t start, 
    /** Number of bytes to read */
    uint64_t size
);

/** Emulate a file stream that can be used with fwrite, fread etc. */
FILE* cb_fopen(
    size_t                   size, 
    read_file_callback_ptr_t read_file_callback, 
    const void*              read_file_handle
);


/** Handle passed by fwrite, libTIFF, etc to cb_fread, cb_fseek etc. */
struct cb_handle {
    /** Size of the file */
    size_t size;
    
    /** Current read position */
    size_t cursor;

    /** Function to call to read from the file */
    read_file_callback_ptr_t read_file_callback;

    /** Handle passed to read_file_callback() */
    const void* read_file_handle;
};


ssize_t cb_fread(void* vhandle, char* buf, size_t size);
int64_t cb_fseek(void* vhandle, uint64_t offset, int whence);
int     cb_fclose(void* handle);


