#include "error_handler.h"

void handle_error(ExitCode error_code) {
  printf("\n");

  switch (error_code) {
    case EXIT_CONFIG_ERROR:
      printf("💥 Configuration Error\n");
      printf("   The application configuration is incomplete or invalid.\n");
      printf("   Please check the environment variables and try again.\n");
      printf("\n");
      print_environment_help();
      break;

    case EXIT_VALIDATION_ERROR:
      printf("💥 Validation Error\n");
      printf("   The provided configuration failed validation checks.\n");
      printf("   Please review the error messages above and correct the issues.\n");
      break;

    case EXIT_MEMORY_ERROR:
      printf("💥 Memory Error\n");
      printf("   The application failed to allocate required memory.\n");
      printf("   This could indicate system resource constraints.\n");
      break;

    default:
      printf("💥 Unknown Error\n");
      printf("   An unexpected error occurred.\n");
      break;
  }
}
