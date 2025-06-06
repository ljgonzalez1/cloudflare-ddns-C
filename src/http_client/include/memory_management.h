/*
 * Memory Module - MIT-Compatible HTTP Client
 *
 * Provides memory-safe allocation, tracking, and cleanup functionality.
 * Includes retry mechanisms and automatic cleanup on program termination.
 */

#ifndef MEMORY_MODULE_H
#define MEMORY_MODULE_H

#include <stddef.h>
#include "../../include/settings.h"

/* Memory statistics structure */
typedef struct {
  size_t total_allocated;     /* Total bytes allocated */
  size_t current_allocated;   /* Currently allocated bytes */
  size_t peak_allocated;      /* Peak allocation */
  size_t allocation_count;    /* Number of allocations */
  size_t deallocation_count;  /* Number of deallocations */
  size_t failed_allocations;  /* Failed allocation attempts */
  size_t retry_count;         /* Number of retries performed */
} memory_stats_t;

/* Function declarations */

/* Module lifecycle */
int memory_init(void);

void memory_cleanup(void);

/* Memory allocation functions with tracking and retries */
void *memory_alloc(size_t size);

void *memory_calloc(size_t count, size_t size);

void *memory_realloc(void *ptr, size_t size);

void memory_free(void *ptr);

/* String utilities with memory tracking */
char *memory_strdup(const char *str);

char *memory_strndup(const char *str, size_t n);

/* Memory statistics and debugging */
void memory_get_stats(memory_stats_t *stats);

void memory_print_stats(void);

int memory_check_leaks(void);

/* Memory validation */
int memory_is_valid_ptr(const void *ptr);

size_t memory_get_size(const void *ptr);

/* Bulk operations */
void memory_free_all(void); /* Emergency cleanup - frees all tracked memory */

/* Configuration */
void memory_set_retry_count(int count);

void memory_set_retry_delay(int delay_us);

/* Helper macros for easier use */
#define MEMORY_NEW(type) ((type*)memory_alloc(sizeof(type)))
#define MEMORY_NEW_ARRAY(type, count) ((type*)memory_calloc((count), sizeof(type)))
#define MEMORY_FREE(ptr) do { memory_free(ptr); (ptr) = NULL; } while(0)

/* Debug-only memory filling for detecting use-after-free */
#if DEBUG_ENABLED && MEMORY_TRACKING_ENABLED
#define MEMORY_FILL_FREED 0xDE
#define MEMORY_FILL_ALLOCATED 0xAB
#else
#define MEMORY_FILL_FREED 0x00
#define MEMORY_FILL_ALLOCATED 0x00
#endif

/* Memory alignment helpers */
#define MEMORY_ALIGN_SIZE 8
#define MEMORY_ALIGN_UP(size) (((size) + MEMORY_ALIGN_SIZE - 1) & ~(MEMORY_ALIGN_SIZE - 1))

/* Error codes */
#define MEMORY_SUCCESS 0
#define MEMORY_ERROR_NULL_POINTER -1
#define MEMORY_ERROR_INVALID_SIZE -2
#define MEMORY_ERROR_OUT_OF_MEMORY -3
#define MEMORY_ERROR_DOUBLE_FREE -4
#define MEMORY_ERROR_NOT_TRACKED -5

#endif /* MEMORY_MODULE_H */
