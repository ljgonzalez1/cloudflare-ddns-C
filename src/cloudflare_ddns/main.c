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
         (int)(58 - strlen(APP_NAME) - strlen(APP_VERSION) + 2), "");
  printf("║  By %s%*s║\n", APP_AUTHOR,
         (int)(60 - strlen(APP_AUTHOR) + 0), "");
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

  printf("\n-------------==============DOES STUFF==============-------------\n");
  printf("\nROADMAP - CODE FLOW\n");

  printf("Primero asumimos que todo funciona y el código no explota...\n");

  printf("🔑 1. Verificando validez del token API de Cloudflare...\n");
  bool is_api_key_valid = check_cloudflare_api_key_validity(Env);

  for (size_t domain_index = 0; domain_index < Env.DOMAINS_COUNT; domain_index++) {
    char *domain = Env.DOMAINS[domain_index];

    printf("Procesando subdominio {DOMAIN}:\n");
    printf("🌐 2. Obteniendo ID de zona para {ZONE}...\n");
    char *zone_id = get_zone_id(domain, Env.CLOUDFLARE_API_KEY);

    printf("🔍 3. Comprobando si el subdominio existe en la zona...\n");
    bool domain_exists = check_domain_existence(domain, zone_id, Env.CLOUDFLARE_API_KEY);

    if (!domain_exists) {
      printf("  ➕ 3.A. Subdominio no existe. Creando subdominio {PROXIED?}proxiado {DOMAIN} en zona {ZONE} con registro A 1.1.1.1...\n");
      create_domain(domain, zone_id, Env.CLOUDFLARE_API_KEY, Env.PROXIED);
      printf("✅ 4. ¡Subdominio {DOMAIN} creado exitosamente en zona {ZONE}!\n");

    } else {
      printf("✅ 4. ¡Subdominio {DOMAIN} encontrado en zona {ZONE}!\n");
    }

    printf("🌍 5. Obteniendo dirección IP pública actual de {A}, {B} y {C} con solicitud GET...\n");
    char *ip = get_public_ip(Env.A, Env.B, Env.C);
  }



  printf("🧵 6. Lanzando hilos...\n");
  printf("⏳ 7. Esperando a que todos los hilos terminen...\n");
  printf("🏁 8. Primer hilo terminó con valor válido: Dirección IP pública: {IP}\n");
  printf("🛑 9. Terminando hilos restantes...\n");

  printf("📡 10. Obteniendo registros DNS de Cloudflare...\n");
  printf("🔄 11. Comparando IP actual con registros DNS...\n");

  printf("  ✅ 11.A. El registro DNS coincide con la IP actual, saltando pasos...\n");
  printf("  🔄 11.B.1. El registro DNS no coincide con la IP actual, actualizando registro DNS de W.X.Y.Z a A.B.C.D...\n");
  printf("  ⏳ 11.B.2. Esperando propagación DNS...\n");
  printf("  🔍 11.B.3. Verificando actualizaciones...\n");

  printf("  ✅ 11.B.4. ¡Registro DNS actualizado exitosamente!\n");
  printf("🧹 12. Limpiando recursos...\n");
  printf("🚪 13. Saliendo... || 💤 Durmiendo...\n");

  printf("\n-------------==============STUFF DONE==============-------------\n");

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