#include "../src/jpeg-io.hpp"

#include <cstdio>



const char* JPEGFILE_0 = "tests/assets/jpeg0.jpg";



// TODO: code-duplication!

int mock_read_callback3(void* handle, void* dstbuf, uint64_t start, uint64_t size) {
    //printf("\nDBG>>> %x %i %i\n", handle, start, size); fflush(stdout);

    FILE* fp = (FILE*) handle;
    fseek(fp, start, SEEK_SET);
    fread(dstbuf, 1, size, fp);

    // TODO
    return 0;
}


static int open_asset_jpeg(const char* path, FILE** fp, size_t* fsize) {
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





int test_jpeg_0(){
    int rc;

    FILE* fp;
    size_t fsize;
    rc = open_asset_jpeg(JPEGFILE_0, &fp, &fsize);
    assert(rc == 0);

    int32_t width, height;
    rc = 777;
    jpeg_get_size(
        fsize, 
        (const void*) mock_read_callback3, 
        (void*)fp, 
        &width,
        &height,
        &rc
    );
    assert(rc==0);
    assert(width == 444);
    assert(height == 777);



    uint8_t buffer[1000*1000*4];
    rc = 777;
    jpeg_read_patch(
        fsize, 
        (const void*) mock_read_callback3, 
        (void*)fp, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 50,
        /*src_width  =*/ 50,
        /*src_height =*/ 50,
        /*dst_width  =*/ 500,
        /*dst_height =*/ 500,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);
    
    
    const uint32_t firstpixel = ((uint32_t*)buffer)[0];
    assert( (firstpixel & 0xff000000) >> 24 == 0xff );
    assert( (firstpixel & 0x00ff0000) >> 16  > 14  );
    assert( (firstpixel & 0x00ff0000) >> 16  < 20  );
    assert( (firstpixel & 0x0000ff00) >> 8   > 174 );
    assert( (firstpixel & 0x0000ff00) >> 8   < 180 );
    assert( (firstpixel & 0x000000ff) >> 0   > 74  );
    assert( (firstpixel & 0x000000ff) >> 0   < 80  );

    const uint32_t lastpixel = ((uint32_t*)buffer)[499*500 + 499];
    assert( (lastpixel & 0xff000000) >> 24 == 0xff );
    assert( (lastpixel & 0x00ff0000) >> 16  < 10 );
    assert( (lastpixel & 0x0000ff00) >> 8   > 44 );
    assert( (lastpixel & 0x0000ff00) >> 8   < 55 );
    assert( (lastpixel & 0x000000ff) >> 0   > 225 );
    assert( (lastpixel & 0x000000ff) >> 0   < 245 );  // big deviation


    rc = 777;
    memset(buffer, 0, sizeof(buffer));
    jpeg_read_patch(
        fsize, 
        (const void*) mock_read_callback3, 
        (void*)fp, 
        /*src_x      =*/ 0,
        /*src_y      =*/ 0,
        /*src_width  =*/ 444,
        /*src_height =*/ 777*2,
        /*dst_width  =*/ 444,
        /*dst_height =*/ 1000,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);
    {
    // last valid pixel
    const uint32_t lastpixel = ((uint32_t*)buffer)[(1000/2-1)*444 + (444-1)];
    assert( (lastpixel & 0xff000000) >> 24 == 0xff );
    assert( (lastpixel & 0x00ff0000) >> 16  > 225 );
    assert( (lastpixel & 0x00ff0000) >> 16  < 245 );
    assert( (lastpixel & 0x0000ff00) >> 8   > 44 );
    assert( (lastpixel & 0x0000ff00) >> 8   < 55 );
    assert( (lastpixel & 0x000000ff) >> 0   > 44 );
    assert( (lastpixel & 0x000000ff) >> 0   < 55 );

    }

    rc = 777;
    jpeg_read_patch(
        fsize, 
        (const void*) mock_read_callback3, 
        (void*)fp, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 50,
        /*src_width  =*/ 500000000,
        /*src_height =*/ 50,
        /*dst_width  =*/ 50,
        /*dst_height =*/ 50,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);




    return 0;
}



int test_jpeg_compress0() {
    const int32_t w = 400;
    const int32_t h = 500;
    uint8_t data[w*h*4] = {255};

    const auto buffer = jpeg_compress(data, w, h);
    assert( buffer.has_value() );

    return 0;
}

