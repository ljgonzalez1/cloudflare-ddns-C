#include "help.h"

/**
 * @brief Print required environment variables help
 */
void print_environment_help(void) {
  printf("📋 Required Environment Variables:\n");
  printf("   CLOUDFLARE_API_KEY    - Your Cloudflare API token\n");
  printf("   DOMAINS              - Comma-separated domain list (e.g., 'example.com,test.org')\n");
  printf("\n");
  printf("🔧 Optional Environment Variables:\n");
  printf("   PROXIED              - Enable Cloudflare proxy ('true'/'false', default: false)\n");
  printf("\n");
  printf("💡 Example setup:\n");
  printf("   export CLOUDFLARE_API_KEY=\"your_api_token_here\"\n");
  printf("   export DOMAINS=\"example.com,subdomain.example.com\"\n");
  printf("   export PROXIED=\"true\"\n");
  printf("   ./cloudflare_ddns\n");
  printf("\n");
}
