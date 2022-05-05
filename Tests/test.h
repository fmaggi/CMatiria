#ifndef MTR_TESTING
#error "This file should only be included in testing"
#endif

#ifndef MTR_TEST_H
#define MTR_TEST_H

#include "core/log.h"

#define STR(x) #x

static int tests = 0;
static int passed = 0;

#define TEST_CASE(name)                                        \
    static void name ## _test(int* checks, int* ok);           \
    static void name() {                                       \
        tests += 1;                                            \
        printf("==========================================\n");\
        printf(MTR_BOLD_DARK(MTR_WHITE)"Test: "MTR_RESET"%s\n", STR(name));                       \
        int checks = 0, ok = 0;                                \
        name ## _test(&checks, &ok);                           \
        if (ok == checks) {                                    \
            printf(MTR_BOLD_DARK(MTR_GREEN)"Passed\n"MTR_RESET); \
            passed++;                                          \
        }                                                      \
        printf("==========================================\n");\
    }                                                          \
    static void name ## _test(int* checks, int* ok)

#define CHECK(condition)                                        \
    do {                                                        \
        *checks += 1;                                           \
        if (!(condition)) {                                     \
            printf(MTR_BOLD_DARK(MTR_RED)"Failed"MTR_RESET" -> %s\n", STR(condition));\
        } else {                                                \
            *ok += 1;                                           \
        }                                                       \
    } while(0)

#define REPORT()                                                \
    do {                                                        \
        const char* ok = MTR_BOLD_DARK(MTR_GREEN)"OK"MTR_RESET; \
        const char* failed = MTR_BOLD_DARK(MTR_RED)"FAILED"MTR_RESET; \
        printf(MTR_BOLD_DARK(MTR_WHITE)"STATUS: %s\n"MTR_RESET, tests == passed ? ok : failed); \
    } while (0)

#endif
