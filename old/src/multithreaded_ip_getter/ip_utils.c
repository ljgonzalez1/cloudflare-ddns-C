#include "ip_utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ───── Sanitizado: conserva solo [0-9.] ───── */
char *strip_noise(const char *raw)
{
  if (!raw) return NULL;

  char *out = malloc(64);
  if (!out) return NULL;

  size_t j = 0;
  for (size_t i = 0; raw[i] && j < 63; ++i) {
    char c = raw[i];
    if ((c >= '0' && c <= '9') || c == '.')
      out[j++] = c;
  }
  out[j] = '\0';

  if (j == 0) {
    free(out);
    return NULL;
  }

  // Remover puntos al inicio y final
  while (j > 0 && out[0] == '.') {
    memmove(out, out + 1, j);
    j--;
  }
  while (j > 0 && out[j-1] == '.') {
    out[--j] = '\0';
  }

  if (j == 0) {
    free(out);
    return NULL;
  }

  return out;
}

/* ───── Validación IPv4 estricta ───── */
bool is_valid_ipv4(const char *ip)
{
  if (!ip) return false;
  size_t len = strlen(ip);
  if (len < 7 || len > 15) return false;

  int segments = 0;
  int acc = 0, ch_cnt = 0;

  for (size_t i = 0; i <= len; ++i) {
    char c = ip[i];

    if (c == '.' || c == '\0') {
      if (ch_cnt == 0) return false;   /* ".." ó ".x" inicial */
      if (++segments > 4) return false;
      if (acc > 255) return false;
      acc = ch_cnt = 0;
    } else if (isdigit((unsigned char)c)) {
      acc = acc * 10 + (c - '0');
      if (++ch_cnt > 3) return false;
    } else {
      return false;                    /* carácter extraño */
    }
  }

  return segments == 4;
}

/* ───── Busca primera IPv4 válida en el texto ───── */
char *extract_first_ipv4(const char *raw)
{
  if (!raw) return NULL;

  const char *p = raw;
  while (*p) {
    // Buscar el primer dígito
    while (*p && !isdigit((unsigned char)*p)) p++;
    if (!*p) break;

    // Extraer secuencia que podría ser una IP
    const char *start = p;
    while (*p && (isdigit((unsigned char)*p) || *p == '.')) p++;

    size_t len = p - start;
    if (len >= 7 && len <= 15) {  // Longitud válida para IP
      char *candidate = strndup(start, len);
      if (candidate) {
        // Limpiar puntos al final
        while (len > 0 && candidate[len-1] == '.') {
          candidate[--len] = '\0';
        }
        // Limpiar puntos al inicio
        char *clean_start = candidate;
        while (*clean_start == '.') clean_start++;

        if (*clean_start && is_valid_ipv4(clean_start)) {
          char *result = strdup(clean_start);
          free(candidate);
          return result;
        }
        free(candidate);
      }
    }

    // Continuar desde donde nos quedamos
    if (*p) p++;
  }

  return NULL;
}