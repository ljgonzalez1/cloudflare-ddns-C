#pragma once

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/**
 * Converts a string to a boolean value.
 *
 * This function interprets a C-string as a boolean by:
 *   1. Returning false if `value` is NULL or consists only of whitespace.
 *   2. Trimming any leading whitespace.
 *   3. Extracting up to 15 non-whitespace characters into a temporary buffer.
 *   4. Comparing that buffer (case-sensitively) against "true", "True", or "1".
 *   5. Returning true if any of those match exactly; otherwise returning false.
 *
 * @param value
 *   A null-terminated C-string to interpret as a boolean. May be NULL.
 *
 * @return
 *   – true if the (trimmed) contents of `value` exactly match "true", "True", or "1".
 *   – false if `value` is NULL, empty/whitespace only, or any other string.
 *
 * @note
 *   – Leading whitespace in `value` is skipped before comparison.
 *   – Only the first up to 15 consecutive non-whitespace characters are examined; any trailing characters beyond the buffer size are ignored.
 *   – The comparison is case-sensitive for "true" vs. "True".
 *   – The temporary buffer `buf[16]` is allocated on the stack and must not be modified beyond its intended use.
 */
bool to_bool(const char *value);
