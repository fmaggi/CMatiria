#ifndef MTR_EXIT_CODE_H
#define MTR_EXIT_CODE_H

enum mtr_exit_code {
    MTR_OK,
    MTR_FILE_ERROR,
    MTR_PARSER_ERROR,
    MTR_TYPE_ERROR,
    MTR_SCOPE_ERROR,
    MTR_COMPILER_ERROR,
    MTR_RUNTIME_ERROR
};

#endif
