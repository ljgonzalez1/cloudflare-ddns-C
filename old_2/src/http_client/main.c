/*
 * MIT-Compatible HTTP Client - Main Entry Point
 *
 * This file contains only the main function and basic argument parsing.
 * All functionality is delegated to specialized modules.
 */

#include "include/http_client.h"

/* Program information */
static void print_version(void) {
  printf(MSG_VERSION_INFO, PROJECT_NAME, PROJECT_VERSION, TLS_LIBRARY, PROJECT_LICENSE);
}

/* Usage information */
static void print_usage(const char *program_name) {
  printf(MSG_USAGE_HEADER, program_name);
  printf(MSG_USAGE_OPTIONS);
  printf(MSG_USAGE_EXAMPLES, program_name, program_name, program_name, program_name);
}

/* Argument parsing */
typedef struct {
  const char *url;
  const char *method;
  const char *output_file;
  const char **headers;
  int header_count;
  const char *body_data;
  bool debug_enabled;
  bool quiet_mode;
  bool show_help;
  bool show_version;
  bool insecure;
} program_args_t;

static int parse_arguments(int argc, char *argv[], program_args_t *args) {
  /* Initialize arguments */
  memset(args, 0, sizeof(program_args_t));
  args->method = "GET"; /* Default method */

  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      args->show_help = true;
    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
      args->show_version = true;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      args->debug_enabled = true;
    } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
      args->quiet_mode = true;
    } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--insecure") == 0) {
      args->insecure = true;
    } else if ((strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) && i + 1 < argc) {
      args->method = argv[++i];
    } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
      args->output_file = argv[++i];
    } else if ((strcmp(argv[i], "-D") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
      args->body_data = argv[++i];
    } else if (argv[i][0] == '-') {
      fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_UNKNOWN_OPTION, argv[i]);
      return ERROR_INVALID_ARGS;
    } else if (args->url == NULL) {
      args->url = argv[i];
    } else {
      fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_MULTIPLE_URLS);
      return ERROR_INVALID_ARGS;
    }
  }

  return SUCCESS;
}

/* Validate arguments */
static int validate_arguments(const program_args_t *args) {
  /* Check for help or version flags */
  if (args->show_help || args->show_version) {
    return SUCCESS;
  }

  /* URL is required */
  if (args->url == NULL) {
    fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_URL_REQUIRED);
    return ERROR_INVALID_ARGS;
  }

  /* Basic URL validation */
  if (strncmp(args->url, "http://", 7) != 0 &&
      strncmp(args->url, "https://", 8) != 0) {
    fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_INVALID_URL);
    return ERROR_INVALID_ARGS;
  }

  /* Conflicting flags */
  if (args->debug_enabled && args->quiet_mode) {
    fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_CONFLICTING_FLAGS);
    return ERROR_INVALID_ARGS;
  }

  /* Validate HTTP method */
  if (http_string_to_method(args->method) < 0) {
    fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_INVALID_METHOD, args->method);
    return ERROR_INVALID_ARGS;
  }

  return SUCCESS;
}

/* Initialize all modules */
static int initialize_all_modules(const program_args_t *args) {
  /* Initialize debug module first */
  if (debug_init(args->debug_enabled || DEBUG_ENABLED) != 0) {
    fprintf(stderr, MSG_ERROR_PREFIX MSG_ERROR_INIT_FAILED, "debug");
    return ERROR_GENERAL;
  }

  DEBUG_LOG(MSG_INFO_INITIALIZING);

  /* Initialize memory module */
  if (memory_init() != 0) {
    DEBUG_ERROR(MSG_ERROR_INIT_FAILED, "memory");
    return ERROR_MEMORY;
  }

  /* Initialize signal handling */
  if (signals_init() != 0) {
    DEBUG_ERROR(MSG_ERROR_INIT_FAILED, "signals");
    memory_cleanup();
    return ERROR_GENERAL;
  }

  /* Initialize HTTP module */
  if (http_init() != 0) {
    DEBUG_ERROR(MSG_ERROR_INIT_FAILED, "HTTP");
    signals_cleanup();
    memory_cleanup();
    return ERROR_HTTP;
  }

  DEBUG_LOG("All modules initialized successfully");
  return SUCCESS;
}

/* Cleanup all modules */
static void cleanup_all_modules(void) {
  DEBUG_LOG(MSG_INFO_CLEANING_UP);

  http_cleanup();
  signals_cleanup();
  memory_cleanup();
  debug_cleanup();
}

/* Signal cleanup callback */
static void signal_cleanup_callback(void) {
  cleanup_all_modules();
}

/* Perform HTTP request */
static int perform_http_request(const program_args_t *args) {
  /* Create HTTP request */
  http_method_t method = (http_method_t)http_string_to_method(args->method);
  http_request_t *request = http_create_request(args->url, method);
  if (!request) {
    DEBUG_ERROR(MSG_ERROR_MEMORY_ALLOCATION);
    return ERROR_MEMORY;
  }

  /* Set request options */
  request->verify_ssl = !args->insecure;

  /* Add body data if provided */
  if (args->body_data) {
    if (http_set_body(request, args->body_data, strlen(args->body_data)) != SUCCESS) {
      DEBUG_ERROR("Failed to set request body");
      http_free_request(request);
      return ERROR_MEMORY;
    }
  }

  /* Show insecure warning */
  if (args->insecure) {
    DEBUG_WARN(MSG_WARN_INSECURE_CONNECTION);
  }

  DEBUG_LOG(MSG_INFO_MAKING_REQUEST, args->url);

  /* Perform request */
  http_response_t response = {0};
  int result = http_request_advanced(request, &response);

  if (result == SUCCESS) {
    DEBUG_LOG(MSG_SUCCESS_REQUEST_COMPLETED);

    /* Output response */
    FILE *output = stdout;
    if (args->output_file) {
      output = fopen(args->output_file, "w");
      if (!output) {
        DEBUG_ERROR(MSG_ERROR_FILE_OPEN, args->output_file);
        output = stdout;
      }
    }

    http_print_response(&response, output);

    if (output != stdout) {
      fclose(output);
    }
  } else {
    DEBUG_ERROR(MSG_ERROR_HTTP_REQUEST_FAILED);
  }

  /* Cleanup */
  http_free_request(request);
  http_free_response(&response);

  return result;
}

/* Main function */
int main(int argc, char *argv[]) {
  program_args_t args;
  int result = EXIT_SUCCESS;

  /* Parse command line arguments */
  if (parse_arguments(argc, argv, &args) != SUCCESS) {
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
  if (validate_arguments(&args) != SUCCESS) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  /* Initialize all modules */
  if (initialize_all_modules(&args) != SUCCESS) {
    return EXIT_FAILURE;
  }

  /* Register signal cleanup callback */
  signals_register_cleanup(signal_cleanup_callback);

  /* Set quiet mode if requested */
  if (args.quiet_mode) {
    debug_set_quiet(1);
  }

  /* Perform HTTP request */
  if (perform_http_request(&args) != SUCCESS) {
    result = EXIT_FAILURE;
  }

  /* Cleanup and exit */
  cleanup_all_modules();
  return result;
}