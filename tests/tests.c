#include "./test_cb_cookiefile.c"



#define RUN_TEST(func)          \
    printf("%s...", #func);    \
    func();                     \
    printf("OK\n");



int main() {
    RUN_TEST(test_cb_cookiefile);

    printf("All tests passed.\n");
    return 0;
}
