#pragma once

#include <stddef.h>

struct meta_array {
  void *arr;
  size_t size;
};

typedef struct meta_array MetaArray;

