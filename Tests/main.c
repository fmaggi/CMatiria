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

static const char* build_path(const char* filename) {
    static char buf[1024];
    #ifdef MTR_MK
       sprintf(buf, "%s", "Tests/");
    #else
       sprintf(buf, "%s", "../../../Tests/");
    #endif
    if (strlen(filename) + strlen(buf) >= 1024) {
        MTR_LOG_ERROR("Filename error");
        return NULL;
    }

    strcat(buf, filename);
    return buf;
}

TEST_CASE(no_file) {
    CHECK(mtr_launch("nofile.mtr") == MTR_FILE_ERROR);
}

TEST_CASE(simple_test) {
    const char* path = build_path("main.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

TEST_CASE(parser_test) {
    const char* path = build_path("parser_error.mtr");
    CHECK(mtr_launch(path) == MTR_PARSER_ERROR);
}

TEST_CASE(fibbonacci_test) {
    const char* path = build_path("fib.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

TEST_CASE(closure_test) {
    const char* path = build_path("closure.mtr");
    CHECK(mtr_launch(path) == MTR_OK);
}

static void all_tests() {
    no_file();
    simple_test();
    parser_test();
    // fibbonacci_test();
    closure_test();
    REPORT();
}

int main()
{
    all_tests();
}
