#include "../include/env.h"

static Env _env;

const Env *const env = &_env;

static const char *get_cloudflare_api_key(void) {
  const char *val = getenv(CLOUDFLARE_API_KEY_ENV_VAR);
  return val ? val : "";
}

static const bool get_proxied(void) {
  const char *val = getenv(PROXIED_ENV_VAR);

  if (val == NULL) return false;
  return strcasecmp(val, "true") == 0;
}

static const try_convert_to_int() {}

static const unsigned int get_minutes_between_updates(void) {
  const char *val = getenv(MINUTES_BETWEEN_UPDATES_ENV_VAR);
  return val == NULL? DEFAULT_MINUTES_BETWEEN_UPDATES : atoi(val);
}

void env_init(void) {
  _env.CLOUDFLARE_API_KEY = get_cloudflare_api_key();
  _env.DOMAINS = get_domains();
  _env.PROXIED = get_proxied();
  _env.MINUTES_BETWEEN_UPDATES = get_minutes_between_updates();
  _env.PROPAGATION_DELAY_SECONDS = get_propagation_delay_seconds();
  _env.IP_V4_APIS = get_ip_v4_apis();
}


void env_cleanup(void) {
  for (size_t i = 0; i < _env.DOMAINS.length; i++) {
    free(_env.DOMAINS.data[i]);
  }

  for (size_t i = 0; i < _env.IP_V4_APIS.length; i++) {
    free(_env.IP_V4_APIS.data[i]);
  }
}
