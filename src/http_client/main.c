/*
 * MIT-Compatible HTTP Client - Main Entry Point
 *
 * This file contains only the main function and basic argument parsing.
 * All functionality is delegated to specialized modules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global configuration */
#include "global_config.h"

/* Module headers */
#include "settings.h"
#include "debug_module.h"
#include "memory_module.h"
#include "signals_module.h"
#include "http_module.h"
#include "config_module.h"

/* Program information */
static void print_version(void) {
  printf("%s v%s\n", PROJECT_NAME, PROJECT_VERSION);
  printf("Using %s (MIT-compatible)\n", TLS_LIBRARY);
  printf("License: %s\n", PROJECT_LICENSE);
}

/* Usage information */
static void print_usage(const char *program_name) {
  printf("Usage: %s [OPTIONS] <URL>\n\n", program_name);
  printf("Options:\n");
  printf("  -h, --help     Show this help message\n");
  printf("  -v, --version  Show version information\n");
  printf("  -d, --debug    Enable debug output\n");
  printf("  -q, --quiet    Disable all output except errors\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s https://www.google.com\n", program_name);
  printf("  %s --debug https://httpbin.org/get\n", program_name);
  printf("  %s --quiet http://httpbin.org/ip\n", program_name);
}

/* Argument parsing */
typedef struct {
  const char *url;
  int debug_enabled;
  int quiet_mode;
  int show_help;
  int show_version;
} arguments_t;

static int parse_arguments(int argc, char *argv[], arguments_t *args) {
  /* Initialize arguments */
  memset(args, 0, sizeof(arguments_t));

  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      args->show_help = 1;
    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
      args->show_version = 1;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      args->debug_enabled = 1;
    } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
      args->quiet_mode = 1;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      return -1;
    } else if (args->url == NULL) {
      args->url = argv[i];
    } else {
      fprintf(stderr, "Multiple URLs specified. Only one URL is allowed.\n");
      return -1;
    }
  }

  return 0;
}

/* Validate arguments */
static int validate_arguments(const arguments_t *args) {
  /* Check for help or version flags */
  if (args->show_help || args->show_version) {
    return 0;
  }

  /* URL is required */
  if (args->url == NULL) {
    fprintf(stderr, "Error: URL is required\n");
    return -1;
  }

  /* Basic URL validation */
  if (strncmp(args->url, "http://", 7) != 0 &&
      strncmp(args->url, "https://", 8) != 0) {
    fprintf(stderr, "Error: URL must start with http:// or https://\n");
    return -1;
  }

  /* Conflicting flags */
  if (args->debug_enabled && args->quiet_mode) {
    fprintf(stderr, "Error: --debug and --quiet flags are mutually exclusive\n");
    return -1;
  }

  return 0;
}

/* Initialize all modules */
static int initialize_modules(const arguments_t *args) {
  /* Initialize debug module first */
  if (debug_init(args->debug_enabled || DEBUG_ENABLED) != 0) {
    fprintf(stderr, "Failed to initialize debug module\n");
    return -1;
  }

  DEBUG_LOG("Initializing modules...");

  /* Initialize memory module */
  if (memory_init() != 0) {
    DEBUG_ERROR("Failed to initialize memory module");
    return -1;
  }

  /* Initialize signal handling */
  if (signals_init() != 0) {
    DEBUG_ERROR("Failed to initialize signals module");
    memory_cleanup();
    return -1;
  }

  /* Initialize HTTP module */
  if (http_init() != 0) {
    DEBUG_ERROR("Failed to initialize HTTP module");
    signals_cleanup();
    memory_cleanup();
    return -1;
  }

  DEBUG_LOG("All modules initialized successfully");
  return 0;
}

/* Cleanup all modules */
static void cleanup_modules(void) {
  DEBUG_LOG("Cleaning up modules...");

  http_cleanup();
  signals_cleanup();
  memory_cleanup();
  debug_cleanup();
}

/* Main function */
int main(int argc, char *argv[]) {
  arguments_t args;
  int result = EXIT_SUCCESS;

  /* Parse command line arguments */
  if (parse_arguments(argc, argv, &args) != 0) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  /* Handle help and version flags */
  if (args.show_help) {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  if (args.show_version) {
    print_version();
    return EXIT_SUCCESS;
  }

  /* Validate arguments */
  if (validate_arguments(&args) != 0) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  /* Initialize all modules */
  if (initialize_modules(&args) != 0) {
    return EXIT_FAILURE;
  }

  /* Set quiet mode if requested */
  if (args.quiet_mode) {
    debug_set_quiet(1);
  }

  /* Perform HTTP request */
  DEBUG_LOG("Making HTTP request to: %s", args.url);

  if (http_request(args.url) != 0) {
    DEBUG_ERROR("HTTP request failed");
    result = EXIT_FAILURE;
  } else {
    DEBUG_LOG("HTTP request completed successfully");
  }

  /* Cleanup and exit */
  cleanup_modules();
  return result;
}
