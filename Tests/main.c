#include "AST/type.h"
#include "core/exitCode.h"
#include "core/log.h"
#include "debug/dump.h"
#include "launch.h"

#include "AST/typeList.h"

#define MTR_TESTING
#include "test.h"

#include "stl/mtr_stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MTR_MK
#   define MTR_PATH(path) "Tests/"path
#else
#   define MTR_PATH(path) "../../../Tests/"path
#endif

TEST_CASE(no_file) {
    CHECK(mtr_launch("nofile.mtr") == MTR_FILE_ERROR);
}

TEST_CASE(simple_test) {
    const char* path = MTR_PATH("main.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

TEST_CASE(parser_test) {
    const char* path = MTR_PATH("parser_error.mtr");
    CHECK(mtr_launch(path) == MTR_PARSER_ERROR);
}

TEST_CASE(fibbonacci_test) {
    const char* path = MTR_PATH("fib.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

TEST_CASE(closure_test) {
    const char* path = MTR_PATH("closure.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

TEST_CASE(user_types) {
    const char* path = MTR_PATH("userTypes.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

static void all_tests() {
    no_file();
    simple_test();
    parser_test();
    // fibbonacci_test();
    closure_test();
    user_types();
    REPORT();
}

int main()
{
    all_tests();
}
