// this file is just temporary (probably :) )

#include "compiler.h"
#include "vm.h"
#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>

int main(int argc, char* argv[])
{
    char* source = mtr_read_file(argv[1]);
    struct mtr_package* package = mtr_compile(source);
    if (NULL == package)
        return -1;

    i32 result = mtr_execute(package);
    MTR_LOG_DEBUG("return %i", result);

    mtr_delete_package(package);
    free(source);

    return result;
}
