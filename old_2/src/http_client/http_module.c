/*
 * HTTP Module - MIT-Compatible HTTP Client
 *
 * Provides HTTP/HTTPS client functionality using LibreSSL.
 * Supports GET, POST, PUT, DELETE methods with SSL/TLS.
 */

#pragma once

#ifndef HTTP_MODULE_H
#define HTTP_MODULE_H

#include "settings.h"
#include <stddef.h>
#include <stdio.h>

/* Forward declarations */
typedef struct http_request http_request_t;
typedef struct http_response http_response_t;
typedef struct url_components url_components_t;

/* HTTP Module Functions */
int http_init(void);
void http_cleanup(void);

/* HTTP Request Functions */
int http_request(const char *url);
int http_request_advanced(const http_request_t *request, http_response_t *response);

/* URL Parsing */
int http_parse_url(const char *url, url_components_t *components);
void http_free_url_components(url_components_t *components);

/* Request/Response Management */
http_request_t* http_create_request(const char *url, int method);
void http_free_request(http_request_t *request);
void http_free_response(http_response_t *response);

/* Header Management */
int http_add_header(http_request_t *request, const char *header);
int http_set_body(http_request_t *request, const char *body, size_t size);

/* Response Processing */
int http_print_response(const http_response_t *response, FILE *output);
int http_save_response_to_file(const http_response_t *response, const char *filename);

/* Utility Functions */
const char* http_method_to_string(int method);
int http_string_to_method(const char *method_str);

/* SSL/TLS Functions */
int http_ssl_init(void);
void http_ssl_cleanup(void);

#endif /* HTTP_MODULE_H */
