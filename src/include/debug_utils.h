/*
 * Debug Module - MIT-Compatible HTTP Client
 *
 * Provides logging and debug functionality with compile-time and runtime control.
 * Supports different log levels and can be completely disabled for release builds.
 */

#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H

#include <stdio.h>
#include "settings.h"

/* Debug levels */
typedef enum {
  DEBUG_LEVEL_ERROR = 0,  /* Errors only */
  DEBUG_LEVEL_WARN  = 1,  /* Warnings and errors */
  DEBUG_LEVEL_INFO  = 2,  /* Info, warnings, and errors */
  DEBUG_LEVEL_DEBUG = 3,  /* All messages */
  DEBUG_LEVEL_TRACE = 4   /* Extremely verbose */
} debug_level_t;

/* Function declarations */
int debug_init(int enabled);
void debug_cleanup(void);
void debug_set_level(debug_level_t level);
void debug_set_quiet(int quiet);
int debug_is_enabled(void);

/* Internal logging function */
void debug_log_internal(debug_level_t level, const char *file, int line,
                        const char *func, const char *format, ...);

/* Macros for different log levels - compile away if DEBUG_MODE is 0 */
#if DEBUG_ENABLED

#define DEBUG_ERROR(...) debug_log_internal(DEBUG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_WARN(...)  debug_log_internal(DEBUG_LEVEL_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_INFO(...)  debug_log_internal(DEBUG_LEVEL_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_LOG(...)   debug_log_internal(DEBUG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_TRACE(...) debug_log_internal(DEBUG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* Debug-only code blocks */
#define DEBUG_ONLY(code) do { code } while(0)

#else

/* No-op macros for release builds */
#define DEBUG_ERROR(...) do { } while(0)
#define DEBUG_WARN(...)  do { } while(0)
#define DEBUG_INFO(...)  do { } while(0)
#define DEBUG_LOG(...)   do { } while(0)
#define DEBUG_TRACE(...) do { } while(0)
#define DEBUG_ONLY(code) do { } while(0)

#endif /* DEBUG_ENABLED */

/* Always-available error logging (even in release builds) */
#define FATAL_ERROR(...) do { \
    fprintf(stderr, "FATAL: " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    fflush(stderr); \
} while(0)

/* Memory debugging helpers */
#if DEBUG_ENABLED
#define DEBUG_MALLOC(ptr, size) DEBUG_TRACE("malloc(%zu) = %p", (size_t)(size), (void*)(ptr))
#define DEBUG_FREE(ptr) DEBUG_TRACE("free(%p)", (void*)(ptr))
#define DEBUG_REALLOC(old_ptr, new_ptr, size) DEBUG_TRACE("realloc(%p, %zu) = %p", (void*)(old_ptr), (size_t)(size), (void*)(new_ptr))
#else
#define DEBUG_MALLOC(ptr, size) do { } while(0)
#define DEBUG_FREE(ptr) do { } while(0)
#define DEBUG_REALLOC(old_ptr, new_ptr, size) do { } while(0)
#endif

/* HTTP debugging helpers */
#if DEBUG_ENABLED
#define DEBUG_HTTP_REQUEST(method, url) DEBUG_INFO("HTTP %s: %s", (method), (url))
#define DEBUG_HTTP_RESPONSE(code, size) DEBUG_INFO("HTTP Response: %ld (%zu bytes)", (long)(code), (size_t)(size))
#define DEBUG_HTTP_ERROR(error) DEBUG_ERROR("HTTP Error: %s", (error))
#else
#define DEBUG_HTTP_REQUEST(method, url) do { } while(0)
#define DEBUG_HTTP_RESPONSE(code, size) do { } while(0)
#define DEBUG_HTTP_ERROR(error) do { } while(0)
#endif

/* Signal debugging helpers */
#if DEBUG_ENABLED
#define DEBUG_SIGNAL(signum) DEBUG_WARN("Received signal %d", (signum))
#else
#define DEBUG_SIGNAL(signum) do { } while(0)
#endif

#endif /* DEBUG_MODULE_H */