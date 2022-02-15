#ifndef MTR_LOG_H
#define MTR_LOG_H

#include <stdio.h>

#define MTR_PROFILE_FUNC() (printf("[Function call] %s\n",__func__))

#define MTR_LOG(...)       (printf(__VA_ARGS__), printf("\n"))

#define MTR_LOG_INFO(...)  (printf("Info: "), MTR_LOG(__VA_ARGS__))                      // white
#define MTR_LOG_TRACE(...) (printf("\033[0;32mTrace: \033[0m"), MTR_LOG(__VA_ARGS__))  // green
#define MTR_LOG_WARN(...)  (printf("\033[0;33mWarning: \033[0m"), MTR_LOG(__VA_ARGS__)) // yellow
#define MTR_LOG_ERROR(...) (printf("\033[0;31mError: \033[0m"), MTR_LOG(__VA_ARGS__))   // red

#ifdef DEBUG
    #define MTR_LOG_INFO_DEBUG(...)   (MTR_LOG_INFO(__VA_ARGS__))
    #define MTR_LOG_TRACE_DEBUG(...)  (MTR_LOG_TRACE(__VA_ARGS__))
    #define MTR_LOG_WARN_DEBUG(...)   (MTR_LOG_WARN(__VA_ARGS__))
    #define MTR_LOG_ERROR_DEBUG(...)  (MTR_LOG_ERROR(__VA_ARGS__))

    #define MTR_ASSERT(x, m) if (!(x)) { MTR_LOG_ERROR((m)); exit(-1); }

    #define IMPLEMENT MTR_LOG_WARN("%s function needs to be implemented!", __func__);
#else
    #define MTR_LOG_INFO_DEBUG(...)
    #define MTR_LOG_TRACE_DEBUG(...)
    #define MTR_LOG_WARN_DEBUG(...)
    #define MTR_LOG_ERROR_DEBUG(...)

    #define MTR_ASSERT(x, m)

    #define IMPLEMENT
#endif

#endif
