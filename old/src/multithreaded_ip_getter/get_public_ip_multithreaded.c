#include "get_public_ip_multithreaded.h"
#include "ip_getter.h"
#include "ip_utils.h"
#include "config.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

typedef struct {
  const char          *url;
  atomic_bool         *done;
  char               **winner;
  pthread_mutex_t     *mtx;
} WorkerArg;

/* ms → timespec helper */
static void sleep_ms(int ms)
{
  struct timespec ts = { .tv_sec = ms / 1000,
      .tv_nsec = (long)(ms % 1000) * 1000000L };
  nanosleep(&ts, NULL);
}

/* -------------------------------------------------------------------------- */
static void *worker(void *arg_)
{
  WorkerArg *arg = arg_;
  printf("[INFO] Starting worker for: %s\n", arg->url);

  for (int attempt = 1; attempt <= MAX_THREAD_GET_ATTEMPTS; ++attempt) {
    if (atomic_load(arg->done)) {
      printf("[INFO] %s: another thread won, stopping\n", arg->url);
      break;
    }

    printf("[INFO] %s: attempt %d/%d\n", arg->url, attempt, MAX_THREAD_GET_ATTEMPTS);

    char *raw = get_url_body(arg->url, HTTP_REQUEST_TIMEOUT_MS);
    if (!raw) {
      printf("[WARN] %s: no response received\n", arg->url);
      goto maybe_retry;
    }

    printf("[INFO] %s: got response, extracting IP\n", arg->url);

    // Usar extract_first_ipv4 para buscar una IP válida en la respuesta
    char *ip = extract_first_ipv4(raw);
    free(raw);

    if (ip) {
      printf("[SUCCESS] %s: found IP: %s\n", arg->url, ip);
      pthread_mutex_lock(arg->mtx);
      if (!*arg->winner) {
        printf("[WINNER] %s: setting as winner: %s\n", arg->url, ip);
        *arg->winner = ip;                       /* ownership transfer  */
        atomic_store(arg->done, true);
        pthread_mutex_unlock(arg->mtx);
        free(arg);
        return NULL;
      } else {
        printf("[INFO] %s: someone else already won\n", arg->url);
      }
      pthread_mutex_unlock(arg->mtx);
      free(ip);  // Ya hay un ganador
    } else {
      printf("[WARN] %s: no valid IP found in response\n", arg->url);
    }

    maybe_retry:
    if (attempt < MAX_THREAD_GET_ATTEMPTS && !atomic_load(arg->done)) {
      printf("[INFO] %s: retrying in %d ms\n", arg->url, THREAD_TASK_RETRY_TIME_MS);
      sleep_ms(THREAD_TASK_RETRY_TIME_MS);
    }
  }

  printf("[FAIL] %s: all attempts failed\n", arg->url);
  free(arg);
  return NULL;
}

/* -------------------------------------------------------------------------- */
char *get_public_ip_multithreaded(const char *const *urls, size_t n)
{
  if (!urls || n == 0) return NULL;

  printf("[INFO] Starting multithreaded IP getter with %zu URLs\n", n);

  pthread_t      *tid = calloc(n, sizeof *tid);
  if (!tid) return NULL;

  atomic_bool     done  = ATOMIC_VAR_INIT(false);
  pthread_mutex_t mtx;  pthread_mutex_init(&mtx, NULL);
  char           *winner = NULL;

  for (size_t i = 0; i < n; ++i) {
    WorkerArg *w = malloc(sizeof *w);
    if (!w) continue;
    *w = (WorkerArg){ .url = urls[i], .done = &done,
        .winner = &winner, .mtx = &mtx };
    printf("[INFO] Creating thread %zu for: %s\n", i, urls[i]);
    pthread_create(&tid[i], NULL, worker, w);
  }

  printf("[INFO] All threads created, waiting for completion\n");
  for (size_t i = 0; i < n; ++i) pthread_join(tid[i], NULL);

  printf("[RESULT] Winner: %s\n", winner ? winner : "NULL");

  pthread_mutex_destroy(&mtx);
  free(tid);
  return winner;
}