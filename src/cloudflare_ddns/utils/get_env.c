/**
 * @file get_env.c
 * @brief Environment Variable Access Utilities
 * @date 2025-06-01
 * 
 * Enhanced environment variable access with better error handling,
 * validation, and user-friendly feedback.
 */

#include "get_env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// CONFIGURATION
// ==============================================================================

/** @brief Whether to print warnings for missing environment variables */
static bool g_print_warnings = true;

/** @brief Whether to print debug information */
static bool g_debug_mode = false;

// ==============================================================================
// INTERNAL UTILITIES
// ==============================================================================

/**
 * @brief Check if environment variable name is valid
 * 
 * @param name Variable name to validate
 * @return true if name is valid, false otherwise
 */
static bool is_valid_env_name(const char* name) {
  if (name == NULL || *name == '\0') {
    return false;
  }

  // Environment variable names should not contain '=' or null bytes
  if (strchr(name, '=') != NULL || strchr(name, '\0') != name + strlen(name)) {
    return false;
  }

  return true;
}

/**
 * @brief Print debug information if debug mode is enabled
 * 
 * @param format Printf-style format string
 * @param ... Printf-style arguments
 */
static void debug_printf(const char* format, ...) {
  if (!g_debug_mode) return;

  va_list args;
  va_start(args, format);
  printf("[ENV DEBUG] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

// ==============================================================================
// PUBLIC API IMPLEMENTATION
// ==============================================================================

const char* get_env_var(const char* variable_name) {
  // Validate input
  if (!is_valid_env_name(variable_name)) {
    if (g_print_warnings) {
      fprintf(stderr, "Warning: Invalid environment variable name provided\n");
    }
    return "";
  }

  debug_printf("Looking up environment variable: %s", variable_name);

  // Attempt to get the environment variable
  const char* value = getenv(variable_name);

  if (value == NULL) {
    if (g_print_warnings) {
      fprintf(stderr, "Environment variable `%s` not defined.\n", variable_name);
    }
    debug_printf("Environment variable %s not found", variable_name);
    return "";
  }

  debug_printf("Environment variable %s found with length %zu",
               variable_name, strlen(value));

  return value;
}

const char* get_env_var_or_default(const char* variable_name, const char* default_value) {
  if (!is_valid_env_name(variable_name)) {
    if (g_print_warnings) {
      fprintf(stderr, "Warning: Invalid environment variable name, using default\n");
    }
    return default_value ? default_value : "";
  }

  const char* value = getenv(variable_name);

  if (value == NULL) {
    debug_printf("Environment variable %s not found, using default: %s",
                 variable_name, default_value ? default_value : "(null)");
    return default_value ? default_value : "";
  }

  debug_printf("Environment variable %s found: %s", variable_name, value);
  return value;
}

bool is_env_var_set(const char* variable_name) {
  if (!is_valid_env_name(variable_name)) {
    return false;
  }

  return getenv(variable_name) != NULL;
}

bool is_env_var_empty(const char* variable_name) {
  if (!is_valid_env_name(variable_name)) {
    return true; // Invalid names are considered "empty"
  }

  const char* value = getenv(variable_name);
  return (value == NULL || *value == '\0');
}

size_t get_env_var_length(const char* variable_name) {
  if (!is_valid_env_name(variable_name)) {
    return 0;
  }

  const char* value = getenv(variable_name);
  return value ? strlen(value) : 0;
}

// ==============================================================================
// CONFIGURATION FUNCTIONS
// ==============================================================================

void set_env_warnings(bool enable) {
  g_print_warnings = enable;
  debug_printf("Environment warnings %s", enable ? "enabled" : "disabled");
}

void set_env_debug(bool enable) {
  g_debug_mode = enable;
  if (enable) {
    printf("[ENV DEBUG] Debug mode enabled\n");
  }
}

// ==============================================================================
// VALIDATION AND UTILITY FUNCTIONS
// ==============================================================================

bool validate_env_requirements(const EnvRequirement* requirements, size_t count) {
  if (requirements == NULL || count == 0) {
    return true; // No requirements = always valid
  }

  bool all_valid = true;

  printf("🔍 Validating %zu environment requirements...\n", count);

  for (size_t i = 0; i < count; i++) {
    const EnvRequirement* req = &requirements[i];

    if (req->name == NULL) {
      printf("❌ Requirement %zu: Invalid name\n", i);
      all_valid = false;
      continue;
    }

    const char* value = getenv(req->name);
    bool is_set = (value != NULL);
    bool is_non_empty = (value != NULL && *value != '\0');

    // Check if required and missing
    if (req->required && !is_set) {
      printf("❌ %s: Required but not set\n", req->name);
      all_valid = false;
      continue;
    }

    // Check if required to be non-empty
    if (req->required && req->must_be_non_empty && !is_non_empty) {
      printf("❌ %s: Required to be non-empty but is empty\n", req->name);
      all_valid = false;
      continue;
    }

    // Check minimum length
    if (is_set && req->min_length > 0 && strlen(value) < req->min_length) {
      printf("❌ %s: Too short (minimum %zu characters)\n", req->name, req->min_length);
      all_valid = false;
      continue;
    }

    // Check maximum length
    if (is_set && req->max_length > 0 && strlen(value) > req->max_length) {
      printf("❌ %s: Too long (maximum %zu characters)\n", req->name, req->max_length);
      all_valid = false;
      continue;
    }

    // If we get here, the requirement is satisfied
    if (is_set) {
      if (req->sensitive) {
        printf("✅ %s: Set (***hidden***)\n", req->name);
      } else {
        printf("✅ %s: %s\n", req->name, value);
      }
    } else {
      printf("ℹ️  %s: Optional, not set\n", req->name);
    }
  }

  if (all_valid) {
    printf("✅ All environment requirements satisfied\n");
  } else {
    printf("❌ Some environment requirements failed\n");
  }

  return all_valid;
}

void print_env_summary(const char** variable_names, size_t count, bool mask_sensitive) {
  if (variable_names == NULL || count == 0) {
    printf("No environment variables to display\n");
    return;
  }

  printf("📊 Environment Variables Summary:\n");
  printf("═══════════════════════════════════════\n");

  for (size_t i = 0; i < count; i++) {
    const char* name = variable_names[i];
    if (name == NULL) continue;

    const char* value = getenv(name);

    if (value == NULL) {
      printf("   %s: (not set)\n", name);
    } else if (*value == '\0') {
      printf("   %s: (empty)\n", name);
    } else {
      bool is_sensitive = mask_sensitive && (
          strstr(name, "KEY") != NULL ||
          strstr(name, "TOKEN") != NULL ||
          strstr(name, "SECRET") != NULL ||
          strstr(name, "PASSWORD") != NULL
      );

      if (is_sensitive) {
        printf("   %s: ***masked*** (length: %zu)\n", name, strlen(value));
      } else {
        printf("   %s: %s\n", name, value);
      }
    }
  }

  printf("═══════════════════════════════════════\n");
}
