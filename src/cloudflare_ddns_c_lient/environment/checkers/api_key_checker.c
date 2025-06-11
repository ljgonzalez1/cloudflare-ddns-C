#include "checkers.h"

static const bool basic_api_key_check(char *cloudflare_api_key) {
  bool key_looks_ok = true;

  if (cloudflare_api_key == NULL) {
    key_looks_ok = false;

  } else if (strlen(cloudflare_api_key) < MINIMUM_CLOUDFLARE_API_KEY_LENGTH) {
    key_looks_ok = false;

  } else if (strlen(cloudflare_api_key) > MAXIMUM_CLOUDFLARE_API_KEY_LENGTH) {
    key_looks_ok = false;
  }

  if (!key_looks_ok) {
    error_set(ERR_INVALID_ENV_CLOUDFLARE_KEY);
  }

  return key_looks_ok;
}
