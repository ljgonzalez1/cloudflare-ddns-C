#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "meta_array.h"

struct working_environment {
  char *CLOUDFLARE_API_KEY;
  bool PROXIED;
  size_t MINUTES_BETWEEN_UPDATES;
  size_t PROPAGATION_DELAY_SECONDS;
  MetaArray DOMAINS;
  MetaArray IP_V4_APIS;
};

typedef struct working_environment Env;

extern const Env * const env;

void env_init(void);
void env_cleanup(void);

