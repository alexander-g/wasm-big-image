#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../src/tiff-io.h"


int mock_read_callback_faulty(void* handle, void* dstbuf, size_t start, size_t size) {
    return -1;
}

int mock_read_callback2(void* handle, void* dstbuf, size_t start, size_t size) {
    //printf("\nDBG>>> %x %i %i\n", handle, start, size); fflush(stdout);

    FILE* fp = (FILE*) handle;
    fseek(fp, start, SEEK_SET);
    fread(dstbuf, 1, size, fp);

    // TODO
    return 0;
}



static int open_asset_tiff(FILE** fp, size_t* fsize) {
    FILE* f = fopen("tests/assets/sheep.tiff", "rb");
    if(f == NULL)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return -1; 
    }
    const long size = ftell(f);
    if(size < 0) { 
        fclose(f); 
        return -1; 
    }

    *fp = f;
    *fsize = (size_t) size;
    return 0;
}


int test_tiff_read() {
    const size_t nbytes = 1024*1024*4;
    void* buffer = malloc(nbytes);

    int rc = tiff_read(999, mock_read_callback_faulty, NULL, buffer, nbytes);
    // faulty should not return nonzero rc
    assert(rc != 0);


    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(&fp, &fsize);
    assert(rc == 0);
    
    rc = tiff_read(fsize, mock_read_callback2, (void*)fp, buffer, nbytes);
    assert(rc == 0);

    return 0;
}



