#pragma once

#include <string.h>
#include <stdio.h>

#define DEFAULT_LOCALE "en_US"

struct messages {
  const char *lang_code;
  const char *msg_emoji;

  const char *msg_en_US;
  const char *msg_es_ES;
  // TODO: add more translations as needed
};

typedef struct messages Messages;

const char *message(const char *msg_key);
