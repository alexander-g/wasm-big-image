#include <cstdio>
#include <cstdint>


int open_asset(const char* path, FILE** fp, size_t* fsize);
int mock_read_callback(void* handle, void* dstbuf, uint64_t start, uint64_t size);
