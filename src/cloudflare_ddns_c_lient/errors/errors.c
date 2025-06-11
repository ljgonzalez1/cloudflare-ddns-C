#include "../include/errors.h"

ErrorFlags g_errors = ERR_NONE;

bool error_matches_any(CombinedErrorCode first, ...) {
  va_list ap;

  CombinedErrorCode code = first;

  va_start(ap, first);

  while (code != ERR_NONE) {
    if (g_errors & code) {
      va_end(ap);
      return true;
    }

    code = va_arg(ap, CombinedErrorCode);
  }

  va_end(ap);

  return false;
}

bool error_matches_all(CombinedErrorCode first, ...)
{
  va_list ap;
  CombinedErrorCode code = first;
  va_start(ap, first);

  while (code != ERR_NONE) {
    if ((g_errors & code) == 0) {
      va_end(ap);
      return false;
    }
    code = va_arg(ap, CombinedErrorCode);
  }

  va_end(ap);
  return true;
}