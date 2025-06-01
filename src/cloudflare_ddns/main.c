#include <stdio.h>
#include <stdlib.h>

#include "utils/get_env.h"
#include "utils/is_true.h"

#include "messages/messages.h"

int main(int argc, char *argv[]) {

  const char *is_proxied = get_env_var("PROXIED");

  printf("%s\n", is_proxied);

  if (to_bool(is_proxied)) {
    printf("Is proxied\n");
  } else {
    printf("Not proxied\n");
  }

  return 0;
}
