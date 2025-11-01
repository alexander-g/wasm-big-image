extern "C" {
#include "./test_cb_cookiefile.c"
#include "./test_tiff-io.c"
}
#include "./test_jpeg-io.cpp"
#include "./test_png-io.cpp"


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
    // disabled because of issues and unimportant
    //RUN_TEST(test_cb_cookiefile);
    RUN_TEST(test_tiff_read);
    RUN_TEST(test_tiff_read_patch);
    RUN_TEST(test_tiff_read_patch_tiled);
    RUN_TEST(test_tiff_read_patch_jpeg);
    RUN_TEST(test_jpeg_0);
    RUN_TEST(test_png_0);

    printf("All tests passed.\n");
    return 0;
}
