#include "messages.h"


static const char *get_local_lang_env(void) {
  const char *env = getenv("APP_LANG");
  const char *lang_code = NULL;

  if (env != NULL && strlen(env) == 2) {
    lang_code = env;
  }
}




const char *message(const char *msg_key) {

}



