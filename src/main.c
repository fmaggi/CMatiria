#include <stdio.h>

#include "base/file.h"
#include "base/log.h"

int main(int argc, char* argv[])
{
    char* test = read_file(argv[1]);
    printf("Hello %s!\n", test);
    LOG_INFO("Hello");
    LOG_TRACE("Hello");
    LOG_ERROR("Hello");
    LOG_WARN("Hello");
}
