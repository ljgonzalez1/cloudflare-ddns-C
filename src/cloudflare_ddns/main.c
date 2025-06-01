#include "utils/get_env.h"
#include "utils/is_true.h"

#include "messages/messages.h"

int main(int argc, char *argv[]) {

  const char *is_proxied = get_env_var("PROXIED");
  const char *cloudflare_api_key = get_env_var("CLOUDFLARE_API_KEY");


  printf("%s\n", is_proxied);

  if (to_bool(is_proxied)) {
    printf("Is proxied\n");
  } else {
    printf("Not proxied\n");
  }

  return 0;
}
