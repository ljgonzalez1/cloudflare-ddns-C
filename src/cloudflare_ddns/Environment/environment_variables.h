#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/get_env.h"
#include "../utils/is_true.h"
#include "../utils/meta_array.h"

#define MAX_MALLOC_ITERATIONS 1000000

/**
 * EnvVariables
 *
 * This struct holds configuration values populated from environment variables.
 * It centralizes all relevant settings needed by the application at runtime.
 *
 * Fields:
 *   – PROXIED
 *       A boolean flag indicating whether requests should be routed through a proxy.
 *       Set to true if the environment variable "PROXIED" (after trimming whitespace)
 *       exactly matches "true", "True", or "1"; otherwise false.
 *
 *   – CLOUDFLARE_API_KEY
 *       A pointer to a null-terminated C-string containing the Cloudflare API key,
 *       as read directly from the environment variable "CLOUDFLARE_API_KEY".
 *       If the environment variable is not set, this pointer may be NULL.
 *
 *   – DOMAINS
 *       An array of null-terminated C-strings, each representing a single domain name.
 *       These strings are allocated on the heap via strdup(token) for each domain
 *       parsed from the environment variable "DOMAINS". If "DOMAINS" is not set or
 *       is the empty string, this pointer is NULL.
 *
 *   – DOMAINS_COUNT
 *       The number of entries in the DOMAINS array. If DOMAINS is NULL or contains
 *       no valid domain tokens, this value is zero. Otherwise, it equals the number of
 *       substrings obtained by splitting the original "DOMAINS" string on commas.
 *
 * Notes on memory ownership:
 *   – The caller (or main program) is responsible for freeing each string in DOMAINS
 *     via free(DOMAINS[i]) and then freeing the DOMAINS array itself via free(DOMAINS),
 *     once those domain names are no longer needed.
 *   – CLOUDFLARE_API_KEY is not duplicated by strdup inside init_env_variables;
 *     it points directly to the string returned by get_env_var("CLOUDFLARE_API_KEY"), so
 *     it should not be freed by the caller.
 */
struct env_variables {
  bool PROXIED;
  char *CLOUDFLARE_API_KEY;
  char **DOMAINS;
  size_t DOMAINS_COUNT;
};

typedef struct env_variables EnvVariables;

extern EnvVariables Env;

/**
 * init_env_variables
 *
 * Populates the global Env variable (of type EnvVariables) by reading and interpreting
 * specific environment variables. This function must be called once at program startup
 * before accessing any fields of Env.
 *
 * Process:
 *   1. Read the string from environment variable "PROXIED" via get_env_var("PROXIED").
 *      – Calls to_bool(proxied_str) to convert that C-string to a boolean, using the same
 *        rules as to_bool (i.e., null or whitespace-only → false; otherwise compare up to
 *        15 non-whitespace characters exactly to "true", "True", or "1").
 *      – Assigns the resulting boolean to Env.PROXIED.
 *
 *   2. Read the string from environment variable "CLOUDFLARE_API_KEY" via
 *      get_env_var("CLOUDFLARE_API_KEY").
 *      – Directly stores the returned pointer (which may be NULL) in
 *        Env.CLOUDFLARE_API_KEY.
 *      – The caller must not free this pointer, as ownership remains external.
 *
 *   3. Read the string from environment variable "DOMAINS" via get_env_var("DOMAINS").
 *      – Passes that C-string (which may be NULL or empty) to parse_domains().
 *      – parse_domains() splits on commas, allocates a modifiable buffer, counts tokens,
 *        mallocs an array of char*, and calls strdup(token) for each domain substring.
 *      – The returned MetaArray.domains.arr is cast to (char**) and stored in
 *        Env.DOMAINS; the MetaArray.size is stored in Env.DOMAINS_COUNT.
 *      – If "DOMAINS" was NULL or empty, Env.DOMAINS is set to NULL and
 *        Env.DOMAINS_COUNT is zero.
 *
 * @param void
 *   No parameters. Relies on the following environment variables:
 *     – "PROXIED"
 *     – "CLOUDFLARE_API_KEY"
 *     – "DOMAINS"
 *
 * @return
 *   This function returns no value (void). After it returns, the global `Env`
 *   struct fields are fully initialized (or left as NULL/0 for missing or invalid
 *   inputs).
 *
 * @note
 *   – Memory allocated for Env.DOMAINS and each DOMAINS[i] must be freed by the caller
 *     when it is safe to do so, using:
 *       for (size_t i = 0; i < Env.DOMAINS_COUNT; i++) {
 *           free(Env.DOMAINS[i]);
 *       }
 *       free(Env.DOMAINS);
 *   – The string pointed to by Env.CLOUDFLARE_API_KEY is not heap-allocated here, and
 *     so must NOT be freed.
 *   – parse_domains uses up to MAX_MALLOC_ITERATIONS retries for strdup and malloc.
 *     If any allocation fails, sets Env.DOMAINS to NULL and Env.DOMAINS_COUNT to 0.
 *   – This function must be called exactly once before any code inspects or uses Env.
 */
void init_env_variables(void);
