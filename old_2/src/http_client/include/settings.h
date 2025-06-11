/*
 * Global Settings - MIT-Compatible HTTP Client
 *
 * Central configuration file for all build-time and runtime settings.
 * Controls debug levels, memory tracking, signal handling, and HTTP behavior.
 */

#pragma once

#ifndef SETTINGS_H
#define SETTINGS_H

/* Project Information */
#define PROJECT_NAME "http-client"
#define PROJECT_VERSION "1.0.0"
#define PROJECT_LICENSE "MIT"
#define TLS_LIBRARY "LibreSSL"

/* Build Configuration */
#ifdef DEBUG
#define DEBUG_ENABLED 1
    #define DEBUG_LEVEL DEBUG_LEVEL_DEBUG
    #define MEMORY_TRACKING_ENABLED 1
    #define SIGNAL_HANDLING_ENABLED 1
#else
#define DEBUG_ENABLED 0
#define DEBUG_LEVEL DEBUG_LEVEL_ERROR
#define MEMORY_TRACKING_ENABLED 0
#define SIGNAL_HANDLING_ENABLED 1
#endif

/* Debug Configuration */
#define DEBUG_PREFIX "HTTP-CLIENT"
#define MAX_DEBUG_LINE_LENGTH 1024

/* Memory Management Settings */
#define MEMORY_MAX_RETRIES 3
#define MEMORY_RETRY_DELAY_US 1000
#define MEMORY_ALIGNMENT 8
#define MEMORY_POOL_SIZE 4096

/* HTTP Client Settings */
#define HTTP_MAX_URL_LENGTH 2048
#define HTTP_MAX_HEADER_SIZE 8192
#define HTTP_MAX_BODY_SIZE (1024 * 1024) /* 1MB */
#define HTTP_CONNECT_TIMEOUT_SECONDS 30
#define HTTP_READ_TIMEOUT_SECONDS 60
#define HTTP_USER_AGENT PROJECT_NAME "/" PROJECT_VERSION
#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
#define HTTP_MAX_REDIRECTS 5

/* TLS/SSL Settings */
#define TLS_VERIFY_PEER 1
#define TLS_VERIFY_HOST 1
#define TLS_MIN_VERSION TLS1_2_VERSION
#define TLS_MAX_VERSION TLS1_3_VERSION

/* Network Buffer Sizes */
#define NETWORK_BUFFER_SIZE 8192
#define NETWORK_READ_CHUNK_SIZE 4096

/* Signal Handling Configuration */
#define MAX_SIGNAL_CALLBACKS 8

/* Platform Detection */
#ifdef _WIN32
#define PLATFORM_WINDOWS 1
    #define PLATFORM_UNIX 0
#else
#define PLATFORM_WINDOWS 0
#define PLATFORM_UNIX 1
#endif

/* Compiler Attributes */
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#define NORETURN __attribute__((noreturn))
#define PACKED __attribute__((packed))
#else
#define UNUSED
    #define NORETURN
    #define PACKED
#endif

/* Error Codes */
#define SUCCESS 0
#define ERROR_GENERAL -1
#define ERROR_MEMORY -2
#define ERROR_NETWORK -3
#define ERROR_TLS -4
#define ERROR_HTTP -5
#define ERROR_INVALID_ARGS -6
#define ERROR_TIMEOUT -7

#endif /* SETTINGS_H */
