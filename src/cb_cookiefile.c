
// https://thomas.touhey.uk/2016/10/30/fopencookie.html



#define _GNU_SOURCE /* otherwise fopencookie is not declared */
#include <stdio.h>
#include <stdlib.h>

#include "./cb_cookiefile.h"
#include "./util.h"




ssize_t cb_fread(void* vcookie, char* buf, size_t size) {
    struct cb_handle* cookie = vcookie;
    size_t toread = min(cookie->size - cookie->cursor, size);
    
    if(toread == 0)
        return 0;

    const int rc = cookie->read_file_callback(
        cookie->read_file_handle, 
        buf, 
        cookie->cursor, 
        size
    );
    if (rc < 0) {
       return -1; // Error reading
}

    cookie->cursor += toread;
    return toread;
}

int64_t cb_fseek(void* vcookie, uint64_t offset, int whence) {
    struct cb_handle* cookie = vcookie;
    size_t pos = 0;

    /* get pos according to whence */
    switch (whence) {
    case SEEK_SET:
    case SEEK_END:
        if (offset < 0 || offset > cookie->size)
            return -1;
        if (whence == SEEK_SET) 
            pos = offset;
        else 
            pos = cookie->size - offset;
        break;

    case SEEK_CUR:
        if (offset + cookie->cursor < 0)
            return -1;
        pos = offset;
        if (pos > cookie->size)
            return -1;
        break;
    }

    /* set the position and return */
    cookie->cursor = pos;
    return (int64_t)cookie->cursor;
}

// fopencookie expects a pointer rather than value
int _cb_fseek(void* vcookie, off64_t* off, int whence){
    const long offset = *off;
    return cb_fseek(vcookie, offset, whence);
}


int cb_fclose(void* cookie) {
    free(cookie);
}

FILE* cb_fopen(
    size_t                   size, 
    read_file_callback_ptr_t read_file_callback, 
    const void*              read_file_handle
) {
    struct cb_handle* cookie = malloc(sizeof(struct cb_handle));
    if (!cookie) 
        return (NULL);
    
    *cookie = (struct cb_handle){
        .cursor = 0,
        .size   = size,
        .read_file_callback = read_file_callback,
        .read_file_handle   = read_file_handle,
    };

    FILE* file = fopencookie(
        cookie, 
        "r",
        (cookie_io_functions_t){cb_fread, NULL, _cb_fseek, cb_fclose}
    );
    if (!file) 
        cb_fclose(cookie);
    
    return file;
}

