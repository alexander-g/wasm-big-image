#include "./test_cb_cookiefile.c"
#include "./test_tiff-io.c"



#define RUN_TEST(func)               \
    {                                \
    printf("%s...", #func);          \
    fflush(stdout);                  \
    const int rc = func();           \
    if(rc != 0){                     \
        printf("FAIL\n");            \
        return 1;                    \
    }                                \
    printf("OK\n");                  \
    }



int main() {
    RUN_TEST(test_cb_cookiefile);
    RUN_TEST(test_tiff_read);
    RUN_TEST(test_tiff_read_patch);

    printf("All tests passed.\n");
    return 0;
}
