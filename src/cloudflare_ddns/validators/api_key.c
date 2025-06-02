#include "api_key.h"

bool check_valid_api_key(void) {
  // Validate API key
  if (Env.CLOUDFLARE_API_KEY == NULL || strlen(Env.CLOUDFLARE_API_KEY) == 0) {
    printf("❌ CLOUDFLARE_API_KEY is required but not set\n");
    return false;

  } else if (strlen(Env.CLOUDFLARE_API_KEY) < 10) {
    printf("⚠️  CLOUDFLARE_API_KEY seems too short (minimum 10 characters expected)\n");
    // Continue anyway, might be a test token

  } else {
    printf("✅ API key configured\n");
  }

  return true;
}
