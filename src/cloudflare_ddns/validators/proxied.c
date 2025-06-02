#include "proxied.h"

bool check_valid_proxied(void) {
  // Check proxy setting (informational)
  printf("ℹ️  Proxy mode: %s\n", Env.PROXIED ? "enabled" : "disabled");

  return true;
}
