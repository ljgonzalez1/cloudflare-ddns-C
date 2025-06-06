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
    // Trim whitespace
    while (*tok == ' ' || *tok == '\t') tok++;
    char *end = tok + strlen(tok) - 1;
    while (end > tok && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
      *end = '\0';
      end--;
    }
    if (*tok) {  // Only add non-empty URLs
      arr[i++] = strdup(tok);
    }
  }
  *out_n = i;
  free(dup);
  return arr;
}

int main(int argc, char *argv[])
{
  // URLs de servicios confiables de IP pública
  const char *default_urls = "https://api.ipify.org/,https://ipv4.icanhazip.com/,https://icanhazip.com/,https://checkip.amazonaws.com/";

  const char *urls_str = (argc == 2) ? argv[1] : default_urls;

  if (argc > 2) {
    fprintf(stderr, "Usage: %s [\"url1,url2,url3\"]\n", argv[0]);
    fprintf(stderr, "Default URLs: %s\n", default_urls);
    return EXIT_FAILURE;
  }

  printf("🌐 Multithreaded IP Getter (libcurl version)\n");
  printf("URLs: %s\n\n", urls_str);

  size_t urln = 0;
  char **urls = split_csv(urls_str, &urln);
  if (!urls || urln == 0) {
    fprintf(stderr, "❌ Could not parse URLs\n");
    return EXIT_FAILURE;
  }

  printf("Parsed %zu URLs:\n", urln);
  for (size_t i = 0; i < urln; i++) {
    printf("  [%zu] %s\n", i, urls[i]);
  }
  printf("\n");

  char *ip = get_public_ip_multithreaded(
      (const char *const *)urls, urln);

  if (ip) {
    printf("\n✅ SUCCESS: Your public IP is %s\n", ip);
    free(ip);
  } else {
    fprintf(stderr, "\n❌ ERROR: Could not determine public IP\n");
  }

  for (size_t i = 0; i < urln; ++i) free(urls[i]);
  free(urls);
  return ip ? EXIT_SUCCESS : EXIT_FAILURE;
}