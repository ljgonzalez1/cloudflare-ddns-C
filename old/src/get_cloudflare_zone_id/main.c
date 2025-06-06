// main.c
// Cliente HTTPS simple para obtener zone ID de Cloudflare usando mbedTLS 4.0.0+

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <psa/crypto.h>
#include "mbedtls/version.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

#define SERVER_NAME "api.cloudflare.com"
#define SERVER_PORT "443"
#define MAX_RESPONSE_SIZE 16384

static void print_error(const char *func, int error_code) {
  char error_buf[200];
  printf("ERROR en %s: -0x%04X\n", func, (unsigned int)(-error_code));
#if defined(MBEDTLS_ERROR_C)
  mbedtls_strerror(error_code, error_buf, sizeof(error_buf));
    printf("Detalles: %s\n", error_buf);
#endif
}

static void debug_print(void *ctx, int level, const char *file, int line, const char *str) {
  (void)ctx;
  (void)level;
  printf("DEBUG [%s:%d]: %s", file, line, str);
}

int main(void) {
  // Variables principales
  const char *api_key = getenv("API_KEY");
  const char *zone_name = getenv("ZONE_NAME");

  if (!api_key || !zone_name) {
    printf("ERROR: Variables requeridas:\n");
    printf("  export API_KEY=\"your_token\"\n");
    printf("  export ZONE_NAME=\"example.com\"\n");
    return 1;
  }

  printf("=== CLOUDFLARE ZONE ID CLIENT ===\n");
  printf("mbedTLS versión: %s\n", MBEDTLS_VERSION_STRING);
  printf("Consultando zona: %s\n\n", zone_name);

  // Inicializar PSA Crypto
  psa_status_t psa_status = psa_crypto_init();
  if (psa_status != PSA_SUCCESS) {
    printf("ERROR: psa_crypto_init() falló: %d\n", psa_status);
    return 1;
  }

  // Inicializar contextos mbedTLS
  mbedtls_net_context server_fd;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;

  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  int ret = 0;
  char *response_buffer = NULL;

  // Semillar el generador de números aleatorios
  const char *pers = "cloudflare_client";
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)pers, strlen(pers));
  if (ret != 0) {
    print_error("mbedtls_ctr_drbg_seed", ret);
    goto cleanup;
  }

  // Conectar al servidor
  printf("Conectando a %s:%s...\n", SERVER_NAME, SERVER_PORT);
  ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP);
  if (ret != 0) {
    print_error("mbedtls_net_connect", ret);
    goto cleanup;
  }
  printf("Conexión TCP establecida.\n");

  // Configurar SSL
  ret = mbedtls_ssl_config_defaults(&conf,
                                    MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    print_error("mbedtls_ssl_config_defaults", ret);
    goto cleanup;
  }

  // Configuración SSL/TLS
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
  mbedtls_ssl_conf_read_timeout(&conf, 5000); // 5 segundos

  // Habilitar debug para ver qué pasa
  // mbedtls_ssl_conf_dbg(&conf, debug_print, NULL);
  // mbedtls_debug_set_threshold(3);

  ret = mbedtls_ssl_setup(&ssl, &conf);
  if (ret != 0) {
    print_error("mbedtls_ssl_setup", ret);
    goto cleanup;
  }

  ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME);
  if (ret != 0) {
    print_error("mbedtls_ssl_set_hostname", ret);
    goto cleanup;
  }

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  // Realizar handshake SSL
  printf("Iniciando handshake SSL/TLS...\n");
  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      print_error("mbedtls_ssl_handshake", ret);
      goto cleanup;
    }
  }
  printf("Handshake completado. Protocolo: %s\n", mbedtls_ssl_get_version(&ssl));

  // Construir petición HTTP
  char request[1024];
  int req_len = snprintf(request, sizeof(request),
                         "GET /client/v4/zones?name=%s HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Authorization: Bearer %s\r\n"
                         "Accept: application/json\r\n"
                         "User-Agent: CloudflareClient/1.0\r\n"
                         "Connection: close\r\n"
                         "\r\n",
                         zone_name, SERVER_NAME, api_key);

  if (req_len >= sizeof(request)) {
    printf("ERROR: Request demasiado largo\n");
    ret = 1;
    goto cleanup;
  }

  printf("\nEnviando petición HTTP (%d bytes)...\n", req_len);

  // Enviar petición HTTP
  size_t bytes_sent = 0;
  while (bytes_sent < (size_t)req_len) {
    ret = mbedtls_ssl_write(&ssl,
                            (const unsigned char *)(request + bytes_sent),
                            (size_t)req_len - bytes_sent);
    if (ret < 0) {
      if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        continue;
      }
      print_error("mbedtls_ssl_write", ret);
      goto cleanup;
    }
    bytes_sent += (size_t)ret;
  }

  printf("Petición enviada exitosamente.\n");

  // Reservar buffer para respuesta
  response_buffer = malloc(MAX_RESPONSE_SIZE);
  if (!response_buffer) {
    printf("ERROR: No se pudo reservar memoria para respuesta\n");
    ret = 1;
    goto cleanup;
  }
  memset(response_buffer, 0, MAX_RESPONSE_SIZE);

  // Leer respuesta
  printf("\nLeyendo respuesta...\n");
  size_t total_received = 0;
  int read_attempts = 0;
  const int MAX_READ_ATTEMPTS = 50;

  while (total_received < MAX_RESPONSE_SIZE - 1 && read_attempts < MAX_READ_ATTEMPTS) {
    ret = mbedtls_ssl_read(&ssl,
                           (unsigned char *)(response_buffer + total_received),
                           MAX_RESPONSE_SIZE - total_received - 1);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      read_attempts++;
      usleep(50000); // 50ms
      continue;
    }

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
      printf("Servidor cerró la conexión.\n");
      break;
    }

    if (ret < 0) {
      // Ignorar errores de NewSessionTicket en TLS 1.3
      if (ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
        printf("Ignorando NewSessionTicket...\n");
        continue;
      }
      print_error("mbedtls_ssl_read", ret);
      break;
    }

    if (ret == 0) {
      printf("EOF recibido.\n");
      break;
    }

    total_received += (size_t)ret;
    printf("Recibidos %d bytes (total: %zu)\n", ret, total_received);
    read_attempts = 0;

    // Si no hay más datos disponibles, intentar una vez más y salir
    if (mbedtls_ssl_get_bytes_avail(&ssl) == 0) {
      usleep(100000); // 100ms
      ret = mbedtls_ssl_read(&ssl,
                             (unsigned char *)(response_buffer + total_received),
                             MAX_RESPONSE_SIZE - total_received - 1);
      if (ret > 0) {
        total_received += (size_t)ret;
        printf("Últimos %d bytes recibidos (total: %zu)\n", ret, total_received);
      }
      break;
    }
  }

  response_buffer[total_received] = '\0';

  // SIEMPRE mostrar la respuesta RAW
  printf("\n" "=============================================" "\n");
  printf("RESPUESTA RAW DE CLOUDFLARE (%zu bytes):\n", total_received);
  printf("=============================================" "\n");

  if (total_received > 0) {
    printf("%s\n", response_buffer);
  } else {
    printf("(Sin datos recibidos)\n");
  }

  printf("=============================================" "\n");

  // Status
  if (total_received > 0) {
    if (strstr(response_buffer, "\"success\":true")) {
      printf("\n✓ SUCCESS: Consulta exitosa\n");
      ret = 0;
    } else if (strstr(response_buffer, "\"success\":false")) {
      printf("\n✗ ERROR: Consulta falló\n");
      ret = 1;
    } else {
      printf("\n? Respuesta recibida pero estado incierto\n");
      ret = 0; // Consideramos éxito si recibimos datos
    }
  } else {
    printf("\n✗ ERROR: No se recibieron datos\n");
    ret = 1;
  }

  cleanup:
  if (response_buffer) {
    free(response_buffer);
  }

  mbedtls_ssl_close_notify(&ssl);
  mbedtls_net_free(&server_fd);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  printf("\nPrograma terminado con código: %d\n", ret);
  return ret;
}