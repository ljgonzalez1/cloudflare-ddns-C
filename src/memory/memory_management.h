#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common.h"
#include "../errors/errors.h"

typedef enum {
  ALLOC_MODE_MALLOC,
  ALLOC_MODE_CALLOC
} alloc_mode_t;

void *mm_malloc(size_t size);

void *mm_calloc(size_t nmemb, size_t size);

void mm_free(void *ptr);



