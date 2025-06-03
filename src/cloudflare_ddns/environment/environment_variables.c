/**
 * @file environment_variables.c
 * @brief Environment Variables Management Implementation
 */

#include "environment_variables.h"

// ==============================================================================
// GLOBAL VARIABLE DEFINITION
// ==============================================================================

/**
 * @brief Global environment configuration instance
 * 
 * Initialized by init_env_variables(), cleaned up by cleanup_env_variables()
 */
EnvVariables Env = {
    .PROXIED = false,
    .CLOUDFLARE_API_KEY = NULL,
    .DOMAINS = NULL,
    .DOMAINS_COUNT = 0,
    .IP_V4_APIS = NULL,
    .IP_V4_APIS_COUNT = 0,
    .MINUTES_BETWEEN_UPDATES = 5,
    .PROPAGATION_DELAY_SECONDS = 60
};

// ==============================================================================
// INTERNAL CONSTANTS
// ==============================================================================

/** @brief Empty result constant for failed parsing operations */
static const MetaArray EMPTY_RESULT = { .arr = NULL, .size = 0 };

// ==============================================================================
// MEMORY MANAGEMENT UTILITIES
// ==============================================================================

/**
 * @brief Safe memory allocation with retry mechanism
 * 
 * Implements robust memory allocation with configurable retry attempts.
 * This helps handle temporary memory pressure scenarios gracefully.
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if all attempts failed
 * 
 * @note Uses exponential backoff would be ideal, but for simplicity
 *       we just retry immediately up to MAX_MALLOC_ITERATIONS times
 */
static void* safe_malloc(size_t size) {
  if (size == 0) return NULL;

  void* ptr = NULL;
  for (unsigned int attempt = 0; attempt < MAX_MALLOC_ITERATIONS && ptr == NULL; attempt++) {
    ptr = malloc(size);

#ifdef ENV_ENABLE_LOGGING
    if (ptr == NULL && attempt > 0) {
            env_log("DEBUG", "Memory allocation attempt %u failed for %zu bytes", attempt, size);
        }
#endif
  }

#ifdef ENV_ENABLE_LOGGING
  if (ptr == NULL) {
        env_log("ERROR", "Failed to allocate %zu bytes after %u attempts", size, MAX_MALLOC_ITERATIONS);
    }
#endif

  return ptr;
}

/**
 * @brief Safe string duplication with retry mechanism
 * 
 * Combines safe memory allocation with string copying, providing
 * the same retry mechanism as safe_malloc().
 * 
 * @param source String to duplicate (must not be NULL)
 * @return Duplicated string on heap, or NULL if allocation failed
 * 
 * @pre source != NULL
 * @post If successful, returned string must be freed by caller
 */
static char* safe_strdup(const char* source) {
  if (source == NULL) return NULL;

  char* duplicate = NULL;
  for (unsigned int attempt = 0; attempt < MAX_MALLOC_ITERATIONS && duplicate == NULL; attempt++) {
    duplicate = strdup(source);
  }

  return duplicate;
}

/**
 * @brief Safely deallocate array of strings
 * 
 * Frees each string in the array, then frees the array itself.
 * Safe to call with NULL array or zero count.
 * 
 * @param string_array Array of string pointers to free
 * @param count Number of strings in the array
 * 
 * @post string_array contents are freed (but pointer itself unchanged)
 * @note Caller should set string_array to NULL after calling this
 */
static void free_string_array(char** string_array, size_t count) {
  if (string_array == NULL) return;

#ifdef ENV_ENABLE_LOGGING
  env_log("DEBUG", "Freeing string array with %zu elements", count);
#endif

  for (size_t i = 0; i < count; i++) {
    if (string_array[i] != NULL) {
      free(string_array[i]);
    }
  }
  free(string_array);
}

// ==============================================================================
// INPUT VALIDATION UTILITIES
// ==============================================================================

/**
 * @brief Validate input string for processing
 * 
 * Checks if string is suitable for parsing (not NULL and not empty).
 * 
 * @param input String to validate
 * @return true if input is valid for processing, false otherwise
 */
static bool is_input_valid(const char* input) {
  return input != NULL && input[0] != '\0';
}

// ==============================================================================
// DOMAIN COUNTING UTILITIES
// ==============================================================================

/**
 * @brief Count number of domains in comma-separated string
 * 
 * Counts domain separators and adds 1 to get total domain count.
 * Handles edge cases like empty strings and strings without separators.
 * 
 * @param domain_string String to analyze
 * @return Number of domains that would result from parsing
 * 
 * Examples:
 * - "example.com" → 1
 * - "example.com,test.org" → 2  
 * - "a,b,c,d" → 4
 * - "" → 0
 * - NULL → 0
 */
static size_t count_expected_domains(const char* domain_string) {
  if (!is_input_valid(domain_string)) {
    return 0;
  }

  size_t separator_count = 0;
  for (const char* ptr = domain_string; *ptr != '\0'; ptr++) {
    if (*ptr == ',') {
      separator_count++;
    }
  }

  // Number of domains = number of separators + 1
  return separator_count + 1;
}

// ==============================================================================
// TOKENIZATION UTILITIES
// ==============================================================================

/**
 * @brief Result structure for tokenization operations
 * 
 * Encapsulates both success/failure status and the number of items
 * successfully processed, enabling better error handling and partial
 * success scenarios.
 */
typedef struct {
  bool success;           ///< Whether tokenization completed successfully
  size_t processed_count; ///< Number of domains successfully tokenized
} TokenizationResult;

/**
 * @brief Tokenize domain string and populate output array
 * 
 * Splits the working buffer on commas and creates individual domain
 * strings in the output array. Handles allocation failures gracefully
 * by cleaning up partial results.
 * 
 * @param working_buffer Modifiable buffer to tokenize (will be modified by strtok)
 * @param output_array Pre-allocated array to populate with domain strings
 * @param max_domains Maximum number of domains the output array can hold
 * @return TokenizationResult with success status and count of processed domains
 * 
 * @pre working_buffer != NULL
 * @pre output_array != NULL  
 * @pre max_domains > 0
 * @post On failure, any allocated strings are freed
 */
static TokenizationResult tokenize_domains(char* working_buffer, char** output_array, size_t max_domains) {
  TokenizationResult result = { .success = false, .processed_count = 0 };

  // Defensive programming: validate inputs
  if (working_buffer == NULL || output_array == NULL || max_domains == 0) {
#ifdef ENV_ENABLE_LOGGING
    env_log("ERROR", "Invalid parameters passed to tokenize_domains");
#endif
    return result;
  }

  char* token = strtok(working_buffer, ENV_DOMAIN_SEPARATOR);
  size_t index = 0;

  while (token != NULL && index < max_domains) {
    // Attempt to duplicate the token
    char* domain_copy = safe_strdup(token);
    if (domain_copy == NULL) {
#ifdef ENV_ENABLE_LOGGING
      env_log("ERROR", "Failed to allocate memory for domain %zu", index);
#endif

      // Clean up what we've allocated so far
      free_string_array(output_array, index);
      return result;
    }

    output_array[index++] = domain_copy;
    token = strtok(NULL, ENV_DOMAIN_SEPARATOR);

#ifdef ENV_ENABLE_LOGGING
    env_log("DEBUG", "Successfully tokenized domain %zu: %s", index - 1, domain_copy);
#endif
  }

  result.success = true;
  result.processed_count = index;
  return result;
}

// ==============================================================================
// MAIN PARSING ORCHESTRATOR
// ==============================================================================

/**
 * @brief Parse comma-separated domain string into MetaArray
 * 
 * This is the main orchestration function that coordinates all the
 * parsing steps. It follows a clear pipeline: validate → count → 
 * allocate → tokenize → return result.
 * 
 * The function is designed to be robust and handle various failure
 * scenarios gracefully, always cleaning up allocated memory.
 * 
 * @param raw_domains Comma-separated string of domain names
 * @return MetaArray with parsed domains, or empty array on failure
 * 
 * Process:
 * 1. Validate input string
 * 2. Count expected number of domains
 * 3. Create working buffer (modifiable copy)
 * 4. Allocate output array
 * 5. Tokenize and populate array
 * 6. Return result or clean up on failure
 * 
 * Examples:
 * - parse_domains("example.com,test.org") → MetaArray with 2 strings
 * - parse_domains("") → Empty MetaArray
 * - parse_domains(NULL) → Empty MetaArray
 */
static MetaArray parse_domains(const char* raw_domains) {
#ifdef ENV_ENABLE_LOGGING
  env_log("DEBUG", "Starting domain parsing for: %s", raw_domains ? raw_domains : "(null)");
#endif

  // Step 1: Validate input
  if (!is_input_valid(raw_domains)) {
#ifdef ENV_ENABLE_LOGGING
    env_log("INFO", "No domains to parse (empty or null input)");
#endif
    return EMPTY_RESULT;
  }

  // Step 2: Count expected domains
  size_t expected_domain_count = count_expected_domains(raw_domains);
  if (expected_domain_count == 0) {
#ifdef ENV_ENABLE_LOGGING
    env_log("WARN", "Domain count calculation returned 0");
#endif
    return EMPTY_RESULT;
  }

#ifdef ENV_ENABLE_LOGGING
  env_log("DEBUG", "Expecting %zu domains", expected_domain_count);
#endif

  // Step 3: Create working buffer
  char* working_buffer = safe_strdup(raw_domains);
  if (working_buffer == NULL) {
#ifdef ENV_ENABLE_LOGGING
    env_log("ERROR", "Failed to create working buffer");
#endif
    return EMPTY_RESULT;
  }

  // Step 4: Allocate output array
  char** domain_array = (char**)safe_malloc(expected_domain_count * sizeof(char*));
  if (domain_array == NULL) {
#ifdef ENV_ENABLE_LOGGING
    env_log("ERROR", "Failed to allocate domain array");
#endif
    free(working_buffer);
    return EMPTY_RESULT;
  }

  // Step 5: Tokenize and populate
  TokenizationResult tokenize_result = tokenize_domains(working_buffer, domain_array, expected_domain_count);
  free(working_buffer); // Working buffer no longer needed

  // Step 6: Handle result
  if (!tokenize_result.success) {
#ifdef ENV_ENABLE_LOGGING
    env_log("ERROR", "Domain tokenization failed");
#endif
    return EMPTY_RESULT;
  }

#ifdef ENV_ENABLE_LOGGING
  env_log("INFO", "Successfully parsed %zu domains", tokenize_result.processed_count);
#endif

  // Return successful result
  MetaArray result = {
      .arr = domain_array,
      .size = tokenize_result.processed_count
  };

  return result;
}

// ==============================================================================
// PUBLIC API IMPLEMENTATION
// ==============================================================================

void init_env_variables(void) {
#ifdef ENV_ENABLE_LOGGING
  env_log("INFO", "Initializing environment variables");
#endif

  // Initialize PROXIED flag
  const char* proxied_string = get_env_var("PROXIED");
  Env.PROXIED = to_bool(proxied_string);

#ifdef ENV_ENABLE_LOGGING
  env_log("DEBUG", "PROXIED set to: %s", Env.PROXIED ? "true" : "false");
#endif

  // Initialize API key (direct pointer, no memory allocation)
  Env.CLOUDFLARE_API_KEY = (char*)get_env_var("CLOUDFLARE_API_KEY");

#ifdef ENV_ENABLE_LOGGING
  if (Env.CLOUDFLARE_API_KEY && strlen(Env.CLOUDFLARE_API_KEY) > 0) {
        env_log("DEBUG", "API key loaded (length: %zu)", strlen(Env.CLOUDFLARE_API_KEY));
    } else {
        env_log("WARN", "API key not found or empty");
    }
#endif

  // initialize propagation delay (direct pointer, no memory allocation)
  const char* propagation_string = get_env_var("PROPAGATION_DELAY");
  // TODO: Validate propagation delay string and convert to integer
  Env.PROPAGATION_DELAY_SECONDS = (unsigned int) atoi(propagation_string);

  // initialize propagation delay (direct pointer, no memory allocation)
  const char* update_time = get_env_var("MINUTES_BETWEEN_UPDATES");
  // TODO: Validate propagation delay string and convert to integer
  Env.MINUTES_BETWEEN_UPDATES = (unsigned int) atoi(update_time);



  // Parse and initialize domains
  const char* domains_string = get_env_var("DOMAINS");
  MetaArray domains_result = parse_domains(domains_string);

  Env.DOMAINS = (char**)domains_result.arr;
  Env.DOMAINS_COUNT = domains_result.size;

  // Parse and initialize APIs for getting public IP. If none provided, use hardcoded values
  const char* api_string = strcmp(get_env_var("IP_V4_APIS"), "") == 0 ? HARDCODED_IP_V4_APIS : get_env_var("IP_V4_APIS");
  MetaArray api_result = parse_domains(api_string);

  Env.IP_V4_APIS = (char**)api_result.arr;
  Env.IP_V4_APIS_COUNT = api_result.size;


#ifdef ENV_ENABLE_LOGGING
  env_log("INFO", "Environment initialization complete. Loaded %zu domains", Env.DOMAINS_COUNT);
#endif
}

void cleanup_env_variables(void) {
#ifdef ENV_ENABLE_LOGGING
  env_log("INFO", "Cleaning up environment variables");
#endif

  // Clean up domains array if allocated
  if (Env.DOMAINS != NULL) {
    free_string_array(Env.DOMAINS, Env.DOMAINS_COUNT);
    Env.DOMAINS = NULL;
    Env.DOMAINS_COUNT = 0;

#ifdef ENV_ENABLE_LOGGING
    env_log("DEBUG", "Domains array cleaned up");
#endif
  }

  // Clean up domains APIs array if allocated
  if (Env.IP_V4_APIS != NULL) {
    free_string_array(Env.IP_V4_APIS, Env.IP_V4_APIS_COUNT);
    Env.IP_V4_APIS = NULL;
    Env.IP_V4_APIS_COUNT = 0;

#ifdef ENV_ENABLE_LOGGING
    env_log("DEBUG", "Domains IP APIs array cleaned up");
#endif
  }

  // Reset other fields (API key is not allocated by us, so don't free)
  Env.PROXIED = false;
  Env.CLOUDFLARE_API_KEY = NULL;

#ifdef ENV_ENABLE_LOGGING
  env_log("INFO", "Environment cleanup complete");
#endif

  // Reset propagation delay and update time
  Env.PROPAGATION_DELAY_SECONDS = 0;
  Env.MINUTES_BETWEEN_UPDATES = 0;
}

// ==============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// ==============================================================================

bool is_env_initialized(void) {
  // Basic sanity checks
  bool api_key_valid = (Env.CLOUDFLARE_API_KEY != NULL && strlen(Env.CLOUDFLARE_API_KEY) > 0);
  bool times_are_valid = (Env.PROPAGATION_DELAY_SECONDS > 0 && Env.MINUTES_BETWEEN_UPDATES > 0);
  bool domains_consistent = (Env.DOMAINS_COUNT == 0) ? (Env.DOMAINS == NULL) : (Env.DOMAINS != NULL);
  bool apis_consistent = (Env.IP_V4_APIS_COUNT == 0) ? (Env.IP_V4_APIS == NULL) : (Env.IP_V4_APIS != NULL);

  return api_key_valid && domains_consistent && apis_consistent && times_are_valid;
}

void print_env_config(bool show_domains) {
  printf("Environment Configuration:\n");
  printf("  Proxied: %s\n", Env.PROXIED ? "true" : "false");

  // Mask API key for security
  if (Env.CLOUDFLARE_API_KEY != NULL && strlen(Env.CLOUDFLARE_API_KEY) > 8) {
    printf("  API Key: %.8s... (masked)\n", Env.CLOUDFLARE_API_KEY);
  } else if (Env.CLOUDFLARE_API_KEY != NULL) {
    printf("  API Key: ***... (masked, short)\n");
  } else {
    printf("  API Key: (not set)\n");
  }

  printf("  Domains: %zu configured\n", Env.DOMAINS_COUNT);

  if (show_domains && Env.DOMAINS_COUNT > 0) {
    for (size_t i = 0; i < Env.DOMAINS_COUNT; i++) {
      printf("    [%zu] %s\n", i, Env.DOMAINS[i]);
    }
  }
}

// ==============================================================================
// OPTIONAL FEATURE IMPLEMENTATIONS
// ==============================================================================

#ifdef ENV_ENABLE_VALIDATION
bool validate_domain_format(const char* domain) {
    if (domain == NULL || strlen(domain) == 0) {
        return false;
    }
    
    size_t length = strlen(domain);
    
    // Basic RFC checks
    if (length > 253) return false;                    // RFC length limit
    if (domain[0] == '.' || domain[length-1] == '.') return false;  // Leading/trailing dots
    if (strstr(domain, "..") != NULL) return false;   // Consecutive dots
    
    // Could add more sophisticated validation here
    return true;
}
#endif

#ifdef ENV_ENABLE_LOGGING
#include <stdarg.h>

void env_log(const char* level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[ENV:%s] ", level);
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}
#endif
