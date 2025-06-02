#include "envitonment_initialized.h"

bool check_environment_is_initialized(void) {
  // Check if environment was initialized properly
  if (!is_env_initialized()) {
    printf("❌ Environment variables not properly initialized\n");
    return false;
  }

  return true;
}