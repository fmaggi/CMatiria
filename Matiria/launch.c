#include "launch.h"

#include "core/file.h"
#include "compiler.h"
#include "package.h"
#include "runtime/engine.h"
#include "stl/mtr_stdlib.h"

#include <stdlib.h>

enum mtr_exit_code mtr_launch(const char* path) {
    char* source = mtr_read_file(path);
    if (!source) {
        return MTR_FILE_ERROR;
    }

    enum mtr_exit_code ec = MTR_OK;

    struct mtr_package package;
    mtr_init_package(&package);

    ec = mtr_compile(source, &package);
    if (ec != MTR_OK) {
        goto end;
    }

    mtr_add_io(&package);

    struct mtr_engine* engine = malloc(sizeof(*engine));
    i32 result = mtr_execute(engine, &package);
    free(engine);

end:
    mtr_delete_package(&package);
    free(source);
    return ec;
}
