/*
 * Signals Module Implementation - MIT-Compatible HTTP Client
 *
 * Handles various termination signals for graceful cleanup.
 */

#include "../include/signal_processing.h"

#if SIGNAL_HANDLING_ENABLED

#include "../../include/debug_utils.h"
#include "../include/memory_management.h"
#include "../include/messages.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Maximum number of cleanup callbacks */
#define MAX_CLEANUP_CALLBACKS 8

/* Module state */
static struct {
  int initialized;
  volatile int cleanup_in_progress;
  signal_cleanup_callback_t callbacks[MAX_CLEANUP_CALLBACKS];
  int callback_count;
  struct sigaction old_handlers[32]; /* Store original handlers */
  sigset_t blocked_signals;
  int signals_blocked;
} signals_state = {
    .initialized = 0,
    .cleanup_in_progress = 0,
    .callback_count = 0,
    .signals_blocked = 0
};

/* Signals we handle */
static const int HANDLED_SIGNALS[] = {
    SIGINT,    /* Ctrl+C */
    SIGTERM,   /* Termination request */
    SIGHUP,    /* Hangup */
    SIGQUIT,   /* Quit */
    SIGPIPE,   /* Broken pipe (ignore) */
    -1         /* Sentinel */
};

/* Signal handler function */
static void signal_handler(int signum) {
  /* Avoid recursive signal handling */
  if (signals_state.cleanup_in_progress) {
    return;
  }

  signals_state.cleanup_in_progress = 1;

  DEBUG_SIGNAL(signum);

  /* Handle specific signals */
  switch (signum) {
    case SIGPIPE:
      /* Ignore broken pipe - we'll handle it in the code */
      DEBUG_LOG("Ignoring SIGPIPE");
      signals_state.cleanup_in_progress = 0;
      return;

    case SIGINT:
      DEBUG_LOG(MSG_WARN_SIGNAL_RECEIVED, signum);
      break;

    case SIGTERM:
      DEBUG_LOG(MSG_WARN_SIGNAL_RECEIVED, signum);
      break;

    case SIGHUP:
      DEBUG_LOG(MSG_WARN_SIGNAL_RECEIVED, signum);
      break;

    case SIGQUIT:
      DEBUG_LOG(MSG_WARN_SIGNAL_RECEIVED, signum);
      break;

    default:
      DEBUG_LOG(MSG_WARN_SIGNAL_RECEIVED, signum);
      break;
  }

  /* Call cleanup callbacks in reverse order */
  for (int i = signals_state.callback_count - 1; i >= 0; i--) {
    if (signals_state.callbacks[i]) {
      DEBUG_TRACE("Calling cleanup callback %d", i);
      signals_state.callbacks[i]();
    }
  }

  DEBUG_LOG(MSG_SIGNAL_CLEANUP_COMPLETE);

  /* Exit gracefully */
  exit(EXIT_SUCCESS);
}

/* Initialize signal handling */
int signals_init(void) {
  if (signals_state.initialized) {
    return 0; /* Already initialized */
  }

  DEBUG_LOG(MSG_SIGNAL_HANDLING_INIT);

  /* Initialize signal set */
  sigemptyset(&signals_state.blocked_signals);

  /* Install signal handlers */
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* Restart interrupted system calls */

  for (int i = 0; HANDLED_SIGNALS[i] != -1; i++) {
    int signum = HANDLED_SIGNALS[i];

    /* Save original handler */
    if (sigaction(signum, NULL, &signals_state.old_handlers[signum]) != 0) {
      DEBUG_ERROR("Failed to get original handler for signal %d", signum);
      continue;
    }

    /* Install our handler */
    if (sigaction(signum, &sa, NULL) != 0) {
      DEBUG_ERROR("Failed to install handler for signal %d", signum);
      continue;
    }

    DEBUG_TRACE("Installed handler for signal %d", signum);
  }

  signals_state.initialized = 1;
  DEBUG_LOG(MSG_SUCCESS_MODULE_INIT, "signal");

  return 0;
}

/* Cleanup signal handling */
void signals_cleanup(void) {
  if (!signals_state.initialized) {
    return;
  }

  DEBUG_LOG(MSG_DEBUG_MODULE_CLEANUP, "signal");

  /* Restore original signal handlers */
  for (int i = 0; HANDLED_SIGNALS[i] != -1; i++) {
    int signum = HANDLED_SIGNALS[i];

    if (sigaction(signum, &signals_state.old_handlers[signum], NULL) != 0) {
      DEBUG_ERROR("Failed to restore original handler for signal %d", signum);
    }
  }

  /* Clear state */
  signals_state.initialized = 0;
  signals_state.callback_count = 0;
  signals_state.cleanup_in_progress = 0;

  DEBUG_LOG(MSG_SUCCESS_CLEANUP_COMPLETED);
}

/* Register cleanup callback */
int signals_register_cleanup(signal_cleanup_callback_t callback) {
  if (!signals_state.initialized) {
    DEBUG_ERROR("Signal module not initialized");
    return -1;
  }

  if (!callback) {
    DEBUG_ERROR("Null callback provided");
    return -1;
  }

  if (signals_state.callback_count >= MAX_CLEANUP_CALLBACKS) {
    DEBUG_ERROR("Maximum number of cleanup callbacks exceeded");
    return -1;
  }

  signals_state.callbacks[signals_state.callback_count] = callback;
  signals_state.callback_count++;

  DEBUG_TRACE(MSG_SIGNAL_CALLBACK_REGISTERED);

  return 0;
}

/* Manual signal handling (for testing or special cases) */
void signals_handle_termination(int signum) {
  DEBUG_LOG("Manual termination signal %d", signum);
  signal_handler(signum);
}

/* Check if cleanup is in progress */
int signals_cleanup_in_progress(void) {
  return signals_state.cleanup_in_progress;
}

/* Block all signals */
int signals_block_all(void) {
  if (!signals_state.initialized) {
    return -1;
  }

  sigset_t all_signals;
  sigfillset(&all_signals);

  if (sigprocmask(SIG_BLOCK, &all_signals, &signals_state.blocked_signals) != 0) {
    DEBUG_ERROR("Failed to block all signals");
    return -1;
  }

  signals_state.signals_blocked = 1;
  DEBUG_TRACE("All signals blocked");

  return 0;
}

/* Unblock all signals */
int signals_unblock_all(void) {
  if (!signals_state.initialized) {
    return -1;
  }

  if (!signals_state.signals_blocked) {
    return 0; /* Nothing to unblock */
  }

  if (sigprocmask(SIG_SETMASK, &signals_state.blocked_signals, NULL) != 0) {
    DEBUG_ERROR("Failed to unblock signals");
    return -1;
  }

  signals_state.signals_blocked = 0;
  DEBUG_TRACE("All signals unblocked");

  return 0;
}

/* Block termination signals only */
int signals_block_termination(void) {
  if (!signals_state.initialized) {
    return -1;
  }

  sigset_t term_signals;
  sigemptyset(&term_signals);

  for (int i = 0; HANDLED_SIGNALS[i] != -1; i++) {
    if (HANDLED_SIGNALS[i] != SIGPIPE) { /* Don't block SIGPIPE */
      sigaddset(&term_signals, HANDLED_SIGNALS[i]);
    }
  }

  if (sigprocmask(SIG_BLOCK, &term_signals, NULL) != 0) {
    DEBUG_ERROR("Failed to block termination signals");
    return -1;
  }

  DEBUG_TRACE("Termination signals blocked");
  return 0;
}

/* Unblock termination signals */
int signals_unblock_termination(void) {
  if (!signals_state.initialized) {
    return -1;
  }

  sigset_t term_signals;
  sigemptyset(&term_signals);

  for (int i = 0; HANDLED_SIGNALS[i] != -1; i++) {
    if (HANDLED_SIGNALS[i] != SIGPIPE) { /* Don't unblock SIGPIPE */
      sigaddset(&term_signals, HANDLED_SIGNALS[i]);
    }
  }

  if (sigprocmask(SIG_UNBLOCK, &term_signals, NULL) != 0) {
    DEBUG_ERROR("Failed to unblock termination signals");
    return -1;
  }

  DEBUG_TRACE("Termination signals unblocked");
  return 0;
}

#endif /* SIGNAL_HANDLING_ENABLED */