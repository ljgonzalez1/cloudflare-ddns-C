/**
 * @file is_true.h
 * @brief Boolean Conversion Utilities Header
 * @date 2025-06-01
 * 
 * Comprehensive string-to-boolean conversion utilities with support for
 * multiple common boolean string formats. Designed for environment variable
 * parsing and configuration file processing.
 * 
 * This module provides robust, well-tested boolean parsing that handles
 * edge cases gracefully and supports internationalization-friendly
 * string formats.
 */

#pragma once

// ==============================================================================
// SYSTEM INCLUDES
// ==============================================================================

#include <stdbool.h>
#include <stddef.h>

// ==============================================================================
// CORE BOOLEAN CONVERSION API
// ==============================================================================

/**
 * @brief Convert string to boolean value with comprehensive format support
 * 
 * This function interprets a C-string as a boolean by following these rules:
 * 
 * 1. Returns false if `value` is NULL or consists only of whitespace
 * 2. Trims leading whitespace from the input
 * 3. Extracts the first non-whitespace token (up to 15 characters)
 * 4. Compares against recognized true/false values:
 * 
 * TRUE VALUES (case-sensitive unless noted):
 * - "true"
 * - "True" 
 * - "TRUE"
 * - "1"
 * - "yes" (case-insensitive)
 * - "on" (case-insensitive)
 * 
 * FALSE VALUES (everything else, including):
 * - "false", "False", "FALSE"
 * - "0"
 * - "no", "No", "NO" 
 * - "off", "Off", "OFF"
 * - Empty or whitespace-only strings
 * - Any unrecognized value
 * 
 * @param value A null-terminated C-string to interpret as boolean
 * @return true if value matches a recognized true format, false otherwise
 * 
 * @note This function is thread-safe and does not modify the input string
 * @note Leading/trailing whitespace is handled gracefully
 * @note The comparison is optimized for common use cases
 * 
 * @example
 * ```c
 * // Environment variable parsing
 * bool debug_mode = to_bool(getenv("DEBUG"));
 * bool proxy_enabled = to_bool(getenv("USE_PROXY"));
 * 
 * // Configuration file values
 * if (to_bool(config_value)) {
 *     enable_feature();
 * }
 * 
 * // Various input formats
 * assert(to_bool("true") == true);
 * assert(to_bool("True") == true);
 * assert(to_bool("1") == true);
 * assert(to_bool("yes") == true);
 * assert(to_bool("YES") == true);  // Case-insensitive
 * assert(to_bool("on") == true);
 * assert(to_bool("  true  ") == true);  // Whitespace handled
 * 
 * assert(to_bool("false") == false);
 * assert(to_bool("0") == false);
 * assert(to_bool("no") == false);
 * assert(to_bool("off") == false);
 * assert(to_bool("") == false);
 * assert(to_bool("   ") == false);
 * assert(to_bool("maybe") == false);  // Unrecognized = false
 * assert(to_bool(NULL) == false);
 * ```
 */
bool to_bool(const char* value);

// ==============================================================================
// ADDITIONAL UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Convert boolean value to string representation
 * 
 * Provides a consistent string representation for boolean values.
 * Useful for logging, configuration file writing, and debugging.
 * 
 * @param value Boolean value to convert
 * @return "true" if value is true, "false" if value is false
 * 
 * @note The returned string is a static constant and should not be freed
 * @note This function is thread-safe
 * 
 * @example
 * ```c
 * bool flag = true;
 * printf("Flag is: %s\n", bool_to_string(flag));  // Prints: "Flag is: true"
 * 
 * // Useful for logging configuration
 * printf("Proxy: %s, Debug: %s\n", 
 *        bool_to_string(config.proxy_enabled),
 *        bool_to_string(config.debug_mode));
 * ```
 */
const char* bool_to_string(bool value);

/**
 * @brief Validate if string is a recognized boolean format
 * 
 * Checks whether the input string is one of the recognized boolean formats,
 * regardless of whether it evaluates to true or false. This is useful for
 * configuration validation and error reporting.
 * 
 * @param value String to validate
 * @return true if string is a recognized boolean format, false otherwise
 * 
 * @note This function uses the same parsing logic as to_bool()
 * @note Useful for providing helpful error messages to users
 * 
 * @example
 * ```c
 * const char* user_input = "maybe";
 * 
 * if (!is_valid_bool_string(user_input)) {
 *     printf("Error: '%s' is not a valid boolean value.\n", user_input);
 *     printf("Valid values: true, false, 1, 0, yes, no, on, off\n");
 *     return CONFIG_ERROR;
 * }
 * 
 * bool value = to_bool(user_input);
 * ```
 */
bool is_valid_bool_string(const char* value);

// ==============================================================================
// CONFIGURATION AND FEATURE FLAGS
// ==============================================================================

/**
 * @brief Feature flags for compile-time customization
 * 
 * These preprocessor definitions can be used to customize behavior:
 * 
 * - BOOL_STRICT_MODE: If defined, only accepts exact case matches
 * - BOOL_EXTENDED_FORMATS: If defined, supports additional formats like "enabled/disabled"
 * - BOOL_NUMERIC_ONLY: If defined, only accepts "1" and "0"
 */

#ifdef BOOL_STRICT_MODE
/**
 * @brief Strict boolean parsing (exact case matches only)
 * 
 * When BOOL_STRICT_MODE is defined, this function only accepts
 * exact case-sensitive matches for boolean values.
 */
bool to_bool_strict(const char* value);
#endif

#ifdef BOOL_EXTENDED_FORMATS
/**
 * @brief Extended boolean parsing with additional formats
 * 
 * When BOOL_EXTENDED_FORMATS is defined, this function supports
 * additional boolean formats like "enabled/disabled", "active/inactive".
 */
bool to_bool_extended(const char* value);
#endif

#ifdef BOOL_NUMERIC_ONLY
/**
 * @brief Numeric-only boolean parsing
 * 
 * When BOOL_NUMERIC_ONLY is defined, this function only accepts
 * "1" (true) and "0" (false), rejecting all text-based formats.
 */
bool to_bool_numeric(const char* value);
#endif

// ==============================================================================
// BACKWARD COMPATIBILITY
// ==============================================================================

/**
 * @brief Legacy function name for backward compatibility
 * 
 * @deprecated Use to_bool() instead. This alias is provided for
 *             compatibility with existing code.
 */
#ifndef BOOL_NO_LEGACY_NAMES
#define is_true(value) to_bool(value)
#endif

// ==============================================================================
// TESTING AND DEBUG SUPPORT
// ==============================================================================

#ifdef UNIT_TESTING
/**
 * @brief Run comprehensive self-tests of boolean conversion functions
 * 
 * This function is only available when UNIT_TESTING is defined.
 * It runs a comprehensive suite of tests to validate the boolean
 * conversion functionality.
 * 
 * @return Number of failed tests (0 indicates all tests passed)
 */
int bool_conversion_self_test(void);

/**
 * @brief Print detailed information about supported boolean formats
 * 
 * Utility function for debugging and documentation purposes.
 * Only available when UNIT_TESTING is defined.
 */
void print_supported_bool_formats(void);
#endif

// ==============================================================================
// PERFORMANCE NOTES
// ==============================================================================

/**
 * Performance Characteristics:
 * 
 * - to_bool(): O(1) time complexity, processes up to 15 characters
 * - bool_to_string(): O(1) time complexity, returns static string
 * - is_valid_bool_string(): O(1) time complexity, same logic as to_bool()
 * 
 * Memory Usage:
 * - No dynamic memory allocation
 * - Stack usage: ~16 bytes for temporary token buffer
 * - No global state (thread-safe)
 * 
 * Optimization Notes:
 * - String comparisons are optimized for most common cases first
 * - Case-insensitive comparisons are only used where necessary
 * - Input validation is minimal but sufficient for robustness
 */
