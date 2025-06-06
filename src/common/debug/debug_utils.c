/*
 * Debug Module Implementation - MIT-Compatible HTTP Client
 *
 * Provides logging functionality with different levels and runtime control.
 * Can be completely compiled out for release builds.
 */

#include "debug_module.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* Module state */
static struct {
  int initialized;
  int enabled;
  int quiet_mode;
  debug_level_t level;
  FILE *output;
} debug_state = {
    .initialized = 0,
    .enabled = 0,
    .quiet_mode = 0,
    .level = DEBUG_LEVEL_INFO,
    .output = NULL
};

/* Color codes for terminal output */
static const char* LEVEL_COLORS[] = {
    "\033[0;31m",  /* ERROR - Red */
    "\033[0;33m",  /* WARN  - Yellow */
    "\033[0;36m",  /* INFO  - Cyan */
    "\033[0;37m",  /* DEBUG - White */
    "\033[0;90m"   /* TRACE - Dark Gray */
};

static const char* LEVEL_NAMES[] = {
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG",
    "TRACE"
};

static const char* COLOR_RESET = "\033[0m";

/* Get current timestamp string */
static void get_timestamp(char *buffer, size_t size) {
  struct timeval tv;
  struct tm *tm_info;

  gettimeofday(&tv, NULL);
  tm_info = localtime(&tv.tv_sec);

  snprintf(buffer, size, "%02d:%02d:%02d.%03ld",
           tm_info->tm_hour,
           tm_info->tm_min,
           tm_info->tm_sec,
           tv.tv_usec / 1000);
}

/* Extract filename from full path */
static const char* extract_filename(const char *filepath) {
  const char *filename = strrchr(filepath, '/');
  return filename ? filename + 1 : filepath;
}

/* Initialize debug module */
int debug_init(int enabled) {
  if (debug_state.initialized) {
    return 0; /* Already initialized */
  }

  debug_state.enabled = enabled;
  debug_state.level = DEBUG_LEVEL;
  debug_state.output = stderr; /* Default to stderr */
  debug_state.quiet_mode = 0;
  debug_state.initialized = 1;

#if DEBUG_ENABLED
  if (enabled) {
        DEBUG_LOG("Debug module initialized (level: %d)", debug_state.level);
    }
#endif

  return 0;
}

/* Cleanup debug module */
void debug_cleanup(void) {
  if (!debug_state.initialized) {
    return;
  }

#if DEBUG_ENABLED
  if (debug_state.enabled) {
        DEBUG_LOG("Debug module cleanup");
    }
#endif

  /* Flush output */
  if (debug_state.output && debug_state.output != stderr && debug_state.output != stdout) {
    fclose(debug_state.output);
  }

  debug_state.initialized = 0;
  debug_state.enabled = 0;
  debug_state.output = NULL;
}

/* Set debug level */
void debug_set_level(debug_level_t level) {
  if (!debug_state.initialized) {
    return;
  }

  debug_state.level = level;

#if DEBUG_ENABLED
  if (debug_state.enabled) {
        DEBUG_LOG("Debug level set to: %s", LEVEL_NAMES[level]);
    }
#endif
}

/* Set quiet mode */
void debug_set_quiet(int quiet) {
  if (!debug_state.initialized) {
    return;
  }

  debug_state.quiet_mode = quiet;
}

/* Check if debug is enabled */
int debug_is_enabled(void) {
  return debug_state.initialized && debug_state.enabled && !debug_state.quiet_mode;
}

/* Internal logging function */
void debug_log_internal(debug_level_t level, const char *file, int line,
                        const char *func, const char *format, ...) {
#if !DEBUG_ENABLED
  /* Compile out completely in release builds */
  (void)level; (void)file; (void)line; (void)func; (void)format;
  return;
#else

  /* Check if module is initialized and enabled */
    if (!debug_state.initialized || !debug_state.enabled) {
        return;
    }

    /* Check quiet mode */
    if (debug_state.quiet_mode) {
        return;
    }

    /* Check log level */
    if (level > debug_state.level) {
        return;
    }

    /* Get output stream */
    FILE *output = debug_state.output ? debug_state.output : stderr;

    /* Get timestamp */
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Extract filename */
    const char *filename = extract_filename(file);

    /* Check if we should use colors (only for terminals) */
    int use_colors = isatty(fileno(output));

    /* Print log header */
    if (use_colors) {
        fprintf(output, "%s[%s %s%s%s %s:%d %s()] ",
                LEVEL_COLORS[level],
                timestamp,
                DEBUG_PREFIX,
                COLOR_RESET,
                LEVEL_COLORS[level],
                filename,
                line,
                func);
    } else {
        fprintf(output, "[%s %s %s %s:%d %s()] ",
                timestamp,
                LEVEL_NAMES[level],
                DEBUG_PREFIX,
                filename,
                line,
                func);
    }

    /* Print the actual message */
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);

    /* Reset color and add newline */
    if (use_colors) {
        fprintf(output, "%s\n", COLOR_RESET);
    } else {
        fprintf(output, "\n");
    }

    /* Flush immediately for debugging */
    fflush(output);

#endif /* DEBUG_ENABLED */
}