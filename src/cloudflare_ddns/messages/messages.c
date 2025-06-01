#include "messages.h"

// TODO: complete locale implementation
// TODO: English only for now
// NOTE: As a buffer is used, message + emoji can't be larger than 254 characters.
static const Messages all_messages[] = {
  {
    .msg_key    = "MSG_PROGRAM_START",
    .msg_emoji  = "\uD83D\uDE80",
    .msg_en_US = "Starting program...",
  },
  {
    .msg_key    = "MSG_PROGRAM_END",
    .msg_emoji  = "\uD83C\uDFC1",
    .msg_en_US  = "Finished execution. Now closing.",
  },
  {
    .msg_key    = "ERR_VAR_NOT_FOUND",
    .msg_emoji  = "❌",
    .msg_en_US  = "Environment variable not found.",
  }
};

#define NUM_MESSAGES (sizeof(all_messages) / sizeof(all_messages[0]))

const char *message(const char *msg_key) {
  if (msg_key == NULL) {
    return NULL;
  }

  static char buf[256];

  for (size_t i = 0; i < NUM_MESSAGES; ++i) {
    if (strcmp(all_messages[i].msg_key, msg_key) == 0) {

      /* Chain together "<emoji> <text>" */
      snprintf(buf, sizeof buf, "%s %s",
               all_messages[i].msg_emoji,
               all_messages[i].msg_en_US);

      return buf;
    }
  }

  return NULL;
}