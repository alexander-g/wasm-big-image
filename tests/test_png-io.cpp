#include <cstdio>

#include "../src/png-io.hpp"




const char* PNGFILE_0 = "tests/assets/png0.png";
const char* PNGFILE_1 = "tests/assets/png1_gray-8bit.png";
const char* PNGFILE_2 = "tests/assets/png2_binary-1bit.png";
const char* PNGFILE_3 = "tests/assets/png3_indexed.png";



int test_png_0(){
    int rc;

    const auto fhandle_o = FileHandle::open(PNGFILE_0);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    int32_t width, height;
    rc = 777;
    png_get_size(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        &width,
        &height,
        &rc
    );
    assert(rc == 0);
    assert(width == 555);
    assert (height == 888);


    uint8_t buffer[1000*1000*4];
    rc = 777;
    png_read_patch(
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
    assert( (firstpixel & 0x00ff0000) >> 16 == 17  );
    assert( (firstpixel & 0x0000ff00) >> 8  == 177 );
    assert( (firstpixel & 0x000000ff) >> 0  == 77  );

    const uint32_t lastpixel = ((uint32_t*)buffer)[499*500 + 499];
    assert( (lastpixel & 0xff000000) >> 24 == 0xff );
    assert( (lastpixel & 0x00ff0000) >> 16 == 0    );
    assert( (lastpixel & 0x0000ff00) >> 8  == 50   );
    assert( (lastpixel & 0x000000ff) >> 0  == 230  );

    return 0;
}


int test_png_1_gray(){
    int rc;

    const auto fhandle_o = FileHandle::open(PNGFILE_1);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    int32_t width, height;
    rc = 777;
    png_get_size(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        &width,
        &height,
        &rc
    );
    assert(rc == 0);
    assert(width == 100);
    assert (height == 200);



    uint8_t buffer[1000*1000*4] = {1};
    memset(buffer, 0, 1000*1000*4);
    rc = 777;
    png_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 55,
        /*src_y      =*/ 55,
        /*src_width  =*/ 2,
        /*src_height =*/ 2,
        /*dst_width  =*/ 2,
        /*dst_height =*/ 2,
        buffer,
        sizeof(buffer),
        &rc
    );


    uint32_t firstpixel = ((uint32_t*)buffer)[0];
    assert( firstpixel == 0xfffefefe );
    uint32_t lastpixel = ((uint32_t*)buffer)[3];
    assert( lastpixel == 0xff000000 );

    rc = 777;
    png_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 30,
        /*src_y      =*/ 20,
        /*src_width  =*/ 40,
        /*src_height =*/ 30,
        /*dst_width  =*/ 40,
        /*dst_height =*/ 30,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);

    firstpixel = ((uint32_t*)buffer)[0];
    assert( firstpixel == 0xffc8c8c8 );
    lastpixel = ((uint32_t*)buffer)[29*40 + 39];
    assert( lastpixel == 0xff000000 );

    return 0;
}



int test_png_2_binary(){
    int rc;

    const auto fhandle_o = FileHandle::open(PNGFILE_2);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    int32_t width, height;
    rc = 777;
    png_get_size(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        &width,
        &height,
        &rc
    );
    assert(rc == 0);
    assert(width == 50);
    assert (height == 150);



    uint8_t buffer[1000*1000*4] = {1};
    memset(buffer, 0, 1000*1000*4);
    rc = 777;
    png_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 20,
        /*src_y      =*/ 10,
        /*src_width  =*/ 16,
        /*src_height =*/ 11,
        /*dst_width  =*/ 16,
        /*dst_height =*/ 11,
        buffer,
        sizeof(buffer),
        &rc
    );


    uint32_t firstpixel = ((uint32_t*)buffer)[0];
    assert( firstpixel == 0xffffffff );
    uint32_t lastpixel = ((uint32_t*)buffer)[10*16 + 15];
    assert( lastpixel == 0xff000000 );

    return 0;
}



int test_png_3_indexed(){
    int rc;

    const auto fhandle_o = FileHandle::open(PNGFILE_3);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    int32_t width, height;
    rc = 777;
    png_get_size(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        &width,
        &height,
        &rc
    );
    assert(rc == 0);
    assert(width == 200);
    assert (height == 250);



    uint8_t buffer[1000*1000*4] = {1};
    memset(buffer, 0, 1000*1000*4);
    rc = 777;
    png_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 50,
        /*src_y      =*/ 30,
        /*src_width  =*/ 51,
        /*src_height =*/ 31,
        /*dst_width  =*/ 51,
        /*dst_height =*/ 31,
        buffer,
        sizeof(buffer),
        &rc
    );


    uint32_t firstpixel = ((uint32_t*)buffer)[0];
    assert( firstpixel == 0xff0000ff );
    uint32_t lastpixel = ((uint32_t*)buffer)[30*51+50];
    assert( lastpixel == 0xff000000 );

    return 0;
}



int memory_read_cb(
    const uint8_t* datahandle,
    void*       dstbuf, 
    uint64_t    start, 
    uint64_t    size
) {
    memcpy(dstbuf, datahandle + start, size);
    return 0;
}


int test_png_compress_binary_image0() {
    const int32_t w = 400;
    const int32_t h = 500;
    uint8_t data[h*w];
    memset(data, 200, sizeof(data));
    data[ 77*w + 99 ] = 0;
    data[ 77*w + 100 ] = 255;

    const auto outbuffer_x = png_compress_image(data, w, h, 1);
    assert( outbuffer_x.has_value() );
    const auto outbuffer = outbuffer_x.value();

    uint8_t buffer[1000*1000*4] = {77};
    int rc = 777;
    png_read_patch(
        outbuffer->size, 
        (const void*) &memory_read_cb, 
        (void*) outbuffer->data, 
        /*src_x      =*/ 98,
        /*src_y      =*/ 76,
        /*src_width  =*/ 3,
        /*src_height =*/ 3,
        /*dst_width  =*/ 3,
        /*dst_height =*/ 3,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);
    
    // white because thresholded during compression
    assert( ((uint32_t*)buffer)[0] == 0xffffffff ); 
    assert( ((uint32_t*)buffer)[1*3 + 1] == 0xff000000 );
    assert( ((uint32_t*)buffer)[1*3 + 2] == 0xffffffff );

    return 0;
}


int test_png_compress_rgb_image0() {
    const int32_t w = 400;
    const int32_t h = 500;
    uint8_t data[h*w*3];
    memset(data, 200, sizeof(data));
    data[ 77*w*3 + 99*3 +0 ] = 0;
    data[ 77*w*3 + 99*3 +1 ] = 0;
    data[ 77*w*3 + 99*3 +2 ] = 0;
    data[ 77*w*3 + 100*3 +0] = 0xff;
    data[ 77*w*3 + 100*3 +1] = 0xf0;
    data[ 77*w*3 + 100*3 +2] = 0x0a;

    const auto outbuffer_x = png_compress_image(data, w, h, 3);
    assert( outbuffer_x.has_value() );
    const auto outbuffer = outbuffer_x.value();

    uint8_t buffer[1000*1000*4] = {77};
    int rc = 777;
    png_read_patch(
        outbuffer->size, 
        (const void*) &memory_read_cb, 
        (void*) outbuffer->data, 
        /*src_x      =*/ 98,
        /*src_y      =*/ 76,
        /*src_width  =*/ 3,
        /*src_height =*/ 3,
        /*dst_width  =*/ 3,
        /*dst_height =*/ 3,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);
    
    
    assert( ((uint32_t*)buffer)[0] == 0xffc8c8c8 );
    assert( ((uint32_t*)buffer)[1*3 + 1] == 0xff000000 );
    assert( ((uint32_t*)buffer)[1*3 + 2] == 0xff0af0ff );

    return 0;
}




int test_png_compress_and_upscale_rgba() {
    EigenRGBAMap imagedata(100,50,4);
    imagedata.setConstant(177);
    imagedata(55,35,0) = 255;
    imagedata(55,35,1) = 0;
    imagedata(55,35,2) = 0;
    imagedata(55,35,3) = 255;

    const auto expect_buffer = 
        resize_image_and_encode_as_png(imagedata, {.width=10002, .height=10001});
    assert(expect_buffer.has_value());
    const auto buffer = expect_buffer.value();

    int width = -1, height = -1;
    int rc = -999;
    png_get_size(
        buffer->size, 
        (const void*) &memory_read_cb, 
        (void*) buffer->data, 
        &width,
        &height,
        &rc
    );
    assert(rc == 0);
    assert(width == 10002);
    assert(height == 10001);


    uint8_t readbuffer[1000*1000*4] = {44};
    rc = 777;
    png_read_patch(
        buffer->size, 
        (const void*) &memory_read_cb, 
        (void*) buffer->data, 
        /*src_x      =*/ 7000,  // 35/50  * 10002
        /*src_y      =*/ 5500,  // 55/100 * 10001
        /*src_width  =*/ 3,
        /*src_height =*/ 3,
        /*dst_width  =*/ 3,
        /*dst_height =*/ 3,
        readbuffer,
        sizeof(readbuffer),
        &rc
    );
    assert(rc==0);
    
    assert( ((uint32_t*)readbuffer)[0]      == 0xb1b1b1b1 ); // 177
    assert( ((uint32_t*)readbuffer)[1+3 +1] == 0xff0000ff ); // red


    return 0;
}



// bug: read patch upscaling image
int test_png_upscale(){
    int rc;

    const auto fhandle_o = FileHandle::open(PNGFILE_0);
    assert(fhandle_o.has_value());
    const FileHandle* fhandle = fhandle_o.value().get();

    // NOTE to self: maximum buffer size: 8MB otherwise getting weird segfaults
    uint8_t buffer[1110*1800*4];
    rc = 777;
    png_read_patch(
        fhandle->size, 
        (const void*) &fhandle->read_callback, 
        (void*) fhandle, 
        /*src_x      =*/ 0,
        /*src_y      =*/ 0,
        /*src_width  =*/ 555,
        /*src_height =*/ 888,
        /*dst_width  =*/ 555*2,
        /*dst_height =*/ 888*2,
        buffer,
        sizeof(buffer),
        &rc
    );
    assert(rc==0);

    return 0;
}

