#include "url_parser.h"

static size_t count_tokens(const char *str) {
  if (!str || *str == '\0' || *str == DOMAIN_DELIMITER) return 0;

  size_t count = 1;

  for (const char *delim = str; *delim; delim++) {
    if (*delim == DOMAIN_DELIMITER) count++;
  }

  return count;
}


static inline size_t compute_block_size(size_t tokens_count, size_t str_len) { return tokens_count * sizeof(char *) + (str_len + 1); }

static inline char *buffer_ptr(void *block, size_t tokens_count) { return (char *)block + tokens_count * sizeof(char *); }

static inline void init_buffer(char *buf, const char *src, size_t len) { memcpy(buf, src, len + 1); }


static void tokenize_buffer(char *buf, char **tokens) {
  size_t idx = 0;
  char *start = buf;

  for (char *delim = buf; ; ++delim) {
    if (*delim == DOMAIN_DELIMITER || *delim == '\0') {
      *delim = '\0';
      tokens[idx++] = start;
      start = delim + 1;
    }

    if (*delim == '\0') break;
  }
}


static void *split_url(const char *urls_str, size_t tokens_count) {
  if (!urls_str || tokens_count == 0) return NULL;

  size_t len = strlen(urls_str);
  size_t block_size = compute_block_size(tokens_count, len);

  void *block = mm_malloc(block_size);
  if (error_has(ERR_ALLOC_FAILURE)) return NULL;

  char **tokens = (char **) block;
  char *buf = buffer_ptr(block, tokens_count);

  init_buffer(buf, urls_str, len);
  tokenize_buffer(buf, tokens);

  return (void *) tokens;
}


MetaArray parse_urls(const char *urls_str) {
  MetaArray urls = {
      .data = NULL,
      .length = count_tokens(urls_str),
  };

  urls.data = split_url(urls_str, urls.length);

  return urls;
}
