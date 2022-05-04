#include "compiler.h"
#include "runtime/engine.h"
#include "core/file.h"
#include "core/log.h"

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

enum code {
    OK,
    FILE_ERROR,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

static enum code launch(const char* name) {
    enum code c = OK;

    char* source = mtr_read_file(build_path(name));
    if (!source) {
        return FILE_ERROR;
    }
    struct mtr_package* package = mtr_compile(source);
    if (NULL == package) {
        c = COMPILE_ERROR;
        goto end;
    }

    mtr_add_io(package);

    struct mtr_engine engine;

    i32 result = mtr_execute(&engine, package);
    (void) result;

    mtr_delete_package(package);
end:
    free(source);
    return c;
}

TEST_CASE(simple_test) {
    CHECK(launch("main.mtr") == OK);
}

TEST_CASE(parser_test) {
    CHECK(launch("parser_error.mtr") == COMPILE_ERROR);
}

TEST_CASE(fibbonacci_test) {
    CHECK(launch("fib.mtr") == OK);
}

TEST_CASE(closure_test) {
    CHECK(launch("closure.mtr") == OK);
}

static void all_tests() {
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
