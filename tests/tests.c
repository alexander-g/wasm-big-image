#include "./test_cb_cookiefile.c"
#include "./test_tiff-io.c"



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
    RUN_TEST(test_cb_cookiefile);
    RUN_TEST(test_tiff_read);
    RUN_TEST(test_tiff_read_patch);
    RUN_TEST(test_tiff_read_patch_tiled);

    printf("All tests passed.\n");
    return 0;
}
