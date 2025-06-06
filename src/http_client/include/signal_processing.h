/*
 * Signals Module - MIT-Compatible HTTP Client
 *
 * Provides graceful signal handling for clean program termination.
 * Ensures proper cleanup of resources when receiving termination signals.
 */

#pragma once

#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "settings.h"

/* Signal handling configuration */
#if SIGNAL_HANDLING_ENABLED

/* Callback function type for custom cleanup */
typedef void (*signal_cleanup_callback_t)(void);

/* Function declarations */
int signals_init(void);
void signals_cleanup(void);

/* Register custom cleanup callback */
int signals_register_cleanup(signal_cleanup_callback_t callback);

/* Manual signal handling */
void signals_handle_termination(int signum);

/* Check if cleanup is in progress */
int signals_cleanup_in_progress(void);

/* Block/unblock signals for critical sections */
int signals_block_all(void);
int signals_unblock_all(void);
int signals_block_termination(void);
int signals_unblock_termination(void);

#else

/* No-op versions when signal handling is disabled */
static inline int signals_init(void) { return 0; }
static inline void signals_cleanup(void) {}
static inline int signals_register_cleanup(signal_cleanup_callback_t callback) {
    (void)callback;
    return 0;
}
static inline void signals_handle_termination(int signum) { (void)signum; }
static inline int signals_cleanup_in_progress(void) { return 0; }
static inline int signals_block_all(void) { return 0; }
static inline int signals_unblock_all(void) { return 0; }
static inline int signals_block_termination(void) { return 0; }
static inline int signals_unblock_termination(void) { return 0; }

#endif /* SIGNAL_HANDLING_ENABLED */

#endif /* SIGNAL_PROCESSING_H */