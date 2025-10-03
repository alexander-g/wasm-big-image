#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/cb_cookiefile.h"


// Mock data to simulate a "file"
static const char mock_data[] = "Hello, fopencookie!";
static const size_t mock_data_size = sizeof(mock_data) - 1; // exclude null terminator

// Mock read callback
int mock_read_callback(void* dstbuf, size_t start, size_t size) {
    if (start >= mock_data_size) 
        return 0;
    size_t available = mock_data_size - start;
    size_t tocopy = size < available ? size : available;
    memcpy(dstbuf, mock_data + start, tocopy);
    return 0;
}

int test_cb_cookiefile(void) {
    FILE* file = cb_fopen(mock_data_size, mock_read_callback);
    assert(file != NULL);

    char buf[32] = {0};

    // Test reading entire data
    size_t nread = fread(buf, 1, mock_data_size, file);
    assert(nread == mock_data_size);
    assert(memcmp(buf, mock_data, mock_data_size) == 0);

    // Test seeking and reading again
    assert(fseek(file, 7, SEEK_SET) == 0);
    memset(buf, 0, sizeof(buf));
    // reading too much, should stop at the end
    nread = fread(buf, 1, 99999, file); // should read "fopencookie"
    assert(nread == 12);
    assert(memcmp(buf, "fopencookie!", 12) == 0);

    // Test EOF
    memset(buf, 0, sizeof(buf));
    nread = fread(buf, 1, 10, file); // past EOF
    assert(nread == 0);

    // seek from the back
    assert(fseek(file, 7, SEEK_END) == 0);
    nread = fread(buf, 1, 6, file);
    assert(nread == 6);
    assert(memcmp(buf, "cookie", 6) == 0);


    fclose(file);
    return 0;
}
