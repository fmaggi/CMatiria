#ifndef LOG_H
#define LOG_H

#include "stdio.h"

#define PROFILE_FUNC() (printf("[Function call] %s\n",__func__))

#define LOG(...)       (printf(__VA_ARGS__), printf("\n"))

#define LOG_INFO(...)  (printf("Info: "), LOG(__VA_ARGS__))                      // white
#define LOG_TRACE(...) (printf("\033[0;32mTrace: \033[0m"), LOG(__VA_ARGS__))  // green
#define LOG_WARN(...)  (printf("\033[0;33mWarning: \033[0m"), LOG(__VA_ARGS__)) // yellow
#define LOG_ERROR(...) (printf("\033[0;31mError: \033[0m"), LOG(__VA_ARGS__))   // red

#ifdef DEBUG
    #define LOG_INFO_DEBUG(...)   (LOG_INFO(__VA_ARGS__))
    #define LOG_TRACE_DEBUG(...)  (LOG_TRACE(__VA_ARGS__))
    #define LOG_WARN_DEBUG(...)   (LOG_WARN(__VA_ARGS__))
    #define LOG_ERROR_DEBUG(...)  (LOG_ERROR(__VA_ARGS__))

    #define ASSERT(x, m) if (!(x)) { LOG_ERROR((m)); exit(-1); }
#else
    #define LOG_INFO_DEBUG(...)
    #define LOG_TRACE_DEBUG(...)
    #define LOG_WARN_DEBUG(...)
    #define LOG_ERROR_DEBUG(...)

    #define ASSERT(x, m)
#endif

#endif
