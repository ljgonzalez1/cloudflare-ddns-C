#include "config_validator.h"

/**
 * @brief Validate critical configuration requirements
 *
 * Performs comprehensive validation of the loaded configuration to ensure
 * the application can operate correctly.
 *
 * @return true if configuration is valid, false otherwise
 */
bool validate_configuration(void) {
  bool is_valid = true;

  printf("🔍 Validating configuration...\n");

  is_valid = is_valid && check_environment_is_initialized();
  is_valid = is_valid && check_valid_api_key();
  is_valid = is_valid && check_valid_domains();
  is_valid = is_valid && check_valid_proxied();

  return is_valid;
}
