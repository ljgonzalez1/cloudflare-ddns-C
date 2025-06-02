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

/**
 * @brief Demonstrate configuration usage
 *
 * Shows how the loaded configuration would be used in the actual DDNS operations.
 * This is primarily for testing and demonstration purposes.
 */
void demonstrate_configuration_usage(void) {
  printf("\n🚀 Configuration loaded successfully!\n");
  printf("\n");

  // Show configuration summary
  print_env_config(true);

  printf("\n");
  printf("🎯 What would happen next:\n");

  if (Env.DOMAINS_COUNT > 0) {
    printf("   1. Get current public IP address\n");
    printf("   2. For each domain:\n");

    for (size_t i = 0; i < Env.DOMAINS_COUNT; i++) {
      printf("      • Update A record for %s\n", Env.DOMAINS[i]);
      if (Env.PROXIED) {
        printf("        (with Cloudflare proxy enabled)\n");
      }
    }

    printf("   3. Verify DNS updates\n");
    printf("   4. Report results\n");
  }

  printf("\n");
  printf("💡 This is a demonstration. Actual DNS operations are not yet implemented.\n");
}

