#include <stdio.h>
#include <stdlib.h>

#include "utils/get_env.h"



int main(int argc, char *argv[]) {

  const char *valor = get_env_var("PROXIED");

  printf("%s\n", valor);


  return 0;
}
