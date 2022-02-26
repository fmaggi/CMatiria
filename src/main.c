// this file is just temporary (probably :) )

#include "compiler.h"
#include "vm.h"
#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>


static void print(const char** var) {
    MTR_LOG_WARN("%s", *var);
}

#define TRACE __attribute__((cleanup(print))) const char* hello = "hello\0"

int main(int argc, char* argv[])
{
    char* source = mtr_read_file(argv[1]);
    struct mtr_package* package = mtr_compile(source);
    if (NULL == package)
        return -1;

    struct mtr_vm vm;
    i32 result = mtr_execute(&vm, package);
    MTR_LOG_DEBUG("%i", result);

    mtr_delete_package(package);
    free(source);

    return result;
}
