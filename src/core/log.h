#ifndef _MTR_LOG_H
#define _MTR_LOG_H

#include <stdio.h>
#include <stdlib.h>

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

#define MTR_LOG(...)       (printf(__VA_ARGS__), putc('\n', stdout))

#define MTR_LOG_INFO(...)  (printf((MTR_INFO_PRE) ), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_TRACE(...) (printf((MTR_TRACE_PRE)), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_WARN(...)  (printf((MTR_WARN_PRE) ), MTR_LOG(__VA_ARGS__))
#define MTR_LOG_ERROR(...) (printf((MTR_ERROR_PRE)), MTR_LOG(__VA_ARGS__))

#define MTR_PROFILE_FUNC() (printf("%s[Function call]%s %s\n", MTR_BOLD_DARK(MTR_WHITE), MTR_RESET, __func__))

#ifndef NDEBUG

    #define MTR_LOG_DEBUG(...)  (printf( (MTR_DEBUG_PRE)), MTR_LOG(__VA_ARGS__))
    #define MTR_PRINT_DEBUG(...) (printf(__VA_ARGS__))

    #define MTR_ASSERT(x, m) if (!(x)) { MTR_PRINT_DEBUG(MTR_BOLD_DARK(MTR_WHITE)"[%s]: "MTR_RESET, __func__); MTR_LOG_ERROR((m)); exit(-1); }

    #define IMPLEMENT MTR_LOG_WARN("%s function needs to be implemented!", __func__);
#else

    #define MTR_LOG_DEBUG(...)
    #define MTR_PRINT_DEBUG(...)

    #define MTR_ASSERT(x, m)

    #define IMPLEMENT

#endif

#endif
