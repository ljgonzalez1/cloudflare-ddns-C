// main.c
#include <stdio.h>
#include "mbedtls/version.h"

int main(void) {
  printf("mbedTLS version: %s\n", MBEDTLS_VERSION_STRING);
  printf("Library imported successfully!\n");
  return 0;
}
