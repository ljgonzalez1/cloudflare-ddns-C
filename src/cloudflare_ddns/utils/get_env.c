#include "get_env.h"

const char *get_env_var(const char *variable_name) {
  const char *var_value = getenv(variable_name);

  if (var_value == NULL) {
    fprintf(stderr, "Environment variable `%s` not defined.\n", variable_name);

    return "";

  }

  return var_value;
}
