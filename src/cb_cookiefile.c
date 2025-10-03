
// https://thomas.touhey.uk/2016/10/30/fopencookie.html



#define _GNU_SOURCE /* otherwise fopencookie is not declared */
#include <stdio.h>
#include <stdlib.h>

#include "./cb_cookiefile.h"


#define min(A, B) ((A) < (B) ? (A) : (B))



struct cb_cookie {
    /** Size of the file */
    size_t size;
    
    /** Current read position */
    size_t cursor;

    /** Function to call to read from the file */
    read_file_callback_ptr_t read_file_callback;
};

static ssize_t cb_fread(void* vcookie, char* buf, size_t size) {
    struct cb_cookie* cookie = vcookie;
    size_t toread = min(cookie->size - cookie->cursor, size);
    
    if(toread == 0)
        return 0;

    const int rc = cookie->read_file_callback(buf, cookie->cursor, size);
    if (rc < 0) {
       return -1; // Error reading
}

    cookie->cursor += toread;
    return toread;
}

static int cb_fseek(void* vcookie, off64_t* off, int whence) {
    struct cb_cookie* cookie = vcookie;
    size_t pos = 0;

    /* get pos according to whence */
    switch (whence) {
    case SEEK_SET:
    case SEEK_END:
        if (*off < 0 || *off > cookie->size)
            return -1;
        if (whence == SEEK_SET) 
            pos = *off;
        else 
            pos = cookie->size - *off;
        break;

    case SEEK_CUR:
        if (*off + cookie->cursor < 0)
            return -1;
        pos = *off;
        if (pos > cookie->size)
            return -1;
        break;
    }

    /* set the position and return */
    cookie->cursor = pos;
    return 0;
}

static int cb_fclose(void* cookie) {
    free(cookie);
}

FILE* cb_fopen(size_t size, read_file_callback_ptr_t read_file_callback) {
    struct cb_cookie* cookie = malloc(sizeof(struct cb_cookie));
    if (!cookie) 
        return (NULL);
    
    *cookie = (struct cb_cookie){
        .cursor = 0,
        .size = size,
        .read_file_callback = read_file_callback,
    };

    FILE* file = fopencookie(
        cookie, 
        "r",
        (cookie_io_functions_t){cb_fread, NULL, cb_fseek, cb_fclose}
    );
    if (!file) 
        cb_fclose(cookie);
    
    return file;
}

