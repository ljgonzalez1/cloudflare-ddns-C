#pragma once

#include <stdint.h>
#include <stdbool.h>

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
  ERR_FREE_FAILURE = 1u << 8,                             // 0x00000100u = 0000 0000 0000 0000 0000 0001 0000 0000

  ERR_PARSE = 1u << 9,                                    // 0x00000200u = 0000 0000 0000 0000 0000 0010 0000 0000

  ERR_INTERNAL = 1u << 10,                                // 0x00000400u = 0000 0000 0000 0000 0000 0100 0000 0000

  ERR_HTTP_REQUEST = 1u << 11,                            // 0x00000800u = 0000 0000 0000 0000 0000 1000 0000 0000
  ERR_HTTP_RESPONSE = 1u << 12,                           // 0x00001000u = 0000 0000 0000 0000 0001 0000 0000 0000

  ERR_CLOUDFLARE_API = 1u << 13,                          // 0x00002000u = 0000 0000 0000 0000 0010 0000 0000 0000
  ERR_CLOUDFLARE_RATE_LIMIT = 1u << 14,                   // 0x00004000u = 0000 0000 0000 0000 0100 0000 0000 0000
  ERR_CLOUDFLARE_AUTH_FAILURE = 1u << 15,                 // 0x00008000u = 0000 0000 0000 0000 1000 0000 0000 0000

  ERR_IP_ADDRESS_RESOLUTION = 1u << 16,                   // 0x00010000u = 0000 0000 0000 0001 0000 0000 0000 0000

  ERR__SPACING__1 = 1u << 17,  // Spacing for future error codes  // 0x00020000u = 0000 0000 0000 0010 0000 0000 0000 0000
  ERR__SPACING__2 = 1u << 18,  // Spacing for future error codes  // 0x00040000u = 0000 0000 0000 0100 0000 0000 0000 0000
  ERR__SPACING__3 = 1u << 19,  // Spacing for future error codes  // 0x00080000u = 0000 0000 0000 1000 0000 0000 0000 0000
  ERR__SPACING__4 = 1u << 20,  // Spacing for future error codes  // 0x00100000u = 0000 0000 0001 0000 0000 0000 0000 0000
  ERR__SPACING__5 = 1u << 21,  // Spacing for future error codes  // 0x00200000u = 0000 0000 0010 0000 0000 0000 0000 0000
  ERR__SPACING__6 = 1u << 22,  // Spacing for future error codes  // 0x00400000u = 0000 0000 0100 0000 0000 0000 0000 0000
  ERR__SPACING__7 = 1u << 23,  // Spacing for future error codes  // 0x00800000u = 0000 0000 1000 0000 0000 0000 0000 0000
  ERR__SPACING__8 = 1u << 24,  // Spacing for future error codes  // 0x01000000u = 0000 0001 0000 0000 0000 0000 0000 0000
  ERR__SPACING__9 = 1u << 25,  // Spacing for future error codes  // 0x02000000u = 0000 0010 0000 0000 0000 0000 0000 0000
  ERR__SPACING__10 = 1u << 26, // Spacing for future error codes  // 0x04000000u = 0000 0100 0000 0000 0000 0000 0000 0000
  ERR__SPACING__11 = 1u << 27, // Spacing for future error codes  // 0x08000000u = 0000 1000 0000 0000 0000 0000 0000 0000
  ERR__SPACING__12 = 1u << 28, // Spacing for future error codes  // 0x10000000u = 0001 0000 0000 0000 0000 0000 0000 0000
  ERR__SPACING__13 = 1u << 29, // Spacing for future error codes  // 0x20000000u = 0010 0000 0000 0000 0000 0000 0000 0000
  ERR__SPACING__14 = 1u << 30, // Spacing for future error codes  // 0x40000000u = 0100 0000 0000 0000 0000 0000 0000 0000
  ERR__SPACING__15 = 1u << 31  // Spacing for future error codes  // 0x80000000u = 1000 0000 0000 0000 0000 0000 0000 0000
};

typedef enum error_signature CombinedErrorCode;

typedef uint32_t ErrorFlags;

extern ErrorFlags g_errors;

static inline void  error_set (CombinedErrorCode e) {
  g_errors |=  e;
}

static inline bool  error_has (CombinedErrorCode e) {
  return (g_errors & e) != 0;
}

static inline void  error_clear (CombinedErrorCode e) {
  g_errors &= ~e;
}

static inline void  error_reset (void) {
  g_errors = ERR_NONE;
}

