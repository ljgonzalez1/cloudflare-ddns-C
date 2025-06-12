#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

// Project configuration
#define PROJECT_NAME "cloudflare-ddns-client"
#define PROJECT_VERSION "1.0.0"

// Memory settings and limits
#define MAX_MALLOC_RETRIES 1000000
#define MIN_CLOUDFLARE_API_KEY_LENGTH 16
#define MAX_CLOUDFLARE_API_KEY_LENGTH 64
#define MIN_URL_LENGTH 3
#define MAX_URL_LENGTH 253

// Defaults values
#define DEFAULT_MINUTES_BETWEEN_UPDATES 15
#define DEFAULT_PROPAGATION_DELAY_SECONDS 60

// Environments variables
#define CLOUDFLARE_API_KEY_ENV_VAR "CLOUDFLARE_API_KEY"
#define DOMAINS_ENV_VAR "DOMAINS"
#define PROXIED_ENV_VAR "PROXIED"
#define MINUTES_BETWEEN_UPDATES_ENV_VAR "MINUTES_BETWEEN_UPDATES"
#define PROPAGATION_DELAY_SECONDS_ENV_VAR "PROPAGATION_DELAY_SECONDS"
#define IP_V4_APIS_ENV_VAR "IP_V4_APIS"

// Delimiters and constants
#define DOMAIN_DELIMITER ','
#define MAX_STRING_LENGTH 1024
#define MAX_ARRAY_SIZE 100

// Useful macros
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
