/**
 * @file get_env.h
 * @brief Environment Variable Access Utilities Header
 * @date 2025-06-01
 * 
 * Enhanced environment variable utilities with validation, error handling,
 * and configuration management features.
 */

#pragma once

// ==============================================================================
// SYSTEM INCLUDES
// ==============================================================================

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

// ==============================================================================
// TYPE DEFINITIONS
// ==============================================================================

/**
 * @brief Environment variable requirement specification
 * 
 * Used for batch validation of environment variables with
 * specific requirements and constraints.
 */
typedef struct {
  const char* name;           ///< Environment variable name
  bool required;              ///< Whether this variable is required
  bool must_be_non_empty;     ///< Whether empty values are allowed (if set)
  size_t min_length;          ///< Minimum required length (0 = no minimum)
  size_t max_length;          ///< Maximum allowed length (0 = no maximum)
  bool sensitive;             ///< Whether to mask value in output (for passwords/keys)
} EnvRequirement;

// ==============================================================================
// CORE ENVIRONMENT VARIABLE ACCESS
// ==============================================================================

/**
 * @brief Retrieve environment variable value with error handling
 * 
 * Enhanced version of the original get_env_var() function with better
 * validation and consistent behavior.
 * 
 * @param variable_name Name of the environment variable to retrieve
 * @return Pointer to the environment variable value, or empty string if not found
 * 
 * @note The returned pointer should NOT be modified or freed
 * @note Returns empty string ("") instead of NULL for consistency
 * @note Prints warning to stderr if variable is not found (configurable)
 * 
 * @warning The returned pointer may become invalid if the environment changes
 * 
 * @example
 * ```c
 * const char* api_key = get_env_var("API_KEY");
 * if (strlen(api_key) == 0) {
 *     printf("API_KEY not configured\n");
 *     return ERROR;
 * }
 * ```
 */
const char* get_env_var(const char* variable_name);

/**
 * @brief Retrieve environment variable with default fallback
 * 
 * Gets environment variable value, returning a default value if the
 * variable is not set. More convenient than checking return values.
 * 
 * @param variable_name Name of the environment variable
 * @param default_value Value to return if variable is not set (can be NULL)
 * @return Environment variable value or default_value
 * 
 * @note If default_value is NULL, returns empty string for unset variables
 * @note No warning is printed when using default values
 * 
 * @example
 * ```c
 * const char* log_level = get_env_var_or_default("LOG_LEVEL", "INFO");
 * const char* debug_mode = get_env_var_or_default("DEBUG", "false");
 * int port = atoi(get_env_var_or_default("PORT", "8080"));
 * ```
 */
const char* get_env_var_or_default(const char* variable_name, const char* default_value);

// ==============================================================================
// ENVIRONMENT VARIABLE TESTING AND VALIDATION
// ==============================================================================

/**
 * @brief Check if environment variable exists
 * 
 * Tests whether an environment variable is defined, regardless of its value.
 * 
 * @param variable_name Name of the environment variable to check
 * @return true if variable is defined (even if empty), false otherwise
 * 
 * @example
 * ```c
 * if (is_env_var_set("DEBUG")) {
 *     enable_debug_mode();
 * }
 * 
 * if (!is_env_var_set("REQUIRED_CONFIG")) {
 *     print_setup_instructions();
 *     exit(1);
 * }
 * ```
 */
bool is_env_var_set(const char* variable_name);

/**
 * @brief Check if environment variable is empty or unset
 * 
 * Tests whether an environment variable is either not defined or
 * set to an empty string.
 * 
 * @param variable_name Name of the environment variable to check
 * @return true if variable is unset or empty, false if it has content
 * 
 * @example
 * ```c
 * if (is_env_var_empty("API_ENDPOINT")) {
 *     printf("Error: API_ENDPOINT must be configured\n");
 *     return CONFIG_ERROR;
 * }
 * ```
 */
bool is_env_var_empty(const char* variable_name);

/**
 * @brief Get length of environment variable value
 * 
 * Returns the length of the environment variable's value,
 * or 0 if the variable is not set.
 * 
 * @param variable_name Name of the environment variable
 * @return Length of the variable's value, or 0 if unset
 * 
 * @example
 * ```c
 * size_t api_key_len = get_env_var_length("API_KEY");
 * if (api_key_len < 10) {
 *     printf("Warning: API key seems too short\n");
 * }
 * ```
 */
size_t get_env_var_length(const char* variable_name);

// ==============================================================================
// BATCH VALIDATION AND MANAGEMENT
// ==============================================================================

/**
 * @brief Validate multiple environment variables against requirements
 * 
 * Performs comprehensive validation of multiple environment variables
 * based on specified requirements. Useful for application startup validation.
 * 
 * @param requirements Array of environment variable requirements
 * @param count Number of requirements in the array
 * @return true if all requirements are satisfied, false otherwise
 * 
 * @note Prints detailed validation results to stdout
 * @note Sensitive variables are masked in output
 * 
 * @example
 * ```c
 * EnvRequirement requirements[] = {
 *     {"API_KEY", true, true, 10, 0, true},          // Required, non-empty, min 10 chars, sensitive
 *     {"DEBUG", false, false, 0, 0, false},          // Optional
 *     {"TIMEOUT", false, true, 1, 10, false},        // Optional but if set, must be 1-10 chars
 * };
 * 
 * if (!validate_env_requirements(requirements, 3)) {
 *     printf("Environment validation failed\n");
 *     exit(1);
 * }
 * ```
 */
bool validate_env_requirements(const EnvRequirement* requirements, size_t count);

/**
 * @brief Print summary of environment variables
 * 
 * Displays a formatted summary of specified environment variables
 * with their current values. Useful for debugging and configuration review.
 * 
 * @param variable_names Array of environment variable names to display
 * @param count Number of variables in the array
 * @param mask_sensitive Whether to mask potentially sensitive values
 * 
 * @note Sensitive variables are auto-detected by name patterns (KEY, TOKEN, etc.)
 * 
 * @example
 * ```c
 * const char* vars[] = {"API_KEY", "DEBUG", "LOG_LEVEL", "PORT"};
 * print_env_summary(vars, 4, true);
 * ```
 */
void print_env_summary(const char** variable_names, size_t count, bool mask_sensitive);

// ==============================================================================
// CONFIGURATION AND CONTROL
// ==============================================================================

/**
 * @brief Enable or disable warning messages for missing variables
 * 
 * Controls whether get_env_var() prints warnings to stderr when
 * environment variables are not found.
 * 
 * @param enable true to enable warnings, false to disable
 * 
 * @note Default is enabled
 * @note Useful for suppressing warnings during testing
 * 
 * @example
 * ```c
 * // Disable warnings during optional configuration loading
 * set_env_warnings(false);
 * load_optional_config();
 * set_env_warnings(true);
 * ```
 */
void set_env_warnings(bool enable);

/**
 * @brief Enable or disable debug output for environment operations
 * 
 * Controls detailed debug output showing environment variable lookups
 * and operations. Useful for troubleshooting configuration issues.
 * 
 * @param enable true to enable debug output, false to disable
 * 
 * @note Default is disabled
 * @note Debug output goes to stdout
 * 
 * @example
 * ```c
 * if (is_debug_build()) {
 *     set_env_debug(true);
 * }
 * ```
 */
void set_env_debug(bool enable);

// ==============================================================================
// CONVENIENCE MACROS
// ==============================================================================

/**
 * @brief Macro for defining required environment variables
 * 
 * Convenience macro for creating EnvRequirement structures for
 * required, non-empty variables.
 */
#define ENV_REQUIRED(name) \
    { (name), true, true, 0, 0, false }

/**
 * @brief Macro for defining optional environment variables
 * 
 * Convenience macro for creating EnvRequirement structures for
 * optional variables.
 */
#define ENV_OPTIONAL(name) \
    { (name), false, false, 0, 0, false }

/**
 * @brief Macro for defining sensitive environment variables
 * 
 * Convenience macro for creating EnvRequirement structures for
 * sensitive variables that should be masked in output.
 */
#define ENV_SENSITIVE(name) \
    { (name), true, true, 0, 0, true }

/**
 * @brief Macro for defining environment variables with length constraints
 * 
 * Convenience macro for creating EnvRequirement structures with
 * minimum and maximum length requirements.
 */
#define ENV_LENGTH(name, min_len, max_len) \
    { (name), true, true, (min_len), (max_len), false }

// ==============================================================================
// BACKWARD COMPATIBILITY
// ==============================================================================

/**
 * @brief Legacy function name mapping
 * 
 * For compatibility with existing code that uses the old function name.
 * 
 * @deprecated Use get_env_var() instead
 */
#ifndef ENV_NO_LEGACY_NAMES
#define get_env_var_legacy get_env_var
#endif

// ==============================================================================
// ADVANCED FEATURES (OPTIONAL)
// ==============================================================================

#ifdef ENV_ENABLE_ADVANCED_FEATURES

/**
 * @brief Environment variable change callback type
 * 
 * Function pointer type for callbacks that are invoked when
 * environment variables change (if monitoring is enabled).
 */
typedef void (*env_change_callback_t)(const char* variable_name, 
                                     const char* old_value, 
                                     const char* new_value);

/**
 * @brief Register callback for environment variable changes
 * 
 * @param variable_name Name of variable to monitor
 * @param callback Function to call when variable changes
 * @return true if monitoring was set up successfully
 * 
 * @note This feature requires platform-specific implementation
 * @note Only available when ENV_ENABLE_ADVANCED_FEATURES is defined
 */
bool monitor_env_var(const char* variable_name, env_change_callback_t callback);

/**
 * @brief Export environment variables to file
 * 
 * @param filename File to write environment variables to
 * @param variable_names Array of variable names to export
 * @param count Number of variables to export
 * @return true if export was successful
 */
bool export_env_vars(const char* filename, const char** variable_names, size_t count);

#endif // ENV_ENABLE_ADVANCED_FEATURES
