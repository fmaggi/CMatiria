// this file is just temporary (probably :) )

#include "compiler.h"
#include "runtime/engine.h"
#include "core/file.h"
#include "core/log.h"

#include "stl/mtr_stdlib.h"

#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return -1;
    }
    char* source = mtr_read_file(argv[1]);
    struct mtr_package* package = mtr_compile(source);
    if (NULL == package)
        return -1;

    mtr_add_io(package);

    struct mtr_engine engine;

    i32 result = mtr_execute(&engine, package);
    (void) result;
    MTR_LOG_DEBUG("return %i", result);

    mtr_delete_package(package);
    free(source);

    return 0;
}
