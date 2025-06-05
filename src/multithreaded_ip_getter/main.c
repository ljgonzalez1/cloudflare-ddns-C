#include "get_public_ip_multithreaded.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **split_csv(const char *csv, size_t *out_n)
{
  if (!csv || !out_n) return NULL;

  size_t commas = 0;
  for (const char *p = csv; *p; ++p) if (*p == ',') ++commas;
  size_t n = commas + 1;

  char **arr = calloc(n, sizeof *arr);
  if (!arr) return NULL;

  char *dup = strdup(csv);
  if (!dup) { free(arr); return NULL; }

  size_t i = 0;
  for (char *tok = strtok(dup, ","); tok; tok = strtok(NULL, ",")) {
    arr[i++] = strdup(tok);
  }
  *out_n = i;
  free(dup);
  return arr;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s \"url1,url2,url3\"\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t urln = 0;
  char **urls = split_csv(argv[1], &urln);
  if (!urls || urln == 0) {
    fprintf(stderr, "✗ Could not parse URLs\n");
    return EXIT_FAILURE;
  }

  char *ip = get_public_ip_multithreaded(
      (const char *const *)urls, urln);

  if (ip) {
    printf("%s\n", ip);
    free(ip);
  } else {
    fprintf(stderr, "✗ No valid IPv4 found\n");
  }

  for (size_t i = 0; i < urln; ++i) free(urls[i]);
  free(urls);
  return ip ? EXIT_SUCCESS : EXIT_FAILURE;
}
