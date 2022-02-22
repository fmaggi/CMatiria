#ifndef _MTR_LOG_H
#define _MTR_LOG_H

#include <stdio.h>
#include <stdlib.h>

extern FILE* mtr_output_file;

#define MTR_PRE    "\033["
#define MTR_SUF    "m"

#define MTR_RESET  MTR_PRE "0" MTR_SUF
#define MTR_BOLD   MTR_PRE "1" MTR_SUF

#define MTR_WHITE  "37"
#define MTR_RED    "31"
#define MTR_GREEN  "32"
#define MTR_YELLOW "33"
#define MTR_BLUE   "34"

#define MTR_DARK(color)  MTR_PRE "0;" color MTR_SUF
#define MTR_LIGHT(color) MTR_PRE "1;" color MTR_SUF

#define MTR_BOLD_DARK(color)  MTR_DARK(color) MTR_BOLD
#define MTR_BOLD_LIGHT(color) MTR_LIGHT(color) MTR_BOLD

#define MTR_INFO_PRE  MTR_BOLD_DARK(MTR_WHITE) "Info: " MTR_RESET
#define MTR_TRACE_PRE MTR_BOLD_DARK(MTR_GREEN) "Trace: " MTR_RESET
#define MTR_WARN_PRE  MTR_BOLD_DARK(MTR_YELLOW) "Warning: " MTR_RESET
#define MTR_ERROR_PRE MTR_BOLD_DARK(MTR_RED) "Error: " MTR_RESET
#define MTR_DEBUG_PRE  MTR_BOLD_DARK(MTR_BLUE) "Debug: " MTR_RESET

#define MTR_LOG(...)       (fprintf(mtr_output_file, __VA_ARGS__), putc('\n', mtr_output_file))

#define MTR_LOG_INFO(...)  (fprintf(mtr_output_file, (MTR_INFO_PRE) ), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_TRACE(...) (fprintf(mtr_output_file, (MTR_TRACE_PRE)), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_WARN(...)  (fprintf(mtr_output_file, (MTR_WARN_PRE) ), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_ERROR(...) (fprintf(mtr_output_file, (MTR_ERROR_PRE)), MTR_LOG(__VA_ARGS__))

#define MTR_PROFILE_FUNC() (fprintf(mtr_output_file, "%s[Function call]%s %s\n", MTR_BOLD_DARK(MTR_WHITE), MTR_RESET, __func__))

#ifndef NDEBUG

    #define MTR_LOG_DEBUG(...)  (fprintf(mtr_output_file, (MTR_DEBUG_PRE)), MTR_LOG(__VA_ARGS__))
    #define MTR_PRINT_DEBUG(...) (fprintf(mtr_output_file, __VA_ARGS__))

    #define MTR_ASSERT(x, m) if (!(x)) { MTR_PRINT_DEBUG(MTR_BOLD_DARK(MTR_WHITE)"[%s]: "MTR_RESET, __func__); MTR_LOG_ERROR((m)); exit(-1); }

    #define IMPLEMENT MTR_LOG_WARN("%s function needs to be implemented!", __func__);
#else

    #define MTR_LOG_DEBUG(...)
    #define MTR_PRINT_DEBUG(...)

    #define MTR_ASSERT(x, m)

    #define IMPLEMENT

#endif

#endif
