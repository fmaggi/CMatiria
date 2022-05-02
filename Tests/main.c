#include "compiler.h"
#include "runtime/engine.h"
#include "core/file.h"
#include "core/log.h"

#define MTR_TESTING
#include "test.h"

#include "stl/mtr_stdlib.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#ifdef MTR_MK
#   define MTR_PATH(path) "Tests/"path
#else
#   define MTR_PATH(path) "../../../Tests/"path
#endif

int main()
{
    char* source = mtr_read_file(MTR_PATH("closure.mtr"));
    MTR_LOG(MTR_PATH(""));
    struct mtr_package* package = mtr_compile(source);
    if (NULL == package)
        goto end;

    MTR_LOG_DEBUG("Adding io...");
    mtr_add_io(package);
    MTR_LOG_DEBUG("Done");


    struct mtr_engine engine;

    MTR_LOG_DEBUG("Executing...");
    i32 result = mtr_execute(&engine, package);
    MTR_LOG_DEBUG("Done");
    (void) result;
    MTR_LOG_DEBUG("return %i", result);

    mtr_delete_package(package);
end:
    free(source);
}
