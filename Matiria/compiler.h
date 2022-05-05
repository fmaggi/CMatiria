#ifndef MTR_COMPILER_H
#define MTR_COMPILER_H

#include "package.h"

#include "core/exitCode.h"
#include "core/types.h"

enum mtr_exit_code mtr_compile(const char* source, struct mtr_package* package);

#endif
