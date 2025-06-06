// ip_getter.c - HTTPS/HTTP downloader usando libcurl
// Mucho más simple y robusto que mbedTLS
#include "ip_getter.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

/* ───── Logging macro ───── */
#define LOG(fmt, ...)  fprintf(stderr, "[ip_getter] " fmt "\n", ##__VA_ARGS__)

/* Estructura para capturar respuesta de curl */
typedef struct {
  char *data;
  size_t size;
} CurlResponse;

/* Callback para escribir datos recibidos por curl */
static size_t write_response_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  CurlResponse *response = (CurlResponse *)userp;

  char *ptr = realloc(response->data, response->size + realsize + 1);
  if (!ptr) {
    LOG("write_response_callback: realloc failed");
    return 0;
  }

  response->data = ptr;
  memcpy(&(response->data[response->size]), contents, realsize);
  response->size += realsize;
  response->data[response->size] = '\0';

  return realsize;
}

/* Inicialización global de curl (thread-safe) */
static pthread_once_t curl_once = PTHREAD_ONCE_INIT;
static void curl_global_init_wrapper(void)
{
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (res != CURLE_OK) {
    LOG("WARNING: curl_global_init failed: %s", curl_easy_strerror(res));
  } else {
    LOG("libcurl initialized successfully");
  }
}

/* Función principal para descargar URL */
char *get_url_body(const char *url, int timeout_ms)
{
  if (!url) return NULL;

  pthread_once(&curl_once, curl_global_init_wrapper);

  LOG("get_url_body: %s (timeout: %d ms)", url, timeout_ms);

  CURL *curl;
  CURLcode res;
  CurlResponse response = {0};
  char *result = NULL;

  curl = curl_easy_init();
  if (!curl) {
    LOG("get_url_body: curl_easy_init failed");
    return NULL;
  }

  // Configurar curl
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // Timeouts
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L); // 5s conexión

  // Headers HTTP
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; IPGetter/1.0)");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Auto-decompresión

  // SSL/TLS (sin verificación para máxima compatibilidad)
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  // Seguir redirects automáticamente
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

  // Habilitar debug en modo verboso (opcional)
#ifdef DEBUG_CURL
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

  // Realizar la request
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    LOG("get_url_body: curl_easy_perform failed: %s", curl_easy_strerror(res));
    goto cleanup;
  }

  // Verificar código de respuesta HTTP
  long response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  LOG("get_url_body: HTTP %ld", response_code);

  if (response_code >= 200 && response_code < 300) {
    if (response.data && response.size > 0) {
      result = strdup(response.data);
      LOG("get_url_body: success (%zu bytes)", response.size);
    } else {
      LOG("get_url_body: empty response");
    }
  } else {
    LOG("get_url_body: HTTP error %ld", response_code);
  }

  cleanup:
  curl_easy_cleanup(curl);
  if (response.data) free(response.data);

  return result;
}