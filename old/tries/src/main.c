#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <curl/curl.h>

#define CA_BUNDLE_URL "https://curl.se/ca/cacert.pem"
#define MAX_RETRIES 3
#define MAX_MEMORY_RETRIES 5

// Messages
#define MSG_DOWNLOADING_CERTS "🔗 Downloading CA certificates...\n"
#define MSG_CERTS_SUCCESS "✅ CA certificates loaded into memory\n"
#define MSG_REQUEST_START "🚀 Making request to: %s\n"
#define MSG_RESPONSE_CODE "📊 HTTP Status: %ld\n"
#define MSG_RESPONSE_SIZE "📏 Response size: %lu bytes\n"
#define MSG_SUCCESS_COMPLETE "✅ Request completed successfully\n"

#define ERR_NO_MEMORY "💥 Error: Cannot allocate memory after %d retries\n"
#define ERR_NO_URL "❌ Usage: %s <URL>\n"
#define ERR_CURL_INIT "🔧 Error: Cannot initialize curl\n"
#define ERR_CURL_GLOBAL_INIT "🔧 Error: Cannot initialize libcurl globally\n"
#define ERR_REQUEST_FAILED "❌ Request failed: %s\n"
#define ERR_NO_CERTS "❌ Error: Could not download certificates\n"

// Memory management
typedef struct memory_block {
  void *ptr;
  size_t size;
  struct memory_block *next;
} memory_block_t;

static memory_block_t *global_memory_list = NULL;
static volatile int cleanup_in_progress = 0;
static char *ca_bundle_data = NULL;
static size_t ca_bundle_size = 0;

// HTTP response structure
struct HttpResponse {
  char *memory;
  size_t size;
};

// Memory tracking
static void track_memory(void *ptr, size_t size) {
  if (!ptr || cleanup_in_progress) return;

  memory_block_t *block = malloc(sizeof(memory_block_t));
  if (!block) return;

  block->ptr = ptr;
  block->size = size;
  block->next = global_memory_list;
  global_memory_list = block;
}

static void untrack_memory(void *ptr) {
  if (!ptr || cleanup_in_progress) return;

  memory_block_t **current = &global_memory_list;
  while (*current) {
    if ((*current)->ptr == ptr) {
      memory_block_t *to_delete = *current;
      *current = (*current)->next;
      free(to_delete);
      return;
    }
    current = &(*current)->next;
  }
}

// Safe memory allocation
static void* safe_malloc(size_t size) {
  void *ptr = NULL;
  int i;

  if (cleanup_in_progress) return NULL;

  for (i = 0; ptr == NULL && i < MAX_MEMORY_RETRIES; i++) {
    ptr = malloc(size);
    if (ptr == NULL && i < MAX_MEMORY_RETRIES - 1) {
      usleep(100000);
    }
  }

  if (ptr == NULL) {
    printf(ERR_NO_MEMORY, MAX_MEMORY_RETRIES);
    exit(1);
  }

  track_memory(ptr, size);
  return ptr;
}

static void* safe_realloc(void *old_ptr, size_t size) {
  void *ptr = NULL;
  int i;

  if (cleanup_in_progress) return NULL;

  for (i = 0; ptr == NULL && i < MAX_MEMORY_RETRIES; i++) {
    ptr = realloc(old_ptr, size);
    if (ptr == NULL && i < MAX_MEMORY_RETRIES - 1) {
      usleep(100000);
    }
  }

  if (ptr == NULL) {
    printf("💥 Error: Realloc failed after retries\n");
    exit(1);
  }

  if (old_ptr != ptr) {
    untrack_memory(old_ptr);
    track_memory(ptr, size);
  }

  return ptr;
}

static void safe_free(void *ptr) {
  if (!ptr || cleanup_in_progress) return;
  untrack_memory(ptr);
  free(ptr);
}

// Cleanup all memory
static void cleanup_all_memory(void) {
  cleanup_in_progress = 1;

  memory_block_t *current = global_memory_list;
  while (current) {
    memory_block_t *next = current->next;
    if (current->ptr) {
      free(current->ptr);
    }
    free(current);
    current = next;
  }
  global_memory_list = NULL;

  if (ca_bundle_data) {
    free(ca_bundle_data);
    ca_bundle_data = NULL;
    ca_bundle_size = 0;
  }
}

// Signal handler
static void signal_handler(int signum) {
  printf("🧹 Received signal %d, cleaning up...\n", signum);
  cleanup_all_memory();
  curl_global_cleanup();
  exit(0);
}

// Install signal handlers
static void install_signal_handlers(void) {
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
}

// Callback to write data to memory
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct HttpResponse *response = (struct HttpResponse *)userp;

  if (cleanup_in_progress) return 0;

  char *ptr = safe_realloc(response->memory, response->size + realsize + 1);

  response->memory = ptr;
  memcpy(&(response->memory[response->size]), contents, realsize);
  response->size += realsize;
  response->memory[response->size] = 0;

  return realsize;
}

// Download CA certificates
static int download_ca_bundle(void) {
  CURL *curl;
  CURLcode res;
  struct HttpResponse chunk;
  int retries = 0;

  printf(MSG_DOWNLOADING_CERTS);

  while (retries < MAX_RETRIES) {
    chunk.memory = safe_malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (!curl) {
      safe_free(chunk.memory);
      printf(ERR_CURL_INIT);
      return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, CA_BUNDLE_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "StaticHTTPClient/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && chunk.size > 1000) {
      ca_bundle_data = chunk.memory;
      ca_bundle_size = chunk.size;
      printf(MSG_CERTS_SUCCESS);
      return 1;
    } else {
      safe_free(chunk.memory);
      printf("⚠️  Certificate download attempt %d/%d failed\n", retries + 1, MAX_RETRIES);
    }

    retries++;
    if (retries < MAX_RETRIES) {
      printf("⏳ Retrying in 2 seconds...\n");
      sleep(2);
    }
  }

  printf(ERR_NO_CERTS);
  return 0;
}

// Make HTTP request
static int make_http_request(const char *url) {
  CURL *curl;
  CURLcode res;
  struct HttpResponse response;
  long http_code;
  struct curl_blob ca_blob;

  if (!ca_bundle_data || ca_bundle_size == 0) {
    if (!download_ca_bundle()) {
      return 0;
    }
  }

  response.memory = safe_malloc(1);
  response.size = 0;

  curl = curl_easy_init();
  if (!curl) {
    safe_free(response.memory);
    printf(ERR_CURL_INIT);
    return 0;
  }

  // Setup CA certificates from memory
  ca_blob.data = ca_bundle_data;
  ca_blob.len = ca_bundle_size;
  ca_blob.flags = CURL_BLOB_COPY;

  // Configure request
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "StaticHTTPClient/1.0");
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &ca_blob);

  printf(MSG_REQUEST_START, url);
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    printf(ERR_REQUEST_FAILED, curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    safe_free(response.memory);
    return 0;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  printf(MSG_RESPONSE_CODE, http_code);
  printf(MSG_RESPONSE_SIZE, (unsigned long)response.size);

  // Print response
  if (response.size > 0) {
    printf("\n--- Response ---\n");
    printf("%s", response.memory);
    printf("\n--- End Response ---\n");
  }

  curl_easy_cleanup(curl);
  safe_free(response.memory);
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf(ERR_NO_URL, argv[0]);
    printf("Example: %s https://www.google.com\n", argv[0]);
    return 1;
  }

  install_signal_handlers();

  if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
    printf(ERR_CURL_GLOBAL_INIT);
    return 1;
  }

  int success = make_http_request(argv[1]);

  curl_global_cleanup();
  cleanup_all_memory();

  if (success) {
    printf("\n%s", MSG_SUCCESS_COMPLETE);
    return 0;
  } else {
    printf("\n❌ Request failed\n");
    return 1;
  }
}
