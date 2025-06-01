#include "is_true.h"

bool to_bool(const char *value) {
  if (value == NULL) return false;

  while (*value && isspace((unsigned char)*value)) value++;

  if (*value == '\0') return false;

  char buf[16];
  size_t i = 0;

  while (*value && !isspace((unsigned char)*value) && i < sizeof(buf)-1) buf[i++] = *value++;

  buf[i] = '\0';

  if (!strcmp(buf, "true") || !strcmp(buf, "True") || !strcmp(buf, "1") ) return true;

  return false;
}
