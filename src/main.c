// this file is just temporary (probably :) )

#include "compiler.h"
#include "core/file.h"

#include <stdlib.h>

int main(int argc, char* argv[])
{
    char* source = mtr_read_file(argv[1]);
    mtr_compile(source);
    free(source);
}
