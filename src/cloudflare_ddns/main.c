#include <stdio.h>
#include <stdlib.h>

#include "utils/get_env.h"
#include "messages/messages.h"


int main(int argc, char *argv[]) {

  const char *valor = get_env_var("PROXIED");

  printf("%s\n", valor);

  /* Ahora “Messages” existe y trae los strings inicializados: */
  printf("%s\n", MSG_PROGRAM_START);
  printf("%s\n", MSG_PROGRAM_END);

  /* Por ejemplo, en algún error: */
  printf("%s\n", MSG_VAR_NOT_FOUND);

  return 0;
}
