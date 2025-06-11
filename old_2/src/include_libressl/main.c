#include <stdio.h>
#include <openssl/opensslv.h>

int main(void) {
  printf("Versión de LibreSSL enlazada: %s\n", OPENSSL_VERSION_TEXT);
  return 0;
}
