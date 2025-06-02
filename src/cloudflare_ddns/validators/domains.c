#include "domains.h"

bool check_valid_domains(void) {
  // Validate domains
  if (Env.DOMAINS_COUNT == 0) {
    printf("❌ No domains configured. Set DOMAINS environment variable\n");
    return false;

  } else {
    printf("✅ %zu domain(s) configured\n", Env.DOMAINS_COUNT);

    // Validate each domain (basic checks)
    for (size_t i = 0; i < Env.DOMAINS_COUNT; i++) {
      if (Env.DOMAINS[i] == NULL || strlen(Env.DOMAINS[i]) == 0) {
        printf("⚠️  Domain %zu is empty\n", i);

      } else if (strlen(Env.DOMAINS[i]) > 253) {
        printf("⚠️  Domain %zu exceeds maximum length (253 chars): %s\n", i, Env.DOMAINS[i]);
      }
    }
  }

  return true;
}
