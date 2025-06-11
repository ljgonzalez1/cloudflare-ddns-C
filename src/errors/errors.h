#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// Allows to combine multiple error codes into a single integer.
// ERROR_A or ERROR_B or ERROR_C can be combined using the bitwise OR operator.
// ---- ---- ---- -A-- ---- --BC ---- ----  ==>  Error A && B && C

enum error_signature {
  ERR_NONE = 0,                                           // 0x00000000u = 0000 0000 0000 0000 0000 0000 0000 0000

  ERR_INVALID_ENV = 1u << 0,                              // 0x00000001u = 0000 0000 0000 0000 0000 0000 0000 0001
  ERR_INVALID_ENV_CLOUDFLARE_KEY = 1u << 1,               // 0x00000002u = 0000 0000 0000 0000 0000 0000 0000 0010
  ERR_INVALID_ENV_DOMAINS = 1u << 2,                      // 0x00000004u = 0000 0000 0000 0000 0000 0000 0000 0100
  ERR_INVALID_ENV_PROXIED = 1u << 3,                      // 0x00000008u = 0000 0000 0000 0000 0000 0000 0000 1000
  ERR_INVALID_ENV_MINUTES_BETWEEN_UPDATES = 1u << 4,      // 0x00000010u = 0000 0000 0000 0000 0000 0000 0001 0000
  ERR_INVALID_ENV_PROPAGATION_DELAY_SECONDS = 1u << 5,    // 0x00000020u = 0000 0000 0000 0000 0000 0000 0010 0000
  ERR_INVALID_ENV_IP_V4_APIS = 1u << 6,                   // 0x00000040u = 0000 0000 0000 0000 0000 0000 0100 0000

  ERR_ALLOC_FAILURE = 1u << 7,                            // 0x00000080u = 0000 0000 0000 0000 0000 0000 1000 0000

  ERR_PARSE = 1u << 8,                                    // 0x00000100u = 0000 0000 0000 0000 0000 0001 0000 0000
};

typedef enum error_signature CombinedErrorCode;

typedef uint32_t ErrorFlags;

extern ErrorFlags g_errors;

static inline void error_set(CombinedErrorCode e) {
  g_errors |= e;
}

static inline bool error_has(CombinedErrorCode e) {
  return (g_errors & e) != 0;
}

static inline bool error_has_eny(void) {
  return g_errors!= ERR_NONE;
}

bool error_matches_any(CombinedErrorCode first, ...);

bool error_matches_all(CombinedErrorCode first, ...);

static inline void error_clear(CombinedErrorCode e) {
  g_errors &= ~e;
}

static inline void error_reset(void) {
  g_errors = ERR_NONE;
}

