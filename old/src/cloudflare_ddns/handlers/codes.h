#pragma once

/**
 * @brief Exit codes for different scenarios
 */
typedef enum {
  EXIT_SUCCESS_CODE = 0,          ///< Application completed successfully
  EXIT_CONFIG_ERROR = 1,          ///< Configuration/environment error
  EXIT_VALIDATION_ERROR = 2,      ///< Validation failed
  EXIT_MEMORY_ERROR = 3           ///< Memory allocation error
} ExitCode;

