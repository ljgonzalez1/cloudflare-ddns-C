// multithreaded_tasks_simulator/main.c
// Self-contained multithreaded task simulator using only mbedTLS
// Compatible with scratch containers - no system dependencies

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

// mbedTLS includes for random number generation
#include "psa/crypto.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

// Structure for data shared between threads
typedef struct {
  pthread_mutex_t result_mutex;     // Lock (3) - protects final result
  pthread_mutex_t init_mutex;       // Lock (1) - initialization synchronization
  volatile int result_written;      // Flag indicating if result was written
  volatile int winner_thread_id;    // ID of winning thread
  uint64_t final_result;           // Final result calculated by winner
  volatile int threads_started;     // Counter of started threads
  int total_threads;               // Total threads to create
  volatile int should_stop;         // Flag to stop threads
} shared_data_t;

// Structure for each thread's data
typedef struct {
  int thread_id;
  shared_data_t* shared;
  pthread_t handle;
  volatile int finished;
} thread_data_t;

// Global variables for random number generation
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static pthread_mutex_t rng_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to initialize random number generator
static int init_random_generator(void) {
  int ret;
  const char *pers = "multithreaded_task_simulator";

  // Initialize PSA Crypto
  if (psa_crypto_init() != PSA_SUCCESS) {
    fprintf(stderr, "Error: Failed to initialize PSA Crypto\n");
    return -1;
  }

  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)pers, strlen(pers));
  if (ret != 0) {
    char error_buf[200];
    mbedtls_strerror(ret, error_buf, sizeof(error_buf));
    fprintf(stderr, "Error: mbedtls_ctr_drbg_seed failed: %s\n", error_buf);
    return -1;
  }

  return 0;
}

// Function to cleanup random number generator
static void cleanup_random_generator(void) {
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
}

// Thread-safe function to generate random number
static uint32_t get_random_number(uint32_t min, uint32_t max) {
  uint32_t range = max - min + 1;
  uint32_t random_val;
  int ret;

  pthread_mutex_lock(&rng_mutex);
  ret = mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char*)&random_val, sizeof(random_val));
  pthread_mutex_unlock(&rng_mutex);

  if (ret != 0) {
    // Fallback to timestamp if generation fails
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    random_val = (uint32_t)(ts.tv_nsec);
  }

  return min + (random_val % range);
}

// Function to simulate intensive computational work
static uint64_t simulate_heavy_computation(int thread_id, int duration_seconds, shared_data_t* shared) {
  uint64_t result = 0;

  // Get start time
  struct timespec start_time, current_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  uint64_t iteration_count = 0;
  double elapsed_seconds = 0.0;
  // Keep working until we reach the target duration
  while (elapsed_seconds < duration_seconds) {
    // Do a batch of intensive calculations
    for (int batch = 0; batch < 10000; batch++) {
      uint64_t temp = (iteration_count * thread_id + 1);
      temp = (temp * temp) % 982451653; // Large prime number
      temp = (temp << 3) ^ (temp >> 5); // Bitwise operations
      temp = (temp * 7919) % 1000000007; // More prime operations
      temp ^= (temp << 13);
      temp ^= (temp >> 17);
      temp ^= (temp << 5);
      result += temp;
      iteration_count++;
    }

    // Check elapsed time every 10k iterations
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                      (current_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    // Check if we should stop (every 0.05 seconds approximately)
    if (iteration_count % 200000 == 0) {
      if (__atomic_load_n(&shared->should_stop, __ATOMIC_ACQUIRE)) {
        break;
      }
    }
  }

  return result;
}

// Function executed by each worker thread
static void* worker_thread(void* arg) {
  thread_data_t* data = (thread_data_t*)arg;
  shared_data_t* shared = data->shared;

  // Generate random duration between 1 and 20 seconds
  uint32_t duration = get_random_number(1, 20);

  printf("Thread %d: Starting work for %u seconds\n", data->thread_id, duration);

  // Perform computational work
  uint64_t my_result = simulate_heavy_computation(data->thread_id, duration, shared);

  // Check if someone already wrote the result
  if (!__atomic_load_n(&shared->should_stop, __ATOMIC_ACQUIRE)) {
    // Try to be the first to write the result
    pthread_mutex_lock(&shared->result_mutex);

    if (!shared->result_written && !shared->should_stop) {
      // We are the winner!
      shared->final_result = my_result;
      shared->winner_thread_id = data->thread_id;
      shared->result_written = 1;

      printf("Thread %d: Finished first! Writing result\n", data->thread_id);

      // Signal other threads to stop
      __atomic_store_n(&shared->should_stop, 1, __ATOMIC_RELEASE);
    } else {
      printf("Thread %d: Finished but result already written\n", data->thread_id);
    }

    pthread_mutex_unlock(&shared->result_mutex);
  } else {
    printf("Thread %d: Finished but was told to stop\n", data->thread_id);
  }

  // Mark that this thread finished
  __atomic_store_n(&data->finished, 1, __ATOMIC_RELEASE);

  return NULL;
}

// Function to terminate threads safely
static void terminate_remaining_threads(thread_data_t* threads, int num_threads) {
  printf("Terminating remaining threads...\n");

  for (int i = 0; i < num_threads; i++) {
    if (!threads[i].finished) {
      // Cancel thread if it hasn't finished
      int ret = pthread_cancel(threads[i].handle);
      if (ret != 0) {
        printf("Warning: Could not cancel thread %d (error: %d)\n", i + 1, ret);
      }
    }

    // Join to clean up resources
    void* retval;
    int ret = pthread_join(threads[i].handle, &retval);
    if (ret != 0) {
      printf("Warning: Error in pthread_join for thread %d (error: %d)\n", i + 1, ret);
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
    fprintf(stderr, "Example: %s 5\n", argv[0]);
    return 1;
  }

  int num_threads = atoi(argv[1]);
  if (num_threads <= 0 || num_threads > 100) {
    fprintf(stderr, "Error: Number of threads must be between 1 and 100\n");
    return 1;
  }

  printf("Starting Multithreaded Task Simulator\n");
  printf("Configuration: %d worker threads\n", num_threads);
  printf("Each thread will perform computational work for 1-20 seconds\n");
  printf("The first thread to finish will write the final result\n");
  printf("\n");

  // Initialize random number generator
  if (init_random_generator() != 0) {
    fprintf(stderr, "Error: Could not initialize random number generator\n");
    return 1;
  }

  // Initialize shared data
  shared_data_t shared = {0};
  shared.total_threads = num_threads;

  if (pthread_mutex_init(&shared.result_mutex, NULL) != 0) {
    fprintf(stderr, "Error: Could not initialize result_mutex\n");
    cleanup_random_generator();
    return 1;
  }

  if (pthread_mutex_init(&shared.init_mutex, NULL) != 0) {
    fprintf(stderr, "Error: Could not initialize init_mutex\n");
    pthread_mutex_destroy(&shared.result_mutex);
    cleanup_random_generator();
    return 1;
  }

  // Create structures for threads
  thread_data_t* threads = calloc(num_threads, sizeof(thread_data_t));
  if (!threads) {
    fprintf(stderr, "Error: Could not allocate memory for threads\n");
    pthread_mutex_destroy(&shared.result_mutex);
    pthread_mutex_destroy(&shared.init_mutex);
    cleanup_random_generator();
    return 1;
  }

  printf("Creating %d worker threads...\n", num_threads);

  // Create and launch threads
  for (int i = 0; i < num_threads; i++) {
    threads[i].thread_id = i + 1;
    threads[i].shared = &shared;
    threads[i].finished = 0;

    int ret = pthread_create(&threads[i].handle, NULL, worker_thread, &threads[i]);
    if (ret != 0) {
      fprintf(stderr, "Error: Could not create thread %d (error: %d)\n", i + 1, ret);

      // Terminate already created threads
      shared.should_stop = 1;
      for (int j = 0; j < i; j++) {
        pthread_cancel(threads[j].handle);
        pthread_join(threads[j].handle, NULL);
      }

      free(threads);
      pthread_mutex_destroy(&shared.result_mutex);
      pthread_mutex_destroy(&shared.init_mutex);
      cleanup_random_generator();
      return 1;
    }
  }

  printf("All threads have been started and are working\n");
  printf("\n");

  // MAIN THREAD DOES NOT WAIT - continues immediately
  // The first thread to finish will set the result_written flag
  // Main thread checks periodically but doesn't block

  struct timespec check_interval;
  check_interval.tv_sec = 0;
  check_interval.tv_nsec = 5000000; // 5ms

  // Poll for result without blocking indefinitely
  while (!shared.result_written) {
    nanosleep(&check_interval, NULL);
  }

  printf("Thread %d finished first!\n", shared.winner_thread_id);
  printf("Final result: %llu\n", (unsigned long long)shared.final_result);
  printf("Signaling other threads to stop...\n");
  printf("\n");

  // Terminate all remaining threads
  terminate_remaining_threads(threads, num_threads);

  printf("Cleaning up resources...\n");

  // Clean up resources
  free(threads);
  pthread_mutex_destroy(&shared.result_mutex);
  pthread_mutex_destroy(&shared.init_mutex);
  cleanup_random_generator();

  printf("Simulation completed successfully\n");
  printf("Program finished\n");

  return 0;
}