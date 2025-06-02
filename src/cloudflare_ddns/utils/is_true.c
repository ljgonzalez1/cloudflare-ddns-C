/**
 * @file is_true.c
 * @brief Boolean Conversion Utilities
 * @date 2025-06-01
 * 
 * Robust string-to-boolean conversion with comprehensive validation
 * and multiple accepted formats for maximum flexibility.
 * 
 * Accepted true values:
 * - "true" (case-sensitive)
 * - "True" (case-sensitive)  
 * - "TRUE" (case-sensitive)
 * - "1"
 * - "yes" (case-insensitive)
 * - "on" (case-insensitive)
 * 
 * All other values (including null, empty, whitespace-only) are false.
 */

#include "is_true.h"
#include <ctype.h>
#include <string.h>

// ==============================================================================
// INTERNAL CONSTANTS
// ==============================================================================

/** @brief Maximum length for boolean string processing */
#define MAX_BOOL_STRING_LENGTH 15

/** @brief Number of accepted true values */
#define NUM_TRUE_VALUES 6

/** @brief Array of accepted string values that evaluate to true */
static const char* TRUE_VALUES[NUM_TRUE_VALUES] = {
    "true",
    "True",
    "TRUE",
    "1",
    "yes",
    "on"
};

// ==============================================================================
// INTERNAL UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Skip leading whitespace in string
 * 
 * @param str Input string
 * @return Pointer to first non-whitespace character, or end of string
 */
static const char* skip_whitespace(const char* str) {
  if (str == NULL) return NULL;

  while (*str && isspace((unsigned char)*str)) {
    str++;
  }
  return str;
}

/**
 * @brief Extract non-whitespace token from string
 * 
 * Extracts up to buffer_size-1 non-whitespace characters into buffer.
 * 
 * @param source Source string (after skipping leading whitespace)
 * @param buffer Output buffer for extracted token
 * @param buffer_size Size of output buffer
 * @return Length of extracted token
 */
static size_t extract_token(const char* source, char* buffer, size_t buffer_size) {
  if (source == NULL || buffer == NULL || buffer_size <= 1) {
    if (buffer && buffer_size > 0) {
      buffer[0] = '\0';
    }
    return 0;
  }

  size_t i = 0;
  while (*source && !isspace((unsigned char)*source) && i < buffer_size - 1) {
    buffer[i++] = *source++;
  }
  buffer[i] = '\0';

  return i;
}

/**
 * @brief Case-insensitive string comparison
 * 
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return true if strings are equal (case-insensitive), false otherwise
 */
static bool strcasecmp_portable(const char* str1, const char* str2) {
  if (str1 == NULL || str2 == NULL) {
    return (str1 == str2); // Both NULL = equal, one NULL = not equal
  }

  while (*str1 && *str2) {
    if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) {
      return false;
    }
    str1++;
    str2++;
  }

  return (*str1 == *str2); // Both should be '\0' for equality
}

// ==============================================================================
// PUBLIC API IMPLEMENTATION
// ==============================================================================

bool to_bool(const char* value) {
  // Handle null input
  if (value == NULL) {
    return false;
  }

  // Skip leading whitespace
  const char* trimmed_value = skip_whitespace(value);

  // Handle empty or whitespace-only string
  if (*trimmed_value == '\0') {
    return false;
  }

  // Extract the first token (up to whitespace or end of string)
  char token_buffer[MAX_BOOL_STRING_LENGTH + 1];
  size_t token_length = extract_token(trimmed_value, token_buffer, sizeof(token_buffer));

  // Handle extraction failure or empty token
  if (token_length == 0) {
    return false;
  }

  // Check against each accepted true value
  for (size_t i = 0; i < NUM_TRUE_VALUES; i++) {
    const char* true_value = TRUE_VALUES[i];

    // For "yes" and "on", use case-insensitive comparison
    if (strcmp(true_value, "yes") == 0 || strcmp(true_value, "on") == 0) {
      if (strcasecmp_portable(token_buffer, true_value)) {
        return true;
      }
    } else {
      // For other values, use case-sensitive comparison
      if (strcmp(token_buffer, true_value) == 0) {
        return true;
      }
    }
  }

  // No match found
  return false;
}

// ==============================================================================
// ADDITIONAL UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Convert boolean to string representation
 * 
 * @param value Boolean value to convert
 * @return String representation ("true" or "false")
 */
const char* bool_to_string(bool value) {
  return value ? "true" : "false";
}

/**
 * @brief Check if string could be a valid boolean value
 * 
 * Determines if the input string is one of the recognized boolean formats,
 * regardless of whether it evaluates to true or false.
 * 
 * @param value String to check
 * @return true if string is a recognized boolean format, false otherwise
 */
bool is_valid_bool_string(const char* value) {
  if (value == NULL) {
    return false;
  }

  // Skip whitespace and extract token
  const char* trimmed_value = skip_whitespace(value);
  if (*trimmed_value == '\0') {
    return false;
  }

  char token_buffer[MAX_BOOL_STRING_LENGTH + 1];
  size_t token_length = extract_token(trimmed_value, token_buffer, sizeof(token_buffer));

  if (token_length == 0) {
    return false;
  }

  // Check against true values
  for (size_t i = 0; i < NUM_TRUE_VALUES; i++) {
    const char* true_value = TRUE_VALUES[i];

    if (strcmp(true_value, "yes") == 0 || strcmp(true_value, "on") == 0) {
      if (strcasecmp_portable(token_buffer, true_value)) {
        return true;
      }
    } else {
      if (strcmp(token_buffer, true_value) == 0) {
        return true;
      }
    }
  }

  // Check against false values (case-insensitive)
  const char* false_values[] = {"false", "False", "FALSE", "0", "no", "off"};
  const size_t num_false_values = sizeof(false_values) / sizeof(false_values[0]);

  for (size_t i = 0; i < num_false_values; i++) {
    const char* false_value = false_values[i];

    if (strcmp(false_value, "no") == 0 || strcmp(false_value, "off") == 0) {
      if (strcasecmp_portable(token_buffer, false_value)) {
        return true;
      }
    } else {
      if (strcmp(token_buffer, false_value) == 0) {
        return true;
      }
    }
  }

  return false;
}
