/*
 * Memory Module Implementation - MIT-Compatible HTTP Client
 *
 * Memory-safe allocation with tracking, retries, and automatic cleanup.
 */

#include "../include/memory_management.h"
#include "../../include/debug_utils.h"
#include "../include/messages.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Memory block tracking structure */
typedef struct memory_block {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  struct memory_block *next;
  struct memory_block *prev;
  unsigned int magic; /* For corruption detection */
} memory_block_t;

#define MEMORY_MAGIC 0xDEADBEEF

/* Module state */
static struct {
  int initialized;
  int cleanup_in_progress;
  memory_block_t *head;
  memory_stats_t stats;
  int retry_count;
  int retry_delay_us;
} memory_state = {
    .initialized = 0,
    .cleanup_in_progress = 0,
    .head = NULL,
    .stats = {0},
    .retry_count = MEMORY_MAX_RETRIES,
    .retry_delay_us = MEMORY_RETRY_DELAY_US
};

/* Internal functions */
static memory_block_t* find_block(const void *ptr);
static void add_block(memory_block_t *block);
static void remove_block(memory_block_t *block);
static void* retry_malloc(size_t size);
static void* retry_realloc(void *ptr, size_t size);

/* Initialize memory module */
int memory_init(void) {
  if (memory_state.initialized) {
    return MEMORY_SUCCESS;
  }

  memset(&memory_state.stats, 0, sizeof(memory_stats_t));
  memory_state.head = NULL;
  memory_state.cleanup_in_progress = 0;
  memory_state.initialized = 1;

  DEBUG_LOG(MSG_DEBUG_MODULE_INIT, "memory");

  return MEMORY_SUCCESS;
}

/* Cleanup memory module */
void memory_cleanup(void) {
  if (!memory_state.initialized) {
    return;
  }

  memory_state.cleanup_in_progress = 1;

  DEBUG_LOG(MSG_DEBUG_MODULE_CLEANUP, "memory");

  /* Check for memory leaks */
  int leak_count = memory_check_leaks();
  if (leak_count > 0) {
    DEBUG_WARN(MSG_WARN_MEMORY_LEAKS, leak_count);
  }

  /* Free all remaining blocks */
  memory_free_all();

  /* Print final statistics */
#if DEBUG_ENABLED
  memory_print_stats();
#endif

  memory_state.initialized = 0;
  DEBUG_LOG(MSG_SUCCESS_CLEANUP_COMPLETED);
}

/* Safe malloc with retries */
void* memory_alloc(size_t size) {
  if (!memory_state.initialized) {
    FATAL_ERROR(MSG_FATAL_MODULE_NOT_INIT, "Memory");
    return NULL;
  }

  if (size == 0) {
    DEBUG_WARN("Attempted to allocate 0 bytes");
    return NULL;
  }

  if (memory_state.cleanup_in_progress) {
    DEBUG_WARN("Memory allocation during cleanup");
    return NULL;
  }

  /* Try allocation with retries */
  void *ptr = retry_malloc(size);
  if (!ptr) {
    memory_state.stats.failed_allocations++;
    DEBUG_ERROR("Memory allocation failed after %d retries (size: %zu)",
                memory_state.retry_count, size);
    return NULL;
  }

  /* Fill with debug pattern */
#if DEBUG_ENABLED && MEMORY_TRACKING_ENABLED
  memset(ptr, MEMORY_FILL_ALLOCATED, size);
#endif

  /* Track allocation if enabled */
#if MEMORY_TRACKING_ENABLED
  memory_block_t *block = malloc(sizeof(memory_block_t));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->file = __FILE__;
        block->line = __LINE__;
        block->magic = MEMORY_MAGIC;
        add_block(block);

        /* Update statistics */
        memory_state.stats.total_allocated += size;
        memory_state.stats.current_allocated += size;
        memory_state.stats.allocation_count++;

        if (memory_state.stats.current_allocated > memory_state.stats.peak_allocated) {
            memory_state.stats.peak_allocated = memory_state.stats.current_allocated;
        }

        DEBUG_MALLOC(ptr, size);
    }
#endif

  return ptr;
}

/* Safe calloc */
void* memory_calloc(size_t count, size_t size) {
  if (count == 0 || size == 0) {
    return NULL;
  }

  /* Check for overflow */
  if (count > SIZE_MAX / size) {
    DEBUG_ERROR("Integer overflow in calloc(%zu, %zu)", count, size);
    return NULL;
  }

  size_t total_size = count * size;
  void *ptr = memory_alloc(total_size);

  if (ptr) {
    memset(ptr, 0, total_size);
  }

  return ptr;
}

/* Safe realloc with retries */
void* memory_realloc(void *ptr, size_t size) {
  if (!memory_state.initialized) {
    FATAL_ERROR(MSG_FATAL_MODULE_NOT_INIT, "Memory");
    return NULL;
  }

  /* Handle special cases */
  if (!ptr) {
    return memory_alloc(size);
  }

  if (size == 0) {
    memory_free(ptr);
    return NULL;
  }

  if (memory_state.cleanup_in_progress) {
    DEBUG_WARN("Memory reallocation during cleanup");
    return NULL;
  }

  /* Find existing block */
#if MEMORY_TRACKING_ENABLED
  memory_block_t *block = find_block(ptr);
    size_t old_size = block ? block->size : 0;
#else
  size_t old_size = 0;
#endif

  /* Try reallocation with retries */
  void *new_ptr = retry_realloc(ptr, size);
  if (!new_ptr) {
    memory_state.stats.failed_allocations++;
    DEBUG_ERROR("Memory reallocation failed (old_size: %zu, new_size: %zu)",
                old_size, size);
    return NULL;
  }

  /* Update tracking if enabled */
#if MEMORY_TRACKING_ENABLED
  if (block) {
        /* Remove old block */
        remove_block(block);
        memory_state.stats.current_allocated -= old_size;

        /* Add new block */
        block->ptr = new_ptr;
        block->size = size;
        add_block(block);
        memory_state.stats.current_allocated += size;

        if (memory_state.stats.current_allocated > memory_state.stats.peak_allocated) {
            memory_state.stats.peak_allocated = memory_state.stats.current_allocated;
        }

        DEBUG_REALLOC(ptr, new_ptr, size);
    }
#endif

  return new_ptr;
}

/* Safe free */
void memory_free(void *ptr) {
  if (!ptr) {
    return; /* Allow freeing NULL pointers */
  }

  if (!memory_state.initialized) {
    FATAL_ERROR(MSG_FATAL_MODULE_NOT_INIT, "Memory");
    return;
  }

#if MEMORY_TRACKING_ENABLED
  /* Find and remove block */
    memory_block_t *block = find_block(ptr);
    if (!block) {
        DEBUG_ERROR("Attempted to free untracked pointer: %p", ptr);
        return;
    }

    /* Check for corruption */
    if (block->magic != MEMORY_MAGIC) {
        DEBUG_ERROR("Memory corruption detected in block: %p", ptr);
        return;
    }

    /* Fill with debug pattern */
#if DEBUG_ENABLED
    memset(ptr, MEMORY_FILL_FREED, block->size);
#endif

    /* Update statistics */
    memory_state.stats.current_allocated -= block->size;
    memory_state.stats.deallocation_count++;

    DEBUG_FREE(ptr);

    /* Remove from tracking */
    remove_block(block);
    free(block);
#endif

  /* Free the actual memory */
  free(ptr);
}

/* String duplication with tracking */
char* memory_strdup(const char *str) {
  if (!str) {
    return NULL;
  }

  size_t len = strlen(str) + 1;
  char *copy = memory_alloc(len);

  if (copy) {
    memcpy(copy, str, len);
  }

  return copy;
}

/* String duplication with length limit */
char* memory_strndup(const char *str, size_t n) {
  if (!str) {
    return NULL;
  }

  size_t len = strnlen(str, n);
  char *copy = memory_alloc(len + 1);

  if (copy) {
    memcpy(copy, str, len);
    copy[len] = '\0';
  }

  return copy;
}

/* Get memory statistics */
void memory_get_stats(memory_stats_t *stats) {
  if (stats && memory_state.initialized) {
    *stats = memory_state.stats;
  }
}

/* Print memory statistics */
void memory_print_stats(void) {
  if (!memory_state.initialized) {
    return;
  }

  DEBUG_INFO(MSG_DEBUG_MEMORY_STATS,
             memory_state.stats.total_allocated,
             memory_state.stats.current_allocated,
             memory_state.stats.peak_allocated);
  DEBUG_INFO("  Allocations: %zu", memory_state.stats.allocation_count);
  DEBUG_INFO("  Deallocations: %zu", memory_state.stats.deallocation_count);
  DEBUG_INFO("  Failed allocations: %zu", memory_state.stats.failed_allocations);
  DEBUG_INFO("  Retries performed: %zu", memory_state.stats.retry_count);
}

/* Check for memory leaks */
int memory_check_leaks(void) {
  if (!memory_state.initialized) {
    return 0;
  }

#if MEMORY_TRACKING_ENABLED
  int leak_count = 0;
    memory_block_t *current = memory_state.head;

    while (current) {
        DEBUG_WARN(MSG_DEBUG_MEMORY_LEAK,
                   current->size, current->ptr, current->file, current->line);
        leak_count++;
        current = current->next;
    }

    return leak_count;
#else
  return 0;
#endif
}

/* Emergency cleanup - free all tracked memory */
void memory_free_all(void) {
  if (!memory_state.initialized) {
    return;
  }

#if MEMORY_TRACKING_ENABLED
  memory_block_t *current = memory_state.head;

    while (current) {
        memory_block_t *next = current->next;

        if (current->ptr) {
#if DEBUG_ENABLED
            memset(current->ptr, MEMORY_FILL_FREED, current->size);
#endif
            free(current->ptr);
        }

        free(current);
        current = next;
    }

    memory_state.head = NULL;
    memory_state.stats.current_allocated = 0;
#endif
}

/* Configuration functions */
void memory_set_retry_count(int count) {
  if (count >= 0) {
    memory_state.retry_count = count;
  }
}

void memory_set_retry_delay(int delay_us) {
  if (delay_us >= 0) {
    memory_state.retry_delay_us = delay_us;
  }
}

/* Internal helper functions */
static memory_block_t* find_block(const void *ptr) {
#if MEMORY_TRACKING_ENABLED
  memory_block_t *current = memory_state.head;

    while (current) {
        if (current->ptr == ptr) {
            return current;
        }
        current = current->next;
    }
#endif

  return NULL;
}

static void add_block(memory_block_t *block) {
#if MEMORY_TRACKING_ENABLED
  block->next = memory_state.head;
    block->prev = NULL;

    if (memory_state.head) {
        memory_state.head->prev = block;
    }

    memory_state.head = block;
#endif
}

static void remove_block(memory_block_t *block) {
#if MEMORY_TRACKING_ENABLED
  if (block->prev) {
        block->prev->next = block->next;
    } else {
        memory_state.head = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }
#endif
}

static void* retry_malloc(size_t size) {
  void *ptr = NULL;

  for (int i = 0; i <= memory_state.retry_count && !ptr; i++) {
    ptr = malloc(size);

    if (!ptr && i < memory_state.retry_count) {
      memory_state.stats.retry_count++;
      DEBUG_TRACE("malloc failed, retrying (%d/%d)", i + 1, memory_state.retry_count);
      usleep((unsigned int)memory_state.retry_delay_us);
    }
  }

  return ptr;
}

static void* retry_realloc(void *ptr, size_t size) {
  void *new_ptr = NULL;

  for (int i = 0; i <= memory_state.retry_count && !new_ptr; i++) {
    new_ptr = realloc(ptr, size);

    if (!new_ptr && i < memory_state.retry_count) {
      memory_state.stats.retry_count++;
      DEBUG_TRACE("realloc failed, retrying (%d/%d)", i + 1, memory_state.retry_count);
      usleep((unsigned int)memory_state.retry_delay_us);
    }
  }

  return new_ptr;
}