#pragma once

#include <stdlib.h>

struct meta_array {
  void *data;
  size_t length;
};

typedef struct meta_array MetaArray;

