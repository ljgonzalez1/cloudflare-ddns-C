#include "../include/boolean_parser.h"

bool parse_str_as_bool(const char *str) {
  if (str == NULL) return false;

  if (strcasecmp(str, "true") == 0) return true;

  if (strcmp(str, "1") == 0) return true;

  return false;
}
