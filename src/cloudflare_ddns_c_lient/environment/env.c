#include "../include/env.h"

static Env _env;

const Env *const env = &_env;

static const char *get_cloudflare_api_key(void) {
  const char *val = getenv("CLOUDFLARE_API_KEY");
  return val ? val : "";
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
