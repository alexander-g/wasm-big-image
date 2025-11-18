#include "../src/jpeg-io.hpp"
#include "../src/util.hpp"

#include <cstdio>



const char* JPEGFILE_0 = "tests/assets/jpeg0.jpg";




int test_jpeg_0(){
    int rc;
    
    const auto fhandle_o = FileHandle::open(JPEGFILE_0);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    int32_t width, height;
    rc = 777;
    jpeg_get_size(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
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
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
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
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
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
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
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


int test_jpeg_upscale(){
    int rc;
    
    const auto fhandle_o = FileHandle::open(JPEGFILE_0);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    uint8_t buffer[1000*1600*4];
    rc = 777;
    jpeg_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 0,
        /*src_y      =*/ 0,
        /*src_width  =*/ 444,
        /*src_height =*/ 777,
        /*dst_width  =*/ 444*2,
        /*dst_height =*/ 777*2,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);

    return 0;
}