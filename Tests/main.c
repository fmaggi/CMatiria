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

TEST_CASE(simple) {
    CHECK(mtr_launch(MTR_PATH("main.mtr")) == MTR_OK);
}

TEST_CASE(parser) {
    CHECK(mtr_launch(MTR_PATH("parser_error.mtr")) == MTR_PARSER_ERROR);
}

TEST_CASE(fibbonacci) {
    CHECK(mtr_launch(MTR_PATH("fib.mtr")) == MTR_OK);
}

TEST_CASE(closure) {
    CHECK(mtr_launch(MTR_PATH("closure.mtr")) == MTR_OK);
}

TEST_CASE(user_types) {
    CHECK(mtr_launch(MTR_PATH("userTypes.mtr")) == MTR_OK);
}

TEST_CASE(scope) {
    CHECK(mtr_launch(MTR_PATH("scope.mtr")) == MTR_OK);
}

static void all_tests() {
    no_file();
    parser();
    simple();
    scope();
    fibbonacci();
    closure();
    user_types();
    scope();
    REPORT();
}

int main()
{
    all_tests();
}
