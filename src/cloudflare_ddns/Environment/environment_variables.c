#include "environment_variables.h"

// Definir la variable global Env
EnvVariables Env;

static MetaArray parse_domains(const char *raw_domains) {
  MetaArray result;

  result.arr  = NULL;
  result.size = 0;

  // No domains
  if (raw_domains == NULL || raw_domains[0] == '\0') return result;

  // Try to copy to modifiable buffer
  char *buffer = NULL;

  for (unsigned int tries = 0; tries < MAX_MALLOC_ITERATIONS && buffer == NULL; tries++)
    buffer = strdup(raw_domains);

  if (buffer == NULL) return result;

  // Calc the number of domains
  size_t count = 1;

  for (const char *pointer = buffer; *pointer; ++pointer)
    if (*pointer == ',') count++;

  // Try to create an array
  char **arr = NULL;

  for (unsigned int tries = 0; tries < MAX_MALLOC_ITERATIONS && arr == NULL; tries++)
    arr = malloc(count * sizeof(char *));

  if (arr == NULL) {
    free(buffer);
    return result;
  }

  // Replace commas by "\0"
  size_t index = 0;
  char *token = strtok(buffer, ",");

  while (token != NULL && index < count) {
    char *dup = NULL;

    for (unsigned int tries = 0; tries < MAX_MALLOC_ITERATIONS && dup == NULL; tries++)
      dup = strdup(token);

    if (dup == NULL) {
      for (size_t j = 0; j < index; j++)
        free(arr[j]);

      free(arr);
      free(buffer);

      // If it fails, return empty meta array (evitar shadowing)
      MetaArray empty_result = { .arr = NULL, .size = 0 };
      return empty_result;
    }

    // If it succeeds, save in arr[index]
    arr[index++] = dup;

    token = strtok(NULL, ",");
  }

  // Save data
  result.arr = arr;
  result.size = index;

  free(buffer);

  return result;
}

void init_env_variables(void) {  // Cambiado el nombre para coincidir con el header
  const char *proxied_str = get_env_var("PROXIED");
  Env.PROXIED = to_bool(proxied_str);  // Cambiado is_true por to_bool

  Env.CLOUDFLARE_API_KEY = (char *) get_env_var("CLOUDFLARE_API_KEY");

  MetaArray domains_data = parse_domains(get_env_var("DOMAINS"));

  Env.DOMAINS = domains_data.arr;
  Env.DOMAINS_COUNT = domains_data.size;

  // NOTE: Free the allocated memory after usage
  return;
}
