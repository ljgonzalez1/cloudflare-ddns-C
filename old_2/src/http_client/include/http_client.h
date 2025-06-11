/*
 * HTTP Client - Main Header File
 *
 * This is the main header that includes all necessary modules.
 * Only this header should be included by main.c and other client code.
 */

#pragma once

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

/* Standard C library headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* System headers */
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Project configuration */
#include "settings.h"
#include "messages.h"

/* Debug utilities */
#include "../../include/debug_utils.h"

/* Memory management */
#include "memory_management.h"

/* Signal processing */
#include "signal_processing.h"

/* HTTP client functionality */
#include "http_module.h"

/* LibreSSL headers */
#include "../lib/LibreSSL/include/openssl/ssl.h"
#include "../lib/LibreSSL/include/openssl/err.h"
#include "../lib/LibreSSL/include/openssl/bio.h"
#include "../lib/LibreSSL/include/openssl/x509.h"
#include "../lib/LibreSSL/include/openssl/x509v3.h"
#include "../lib/LibreSSL/include/openssl/rand.h"

/* HTTP Methods */
typedef enum {
  HTTP_METHOD_GET = 0,
  HTTP_METHOD_POST,
  HTTP_METHOD_PUT,
  HTTP_METHOD_DELETE,
  HTTP_METHOD_HEAD,
  HTTP_METHOD_OPTIONS,
  HTTP_METHOD_PATCH
} http_method_t;

/* HTTP Response Structure */
typedef struct {
  int status_code;
  char *status_message;
  char *headers;
  char *body;
  size_t body_size;
  size_t header_size;
} http_response_t;

/* HTTP Request Structure */
typedef struct {
  http_method_t method;
  char *url;
  char *headers;
  char *body;
  size_t body_size;
  bool verify_ssl;
  int timeout;
  int max_redirects;
} http_request_t;

/* URL Components */
typedef struct {
  char *scheme;     /* http or https */
  char *hostname;   /* domain name or IP */
  int port;         /* port number */
  char *path;       /* path component */
  char *query;      /* query string */
  bool is_ssl;      /* true if https */
} url_components_t;

/* Program Arguments */
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

/* Main HTTP client functions */
int http_client_init(void);
void http_client_cleanup(void);
int http_client_request(const http_request_t *request, http_response_t *response);

/* HTTP utility functions */
int http_parse_url(const char *url, url_components_t *components);
void http_free_url_components(url_components_t *components);
void http_free_response(http_response_t *response);
http_request_t* http_create_request(const char *url, http_method_t method);
void http_free_request(http_request_t *request);
int http_add_header(http_request_t *request, const char *header);
int http_set_body(http_request_t *request, const char *body, size_t size);

/* Method conversion utilities */
const char* http_method_to_string(http_method_t method);
http_method_t http_string_to_method(const char *method_str);

/* Response processing */
int http_print_response(const http_response_t *response, FILE *output);
int http_save_response_to_file(const http_response_t *response, const char *filename);

/* Argument parsing */
int parse_arguments(int argc, char *argv[], program_args_t *args);
int validate_arguments(const program_args_t *args);
void print_usage(const char *program_name);
void print_version(void);

/* Module initialization helpers */
int initialize_all_modules(const program_args_t *args);
void cleanup_all_modules(void);

#endif /* HTTP_CLIENT_H */
