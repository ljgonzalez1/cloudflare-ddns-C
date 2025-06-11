#include "memory_management.h"

static void *try_alloc(alloc_mode_t mode, size_t arg0, size_t arg1) {
  void *ptr = NULL;

  for (unsigned int i = 0; ptr == NULL && i < MAX_MALLOC_RETRIES; i++) {
    if (mode == ALLOC_MODE_MALLOC) {
      ptr = malloc(arg0);
    } else {
      ptr = calloc(arg0, arg1);
    }
  }

  if (ptr == NULL) error_set(ERR_ALLOC_FAILURE);

  return ptr;
}

void *mm_malloc(size_t size) {
  return try_alloc(ALLOC_MODE_MALLOC, size, 0);
}

void *mm_calloc(size_t nmemb, size_t size) {
  return try_alloc(ALLOC_MODE_CALLOC, nmemb, size);
}

void mm_free(void *ptr) {
  free(ptr);
}
