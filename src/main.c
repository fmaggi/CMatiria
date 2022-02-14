#include <stdio.h>

#include "base/file.h"
#include "base/log.h"

#include "scanner.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char* argv[])
{
    struct mtr_file file = mtr_read_file(argv[1]);

    struct mtr_scanner scanner = {
        .current = file.bytes,
        .source = file.bytes,
        .start = file.bytes
    };

    while (*scanner.current != '\n') {
        struct mtr_token t = next_token(&scanner);
        char buf[5];
        memcpy(buf, t.start, sizeof(char) * 2);
        buf[4] = 0;
        MTR_LOG_INFO("%s", buf);
    }

    free(file.bytes);
}
