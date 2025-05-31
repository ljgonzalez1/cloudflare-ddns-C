#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

/* Structure to store response data */
struct Memory {
  char *response;
  size_t size;
};

/*
 * Callback function for libcurl to write received data into Memory structure
 * "contents" contains data, "size * nmemb" is the data length, and "userp" is pointer to Memory.
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct Memory *mem = (struct Memory *)userp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if (!ptr) {
    /* Memory allocation failed */
    return 0;
  }
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->response[mem->size] = '\0';

  return realsize;
}

int main(void) {
  CURL *curl;
  CURLcode res;
  struct Memory chunk;

  /* Initialize Memory structure */
  chunk.response = malloc(1);  /* Start with empty response */
  chunk.size = 0;

  /* Initialize libcurl globally */
  res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }

  /* Obtain a CURL handle */
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    curl_global_cleanup();
    return 1;
  }

  /* Set URL to request public IP */
  curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
  /* Use write_callback to capture response data */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  /* Optional: set a reasonable timeout (seconds) */
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  /* Perform the HTTP request */
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(chunk.response);
    return 1;
  }

  /* Print the public IP address */
  printf("%s\n", chunk.response);

  /* Clean up CURL resources */
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  free(chunk.response);

  return 0;
}
