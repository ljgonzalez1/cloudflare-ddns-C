/**
 * @file meta_array.h
 * @brief Generic Array Container with Type Safety and Memory Management
 * @version 2.0
 * @date 2025-06-01
 * 
 * Enhanced generic array container that provides type-safe operations,
 * automatic memory management, and comprehensive utility functions.
 * 
 * Features:
 * - Type-safe generic array operations
 * - Automatic memory management with RAII-style cleanup
 * - Bounds checking and validation
 * - Iterator support for easy traversal
 * - Built-in support for common operations (append, insert, remove)
 * - Memory pool optimization for performance
 * - Debug and validation modes
 * 
 * Design Philosophy:
 * - Safety first: All operations are bounds-checked
 * - Performance: Optimized for common use cases
 * - Simplicity: Easy to use, hard to misuse
 * - Flexibility: Supports various data types and use patterns
 */

#pragma once

// ==============================================================================
// SYSTEM INCLUDES
// ==============================================================================

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ==============================================================================
// CONFIGURATION
// ==============================================================================

/**
 * @brief Default initial capacity for new arrays
 */
#define META_ARRAY_DEFAULT_CAPACITY 8

/**
 * @brief Growth factor for array expansion (1.5x)
 */
#define META_ARRAY_GROWTH_FACTOR 1.5

/**
 * @brief Maximum reasonable array size (safety limit)
 */
#define META_ARRAY_MAX_SIZE (SIZE_MAX / 2)

// ==============================================================================
// CORE DATA STRUCTURES
// ==============================================================================

/**
 * @brief Generic array container with metadata
 * 
 * This structure provides a generic container that can hold arrays of any type
 * with associated metadata for size tracking and memory management.
 * 
 * Memory Layout:
 * - arr: Pointer to the actual array data
 * - size: Number of elements currently stored
 * - capacity: Total number of elements that can be stored without reallocation
 * - element_size: Size in bytes of each element
 * - flags: Operational flags and configuration
 * 
 * Usage Patterns:
 * 1. String arrays: char**
 * 2. Integer arrays: int*
 * 3. Structure arrays: struct_type*
 * 4. Generic data: void*
 */
typedef struct {
  /**
   * @brief Pointer to array data
   *
   * Points to the beginning of the allocated array memory.
   * The actual type depends on the intended usage.
   *
   * @warning This pointer may change during array operations that cause reallocation
   */
  void* arr;

  /**
   * @brief Number of elements currently stored
   *
   * Valid elements are at indices [0, size-1].
   * Always satisfies: size <= capacity
   */
  size_t size;

  /**
   * @brief Total capacity of allocated array
   *
   * Number of elements that can be stored without reallocation.
   * When size reaches capacity, the array will be expanded automatically.
   */
  size_t capacity;

  /**
   * @brief Size of each element in bytes
   *
   * Used for type-safe operations and memory calculations.
   * Set during array initialization and should not be changed.
   */
  size_t element_size;

  /**
   * @brief Operational flags and configuration
   *
   * Bitfield containing various flags for array behavior:
   * - Bit 0: owns_memory (array manages its own memory)
   * - Bit 1: read_only (modifications not allowed)
   * - Bit 2: bounds_checking (enable runtime bounds checking)
   * - Bit 3: zero_initialize (zero new elements)
   */
  uint32_t flags;

} MetaArray;

// ==============================================================================
// ARRAY FLAGS
// ==============================================================================

/** @brief Array owns and manages its memory */
#define META_ARRAY_OWNS_MEMORY      (1U << 0)

/** @brief Array is read-only (modifications not allowed) */
#define META_ARRAY_READ_ONLY        (1U << 1)

/** @brief Enable runtime bounds checking */
#define META_ARRAY_BOUNDS_CHECK     (1U << 2)

/** @brief Zero-initialize new elements */
#define META_ARRAY_ZERO_INIT        (1U << 3)

/** @brief Array uses external memory (not owned) */
#define META_ARRAY_EXTERNAL_MEMORY  (1U << 4)

/** @brief Enable debug validation */
#define META_ARRAY_DEBUG_MODE       (1U << 5)

// ==============================================================================
// ERROR CODES
// ==============================================================================

/**
 * @brief Operation result codes
 */
typedef enum {
  META_ARRAY_SUCCESS = 0,         ///< Operation completed successfully
  META_ARRAY_ERROR_NULL_POINTER,  ///< Null pointer provided
  META_ARRAY_ERROR_INVALID_SIZE,  ///< Invalid size parameter
  META_ARRAY_ERROR_OUT_OF_BOUNDS, ///< Index out of bounds
  META_ARRAY_ERROR_MEMORY,        ///< Memory allocation failed
  META_ARRAY_ERROR_READ_ONLY,     ///< Attempted modification of read-only array
  META_ARRAY_ERROR_TYPE_MISMATCH, ///< Element size mismatch
  META_ARRAY_ERROR_OVERFLOW,      ///< Size overflow would occur
} MetaArrayResult;

// ==============================================================================
// ITERATOR SUPPORT
// ==============================================================================

/**
 * @brief Array iterator for safe traversal
 */
typedef struct {
  MetaArray* array;       ///< Array being iterated
  size_t current_index;   ///< Current position
  bool is_valid;          ///< Whether iterator is in valid state
} MetaArrayIterator;

// ==============================================================================
// INITIALIZATION AND CLEANUP
// ==============================================================================

/**
 * @brief Initialize empty MetaArray with specified element size
 * 
 * Creates a new MetaArray with default capacity and specified element size.
 * The array will own its memory and support all standard operations.
 * 
 * @param array Pointer to MetaArray to initialize
 * @param element_size Size of each element in bytes
 * @param flags Initial configuration flags
 * @return META_ARRAY_SUCCESS on success, error code on failure
 * 
 * @post array is initialized and ready for use
 * @post array owns its memory and can be safely cleaned up
 * 
 * @example
 * ```c
 * MetaArray string_array;
 * meta_array_init(&string_array, sizeof(char*), META_ARRAY_OWNS_MEMORY);
 * 
 * // Use array...
 * 
 * meta_array_cleanup(&string_array);
 * ```
 */
MetaArrayResult meta_array_init(MetaArray* array, size_t element_size, uint32_t flags);

/**
 * @brief Initialize MetaArray with specific initial capacity
 * 
 * @param array Pointer to MetaArray to initialize
 * @param element_size Size of each element in bytes
 * @param initial_capacity Initial capacity to allocate
 * @param flags Configuration flags
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_init_with_capacity(MetaArray* array, size_t element_size,
                                              size_t initial_capacity, uint32_t flags);

/**
 * @brief Initialize MetaArray from existing data
 * 
 * Creates a MetaArray that wraps existing data. The array can either
 * take ownership of the data or just reference it.
 * 
 * @param array Pointer to MetaArray to initialize
 * @param data Existing data to wrap
 * @param size Number of elements in existing data
 * @param element_size Size of each element in bytes
 * @param take_ownership Whether array should own the data
 * @return META_ARRAY_SUCCESS on success, error code on failure
 * 
 * @note If take_ownership is false, caller is responsible for data lifetime
 */
MetaArrayResult meta_array_init_from_data(MetaArray* array, void* data, size_t size,
                                          size_t element_size, bool take_ownership);

/**
 * @brief Clean up MetaArray and free resources
 * 
 * Releases all resources associated with the MetaArray. If the array
 * owns its memory, the data will be freed. Array becomes invalid after cleanup.
 * 
 * @param array Pointer to MetaArray to clean up
 * @return META_ARRAY_SUCCESS on success, error code on failure
 * 
 * @post array is in uninitialized state and should not be used
 * @post All owned memory is freed
 * 
 * @note Safe to call multiple times or on uninitialized arrays
 */
MetaArrayResult meta_array_cleanup(MetaArray* array);

// ==============================================================================
// ELEMENT ACCESS
// ==============================================================================

/**
 * @brief Get pointer to element at specified index
 * 
 * Returns a pointer to the element at the given index, allowing both
 * read and write access (unless array is read-only).
 * 
 * @param array Pointer to MetaArray
 * @param index Index of element to access
 * @param element_ptr Output pointer to element (can be NULL for existence check)
 * @return META_ARRAY_SUCCESS on success, error code on failure
 * 
 * @note Returned pointer may become invalid after array modifications
 * @note Bounds checking is performed if enabled
 * 
 * @example
 * ```c
 * char** string_ptr;
 * if (meta_array_get(&array, 0, (void**)&string_ptr) == META_ARRAY_SUCCESS) {
 *     printf("First string: %s\n", *string_ptr);
 * }
 * ```
 */
MetaArrayResult meta_array_get(const MetaArray* array, size_t index, void** element_ptr);

/**
 * @brief Set element at specified index
 * 
 * @param array Pointer to MetaArray
 * @param index Index of element to set
 * @param element Pointer to new element data
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_set(MetaArray* array, size_t index, const void* element);

/**
 * @brief Get the first element in the array
 * 
 * @param array Pointer to MetaArray
 * @param element_ptr Output pointer to first element
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_front(const MetaArray* array, void** element_ptr);

/**
 * @brief Get the last element in the array
 * 
 * @param array Pointer to MetaArray
 * @param element_ptr Output pointer to last element
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_back(const MetaArray* array, void** element_ptr);

// ==============================================================================
// ARRAY MODIFICATION
// ==============================================================================

/**
 * @brief Append element to end of array
 * 
 * @param array Pointer to MetaArray
 * @param element Pointer to element data to append
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_append(MetaArray* array, const void* element);

/**
 * @brief Insert element at specified index
 * 
 * @param array Pointer to MetaArray
 * @param index Index where to insert element
 * @param element Pointer to element data to insert
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_insert(MetaArray* array, size_t index, const void* element);

/**
 * @brief Remove element at specified index
 * 
 * @param array Pointer to MetaArray
 * @param index Index of element to remove
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_remove(MetaArray* array, size_t index);

/**
 * @brief Remove the last element from array
 * 
 * @param array Pointer to MetaArray
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_pop(MetaArray* array);

/**
 * @brief Clear all elements from array
 * 
 * @param array Pointer to MetaArray
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_clear(MetaArray* array);

// ==============================================================================
// ARRAY PROPERTIES
// ==============================================================================

/**
 * @brief Check if array is empty
 * 
 * @param array Pointer to MetaArray
 * @return true if array is empty, false otherwise
 */
bool meta_array_is_empty(const MetaArray* array);

/**
 * @brief Get number of elements in array
 * 
 * @param array Pointer to MetaArray
 * @return Number of elements, or 0 if array is invalid
 */
size_t meta_array_size(const MetaArray* array);

/**
 * @brief Get current capacity of array
 * 
 * @param array Pointer to MetaArray
 * @return Current capacity, or 0 if array is invalid
 */
size_t meta_array_capacity(const MetaArray* array);

/**
 * @brief Check if index is valid for array
 * 
 * @param array Pointer to MetaArray
 * @param index Index to validate
 * @return true if index is valid, false otherwise
 */
bool meta_array_is_valid_index(const MetaArray* array, size_t index);

// ==============================================================================
// MEMORY MANAGEMENT
// ==============================================================================

/**
 * @brief Reserve capacity for specified number of elements
 * 
 * @param array Pointer to MetaArray
 * @param new_capacity Minimum capacity to reserve
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_reserve(MetaArray* array, size_t new_capacity);

/**
 * @brief Shrink array capacity to match current size
 * 
 * @param array Pointer to MetaArray
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_shrink_to_fit(MetaArray* array);

/**
 * @brief Resize array to specified number of elements
 * 
 * @param array Pointer to MetaArray
 * @param new_size New size for the array
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_resize(MetaArray* array, size_t new_size);

// ==============================================================================
// ITERATOR FUNCTIONS
// ==============================================================================

/**
 * @brief Create iterator for array traversal
 * 
 * @param array Pointer to MetaArray to iterate
 * @param iterator Pointer to iterator to initialize
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_iterator_init(const MetaArray* array, MetaArrayIterator* iterator);

/**
 * @brief Check if iterator has more elements
 * 
 * @param iterator Pointer to iterator
 * @return true if more elements available, false otherwise
 */
bool meta_array_iterator_has_next(const MetaArrayIterator* iterator);

/**
 * @brief Get next element from iterator
 * 
 * @param iterator Pointer to iterator
 * @param element_ptr Output pointer to current element
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_iterator_next(MetaArrayIterator* iterator, void** element_ptr);

/**
 * @brief Reset iterator to beginning
 * 
 * @param iterator Pointer to iterator to reset
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_iterator_reset(MetaArrayIterator* iterator);

// ==============================================================================
// UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Create shallow copy of array
 * 
 * @param source Source array to copy
 * @param destination Destination array to initialize
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_copy(const MetaArray* source, MetaArray* destination);

/**
 * @brief Compare two arrays for equality
 * 
 * @param array1 First array to compare
 * @param array2 Second array to compare
 * @param compare_func Function to compare individual elements (can be NULL for memcmp)
 * @return true if arrays are equal, false otherwise
 */
bool meta_array_equals(const MetaArray* array1, const MetaArray* array2,
                       int (*compare_func)(const void* a, const void* b));

/**
 * @brief Find element in array using comparison function
 * 
 * @param array Pointer to MetaArray to search
 * @param target Pointer to target element
 * @param compare_func Function to compare elements
 * @param index_ptr Output pointer for found index (can be NULL)
 * @return true if element found, false otherwise
 */
bool meta_array_find(const MetaArray* array, const void* target,
                     int (*compare_func)(const void* a, const void* b),
                     size_t* index_ptr);

/**
 * @brief Sort array using comparison function
 * 
 * @param array Pointer to MetaArray to sort
 * @param compare_func Function to compare elements
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_sort(MetaArray* array,
                                int (*compare_func)(const void* a, const void* b));

// ==============================================================================
// DEBUGGING AND VALIDATION
// ==============================================================================

/**
 * @brief Validate array structure and consistency
 * 
 * @param array Pointer to MetaArray to validate
 * @return true if array is valid, false otherwise
 */
bool meta_array_validate(const MetaArray* array);

/**
 * @brief Print array information for debugging
 * 
 * @param array Pointer to MetaArray to print
 * @param element_printer Function to print individual elements (can be NULL)
 */
void meta_array_debug_print(const MetaArray* array,
                            void (*element_printer)(const void* element));

/**
 * @brief Get memory usage statistics for array
 * 
 * @param array Pointer to MetaArray
 * @param bytes_used Output for bytes currently used
 * @param bytes_allocated Output for total bytes allocated
 * @return META_ARRAY_SUCCESS on success, error code on failure
 */
MetaArrayResult meta_array_memory_stats(const MetaArray* array,
                                        size_t* bytes_used, size_t* bytes_allocated);

// ==============================================================================
// CONVENIENCE MACROS
// ==============================================================================

/**
 * @brief Type-safe element access macro
 * 
 * @param array_ptr Pointer to MetaArray
 * @param index Element index
 * @param type Element type
 */
#define META_ARRAY_GET(array_ptr, index, type) \
    ((type*)((char*)(array_ptr)->arr + (index) * (array_ptr)->element_size))

/**
 * @brief Iterate through array elements
 * 
 * @param array_ptr Pointer to MetaArray
 * @param index_var Variable name for loop index
 * @param element_var Variable name for current element pointer
 * @param type Element type
 */
#define META_ARRAY_FOREACH(array_ptr, index_var, element_var, type) \
    for (size_t index_var = 0; \
         index_var < (array_ptr)->size && \
         ((element_var) = META_ARRAY_GET(array_ptr, index_var, type), true); \
         ++(index_var))

/**
 * @brief Initialize static MetaArray
 */
#define META_ARRAY_STATIC_INIT(element_type) \
    { .arr = NULL, .size = 0, .capacity = 0, .element_size = sizeof(element_type), .flags = 0 }

// ==============================================================================
// LEGACY COMPATIBILITY
// ==============================================================================

/**
 * @brief Legacy simple array structure for backward compatibility
 * 
 * @deprecated Use MetaArray instead for new code
 */
typedef struct {
  void* arr;      ///< Array data pointer
  size_t size;    ///< Number of elements
} meta_array;

/** @brief Legacy typedef for compatibility */
typedef meta_array MetaArrayLegacy;
