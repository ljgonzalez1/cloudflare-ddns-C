/*
 * System Messages - MIT-Compatible HTTP Client
 *
 * Centralized message definitions for consistent user interface.
 * All user-facing strings should be defined here.
 */

#pragma once

#ifndef MESSAGES_H
#define MESSAGES_H

/* Version and Help Messages */
#define MSG_VERSION_INFO "%s v%s\nUsing %s (MIT-compatible)\nLicense: %s\n"
#define MSG_USAGE_HEADER "Usage: %s [OPTIONS] <URL>\n\n"
#define MSG_USAGE_OPTIONS "Options:\n" \
    "  -h, --help     Show this help message\n" \
    "  -v, --version  Show version information\n" \
    "  -d, --debug    Enable debug output\n" \
    "  -q, --quiet    Disable all output except errors\n" \
    "  -X, --request  HTTP method (GET, POST, PUT, DELETE)\n" \
    "  -H, --header   Add custom header (can be used multiple times)\n" \
    "  -D, --data     Request body data (for POST/PUT)\n" \
    "  -o, --output   Output file (default: stdout)\n" \
    "  -k, --insecure Disable SSL certificate verification\n\n"
#define MSG_USAGE_EXAMPLES "Examples:\n" \
    "  %s https://www.google.com\n" \
    "  %s --debug https://httpbin.org/get\n" \
    "  %s --quiet http://httpbin.org/ip\n" \
    "  %s -X POST -H \"Content-Type: application/json\" -D '{\"test\":\"data\"}' https://httpbin.org/post\n"

/* Error Messages */
#define MSG_ERROR_PREFIX "Error: "
#define MSG_ERROR_UNKNOWN_OPTION "Unknown option: %s\n"
#define MSG_ERROR_MULTIPLE_URLS "Multiple URLs specified. Only one URL is allowed.\n"
#define MSG_ERROR_URL_REQUIRED "URL is required\n"
#define MSG_ERROR_INVALID_URL "URL must start with http:// or https://\n"
#define MSG_ERROR_CONFLICTING_FLAGS "--debug and --quiet flags are mutually exclusive\n"
#define MSG_ERROR_INIT_FAILED "Failed to initialize %s module\n"
#define MSG_ERROR_HTTP_REQUEST_FAILED "HTTP request failed\n"
#define MSG_ERROR_MEMORY_ALLOCATION "Memory allocation failed\n"
#define MSG_ERROR_FILE_OPEN "Failed to open file: %s\n"
#define MSG_ERROR_INVALID_METHOD "Invalid HTTP method: %s\n"

/* Success Messages */
#define MSG_SUCCESS_REQUEST_COMPLETED "HTTP request completed successfully\n"
#define MSG_SUCCESS_MODULE_INIT "%s module initialized\n"
#define MSG_SUCCESS_CLEANUP_COMPLETED "Cleanup completed\n"

/* Warning Messages */
#define MSG_WARN_PREFIX "Warning: "
#define MSG_WARN_MEMORY_LEAKS "Memory leaks detected: %d blocks\n"
#define MSG_WARN_SIGNAL_RECEIVED "Received signal %d, cleaning up...\n"
#define MSG_WARN_INSECURE_CONNECTION "Using insecure connection (SSL verification disabled)\n"
#define MSG_WARN_REDIRECT_LIMIT "Maximum redirects reached (%d)\n"

/* Info Messages */
#define MSG_INFO_INITIALIZING "Initializing modules...\n"
#define MSG_INFO_CLEANING_UP "Cleaning up modules...\n"
#define MSG_INFO_MAKING_REQUEST "Making HTTP request to: %s\n"
#define MSG_INFO_CONNECTING "Connecting to %s:%d\n"
#define MSG_INFO_TLS_HANDSHAKE "Performing TLS handshake\n"
#define MSG_INFO_SENDING_REQUEST "Sending request\n"
#define MSG_INFO_RECEIVING_RESPONSE "Receiving response\n"
#define MSG_INFO_RESPONSE_STATUS "Response: %d %s\n"
#define MSG_INFO_RESPONSE_SIZE "Response size: %zu bytes\n"
#define MSG_INFO_REDIRECT_FOLLOWING "Following redirect to: %s\n"

/* Debug Messages */
#define MSG_DEBUG_MODULE_INIT "Initializing %s module\n"
#define MSG_DEBUG_MODULE_CLEANUP "Cleaning up %s module\n"
#define MSG_DEBUG_MEMORY_STATS "Memory - Total: %zu bytes, Current: %zu bytes, Peak: %zu bytes\n"
#define MSG_DEBUG_HTTP_METHOD "HTTP Method: %s\n"
#define MSG_DEBUG_HTTP_HEADERS "HTTP Headers:\n%s\n"
#define MSG_DEBUG_HTTP_BODY "HTTP Body (%zu bytes):\n%s\n"
#define MSG_DEBUG_TLS_VERSION "TLS Version: %s\n"
#define MSG_DEBUG_TLS_CIPHER "TLS Cipher: %s\n"
#define MSG_DEBUG_PARSING_URL "Parsing URL: %s\n"
#define MSG_DEBUG_RESOLVED_HOST "Resolved %s to %s\n"

/* Memory Debug Messages */
#define MSG_DEBUG_MALLOC "malloc(%zu) = %p\n"
#define MSG_DEBUG_FREE "free(%p)\n"
#define MSG_DEBUG_REALLOC "realloc(%p, %zu) = %p\n"
#define MSG_DEBUG_MEMORY_LEAK "Memory leak: %zu bytes at %p (allocated at %s:%d)\n"

/* Signal Messages */
#define MSG_SIGNAL_HANDLING_INIT "Signal handling initialized\n"
#define MSG_SIGNAL_CLEANUP_START "Signal cleanup starting\n"
#define MSG_SIGNAL_CLEANUP_COMPLETE "Signal cleanup completed, exiting\n"
#define MSG_SIGNAL_CALLBACK_REGISTERED "Cleanup callback registered\n"

/* Fatal Error Messages */
#define MSG_FATAL_PREFIX "FATAL: "
#define MSG_FATAL_MODULE_NOT_INIT "%s module not initialized\n"
#define MSG_FATAL_OUT_OF_MEMORY "Out of memory\n"
#define MSG_FATAL_SYSTEM_ERROR "System error: %s\n"

#endif /* MESSAGES_H */