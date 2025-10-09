#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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


int test_tiff_read_patch() {
    int rc;
    const size_t nbytes = 1024*1024*4;
    void* buffer = malloc(nbytes);

    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(&fp, &fsize);
    assert(rc == 0);
    
    rc = tiff_read_patch(
        fsize, 
        mock_read_callback2, 
        (void*)fp, 
        /*offset_x     =*/ 50,
        /*offset_y     =*/ 140,
        /*patch_width  =*/ 25,
        /*patch_height =*/ 25,
        buffer, 
        nbytes
    );
    assert(rc == 0);

    // actual bug
    rc = tiff_read_patch(
        fsize, 
        mock_read_callback2, 
        (void*)fp, 
        /*offset_x     =*/ 50,
        /*offset_y     =*/ 50,
        /*patch_width  =*/ 50,
        /*patch_height =*/ 50,
        buffer, 
        nbytes
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2]);


    rc = tiff_read_patch(
        fsize, 
        mock_read_callback2, 
        (void*)fp, 
        /*offset_x     =*/ 50,
        /*offset_y     =*/ 0,
        /*patch_width  =*/ 50,
        /*patch_height =*/ 100,
        buffer, 
        nbytes
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2]);

    rc = tiff_read_patch(
        fsize, 
        mock_read_callback2, 
        (void*)fp, 
        /*offset_x     =*/ 100,
        /*offset_y     =*/ 0,
        /*patch_width  =*/ 100,
        /*patch_height =*/ 150,
        buffer, 
        nbytes
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2], ((uint8_t*) buffer)[3]);
    printf("rgb= %i - %i - %i - %i\n", ((uint8_t*) buffer)[400+0], ((uint8_t*) buffer)[400+1], ((uint8_t*) buffer)[400+2], ((uint8_t*) buffer)[400+3]);


    // out of bounds
    rc = tiff_read_patch(
        fsize, 
        mock_read_callback2, 
        (void*)fp, 
        /*offset_x     =*/ 150,
        /*offset_y     =*/ 100,
        /*patch_width  =*/ 100,
        /*patch_height =*/ 100,
        buffer, 
        nbytes
    );
    assert(rc == 0);

    return 0;
}
