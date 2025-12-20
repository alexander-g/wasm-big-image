#undef NDEBUG

extern "C" {
#include "./test_cb_cookiefile.c"
}
#include "./test_tiff-io.cpp"
#include "./test_jpeg-io.cpp"
#include "./test_png-io.cpp"
#include "./test_util.cpp"


#define RUN_TEST(func)               \
    {                                \
    printf("\n%s...\n", #func);      \
    fflush(stdout);                  \
    const int rc = func();           \
    if(rc != 0){                     \
        printf("%s...FAIL\n", #func);\
        return 1;                    \
    }                                \
    printf("%s...OK\n", #func);      \
    }



int main() {
    RUN_TEST(test_nn_interpolation_full);
    RUN_TEST(test_nn_interpolation_streaming_only);
    RUN_TEST(test_nn_interpolation_single_row_upscaling);

    // disabled because of issues and unimportant
    //RUN_TEST(test_cb_cookiefile);

    RUN_TEST(test_tiff_read);
    RUN_TEST(test_tiff_read_patch);
    RUN_TEST(test_tiff_read_patch_tiled);
    RUN_TEST(test_tiff_read_patch_jpeg);

    RUN_TEST(test_jpeg_0);
    RUN_TEST(test_jpeg_upscale);
    RUN_TEST(test_jpeg_compress0);

    RUN_TEST(test_png_0);
    RUN_TEST(test_png_upscale);
    RUN_TEST(test_png_1_gray);
    RUN_TEST(test_png_2_binary);
    RUN_TEST(test_png_3_indexed);
    RUN_TEST(test_png_compress_binary_image0);
    RUN_TEST(test_png_compress_and_upscale_rgba);
    RUN_TEST(test_png_compress_rgb_image0);

    printf("All tests passed.\n");
    return 0;
}
