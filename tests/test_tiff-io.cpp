#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "../src/tiff-io.hpp"


const char* TIFF_SHEEP = "tests/assets/sheep.tiff";
const char* TIFF_TILED = "tests/assets/tiled0.tiff";
const char* TIFF_JPEG  = "tests/assets/jpeg0.tiff";



int mock_read_callback_faulty(void* handle, void* dstbuf, uint64_t start, uint64_t size) {
    return -1;
}

int mock_read_callback2(void* handle, void* dstbuf, uint64_t start, uint64_t size) {
    //printf("\nDBG>>> %x %i %i\n", handle, start, size); fflush(stdout);

    FILE* fp = (FILE*) handle;
    fseek(fp, start, SEEK_SET);
    fread(dstbuf, 1, size, fp);

    // TODO
    return 0;
}



static int open_asset_tiff(const char* path, FILE** fp, size_t* fsize) {
    FILE* f = fopen(path, "rb");
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

    int rc = 
        tiff_read(999, (void*) mock_read_callback_faulty, NULL, buffer, nbytes);
    // faulty should not return nonzero rc
    assert(rc != 0);


    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(TIFF_SHEEP, &fp, &fsize);
    assert(rc == 0);
    
    rc = 
        tiff_read(fsize, (void*) mock_read_callback2, (void*)fp, buffer, nbytes);
    assert(rc == 0);

    return 0;
}


int test_tiff_read_patch() {
    int rc;
    const size_t nbytes = 1024*1024*4;
    void* buffer = malloc(nbytes);

    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(TIFF_SHEEP, &fp, &fsize);
    assert(rc == 0);
    
    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 140,
        /*src_width  =*/ 25,
        /*src_height =*/ 25,
        /*dst_width  =*/ 25,
        /*dst_height =*/ 25,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);

    // actual bug
    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 50,
        /*src_width  =*/ 50,
        /*src_height =*/ 50,
        /*dst_width  =*/ 50,
        /*dst_height =*/ 50,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2]);


    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 0,
        /*src_width  =*/ 50,
        /*src_height =*/ 100,
        /*dst_width  =*/ 50,
        /*dst_height =*/ 100,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2]);

    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 100,
        /*src_y      =*/ 0,
        /*src_width  =*/ 100,
        /*src_height =*/ 150,
        /*dst_width  =*/ 100,
        /*dst_height =*/ 150,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    printf("rgb= %i - %i - %i - %i\n", ((uint8_t*) buffer)[0], ((uint8_t*) buffer)[1], ((uint8_t*) buffer)[2], ((uint8_t*) buffer)[3]);
    printf("rgb= %i - %i - %i - %i\n", ((uint8_t*) buffer)[400+0], ((uint8_t*) buffer)[400+1], ((uint8_t*) buffer)[400+2], ((uint8_t*) buffer)[400+3]);


    // out of bounds
    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 150,
        /*src_y      =*/ 100,
        /*src_width  =*/ 100,
        /*src_height =*/ 100,
        /*dst_width  =*/ 100,
        /*dst_height =*/ 100,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);

    return 0;
}



int test_tiff_read_patch_tiled() {
    int rc;
    const size_t nbytes = 1024*1024*4;
    void* buffer = malloc(nbytes);

    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(TIFF_TILED, &fp, &fsize);
    assert(rc == 0);


    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 20,
        /*src_y      =*/ 20,
        /*src_width  =*/ 300,
        /*src_height =*/ 500,
        /*dst_width  =*/ 300,
        /*dst_height =*/ 500,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    assert(((uint32_t*)buffer)[0] == 0xff000000);  // (0,0,0,255)
    assert(((uint32_t*)buffer)[499*300+299] == 0xffff0000);  // (0,0,255,255)


    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 999,
        /*src_y      =*/ 799,
        /*src_width  =*/ 1,
        /*src_height =*/ 1,
        /*dst_width  =*/ 1,
        /*dst_height =*/ 1,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    assert(((uint32_t*)buffer)[0] == 0xfffafafa);  // (0,255,255,255)

    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 300,
        /*src_y      =*/ 400,
        /*src_width  =*/ 700,
        /*src_height =*/ 400,
        /*dst_width  =*/ 700,
        /*dst_height =*/ 400,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    assert(((uint32_t*)buffer)[0] == 0xffffff00);  // (0,255,255,255)
    assert(((uint32_t*)buffer)[399*700+699] == 0xfffafafa);  // (250,250,250,255)

    return 0;
}



int test_tiff_read_patch_jpeg() {
    int rc;
    const size_t nbytes = 1024*1024*4;
    void* buffer = malloc(nbytes);

    FILE* fp;
    size_t fsize;
    rc = open_asset_tiff(TIFF_JPEG, &fp, &fsize);
    assert(rc == 0);


    rc = tiff_read_patch(
        fsize, 
        (void*) mock_read_callback2, 
        (void*)fp, 
        /*src_x      =*/ 20,
        /*src_y      =*/ 20,
        /*src_width  =*/ 380,
        /*src_height =*/ 480,
        /*dst_width  =*/ 380,
        /*dst_height =*/ 480,
        buffer, 
        nbytes,
        NULL
    );
    assert(rc == 0);
    assert(((uint32_t*)buffer)[0] == 0xff000000);  // (0,0,0,255)

    // lossy compression
    const uint32_t lastpixel = ((uint32_t*)buffer)[479*380+379];
    assert( (lastpixel & 0xff000000) >> 24 == 0xff );
    assert( (lastpixel & 0x00ff0000) >> 16  > 122 );
    assert( (lastpixel & 0x00ff0000) >> 16  < 130 );
    assert( (lastpixel & 0x0000ff00) >> 8   > 250 );
    assert( (lastpixel & 0x000000ff) >> 0   > 122 );
    assert( (lastpixel & 0x000000ff) >> 0   < 130 );

    return 0;
}

