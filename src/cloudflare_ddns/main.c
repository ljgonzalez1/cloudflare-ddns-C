/**
 * @file main.c
 * @brief Cloudflare DDNS Client - Main Entry Point
 * @date 2025-06-01
 *
 * Main application entry point for the Cloudflare Dynamic DNS client.
 * Demonstrates proper use of the environment variables system and
 * serves as the foundation for the full DDNS functionality.
 */


// ==============================================================================
// SYSTEM INCLUDES
// ==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// ==============================================================================
// LOCAL INCLUDES
// ==============================================================================

#include "environment/environment_variables.h"
#include "messages/messages.h"
#include "handlers/error_handler.h"
#include "handlers/help.h"
#include "validators/config_validator.h"

// ==============================================================================
// APPLICATION CONSTANTS
// ==============================================================================

/**
 * @brief Application metadata
 */
#define APP_NAME "Cloudflare DDNS C-lient"
#define APP_VERSION "1.0"
#define APP_AUTHOR "Luis González"

// ==============================================================================
// UTILITY FUNCTIONS
// ==============================================================================

/**
 * @brief Print application header with version information
 */
static void print_application_header(void) {
  printf("╔════════════════════════════════════════════════════════════════╗\n");
  printf("║  %s v%s%*s║\n", APP_NAME, APP_VERSION,
         (int)(58 - strlen(APP_NAME) - strlen(APP_VERSION) - 3), "");
  printf("║  By %s%*s║\n", APP_AUTHOR,
         (int)(60 - strlen(APP_AUTHOR) - 5), "");
  printf("╚════════════════════════════════════════════════════════════════╝\n");
  printf("\n");
}


int main(void) {
  ExitCode exit_code = EXIT_SUCCESS_CODE;

  // Display application information
  print_application_header();
  printf("%s\n", MSG_INFO_PROGRAM_START);
  printf("\n");

  // Initialize environment variables
  printf("🔧 Loading environment configuration...\n");
  init_env_variables();

  // Validate the loaded configuration
  if (!validate_configuration()) {
    printf("\n");
    handle_error(EXIT_CONFIG_ERROR);
    exit_code = EXIT_CONFIG_ERROR;
    goto cleanup;
  }

  // Demonstrate how the configuration would be used
  demonstrate_configuration_usage();

  printf("\n");
  printf("✨ Application completed successfully!\n");

cleanup:
  // Always clean up allocated resources
  printf("\n🧹 Cleaning up resources...\n");
  cleanup_env_variables();

  printf("%s\n", MSG_INFO_PROGRAM_END);

  return exit_code;
}

// ==============================================================================
// OPTIONAL: SIGNAL HANDLING FOR GRACEFUL SHUTDOWN
// ==============================================================================

#ifdef ENABLE_SIGNAL_HANDLING
#include <signal.h>

/**
 * @brief Signal handler for graceful shutdown
 *
 * Ensures proper cleanup when the application is interrupted.
 */
static void signal_handler(int signal_number) {
    printf("\n\n🛑 Received signal %d, shutting down gracefully...\n", signal_number);
    cleanup_env_variables();
    exit(EXIT_SUCCESS_CODE);
}

/**
 * @brief Setup signal handlers for graceful shutdown
 *
 * Call this from main() if you want to handle signals.
 */
static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // Termination request
}
#endif

// ==============================================================================
// OPTIONAL: ADVANCED FEATURES
// ==============================================================================

#ifdef ENABLE_DRY_RUN
/**
 * @brief Perform a dry run without making actual DNS changes
 *
 * Useful for testing configuration and seeing what would happen
 * without actually modifying DNS records.
 */
static void perform_dry_run(void) {
    printf("\n🏃‍♂️ Performing dry run (no actual DNS changes)...\n");

    // Simulate the operations that would be performed
    printf("   • Would fetch public IP address\n");

    for (size_t i = 0; i < Env.DOMAINS_COUNT; i++) {
        printf("   • Would update DNS record: %s → [current_ip]\n", Env.DOMAINS[i]);
    }

    printf("   • Would verify DNS propagation\n");
    printf("\n✅ Dry run completed successfully\n");
}
#endif