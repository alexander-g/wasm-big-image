/** Emulate a readonly file stream by requesting data from a callback */

typedef int (*read_file_callback_ptr_t)(void* dstbuf, size_t start, size_t size);

FILE* cb_fopen(size_t size, read_file_callback_ptr_t read_file_callback);

