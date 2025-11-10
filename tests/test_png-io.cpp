#include <cstdio>

#include "../src/png-io.hpp"
#include "./util.hpp"




const char* PNGFILE_0 = "tests/assets/png0.png";





int test_png_0(){
    int rc;

    FILE* fp;
    size_t fsize;
    rc = open_asset(PNGFILE_0, &fp, &fsize);
    assert(rc == 0);

    int32_t width, height;
    rc = 777;
    png_get_size(
        fsize, 
        (const void*) mock_read_callback3, 
        (void*)fp, 
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


int test_png_compress_binary_image0() {
    const int32_t w = 400;
    const int32_t h = 500;
    uint8_t data[w*h] = {255};
    data[777] = 1;

    const auto buffer = png_compress_binary_image(data, w, h);
    assert( buffer.has_value() );

    return 0;
}



