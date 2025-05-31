// main.c
// Cliente HTTPS para crear un registro A en Cloudflare usando mbedTLS 4.0.0 +
// PSA Crypto como fuente de entropía.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>            // PSA Crypto API (se incluye con mbedTLS 4.0)
#include "mbedtls/version.h"       // Para MBEDTLS_VERSION_STRING
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#define SERVER_NAME           "api.cloudflare.com"
#define SERVER_PORT           "443"
#define RESPONSE_BUFFER_SIZE  8192

// Helper: imprime errores de mbedTLS con descripción
static void print_mbedtls_error(const char *func, int error_code) {
  char buf[256];
  printf("ERROR en %s: -0x%04X\n", func, (unsigned int)(-error_code));
#if defined(MBEDTLS_ERROR_C)
  mbedtls_strerror(error_code, buf, sizeof(buf));
    printf("Detalles: %s\n", buf);
#endif
}

int main(void) {
  int ret = 0;
  psa_status_t psa_ret;
  mbedtls_net_context server_fd;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  char response_buffer[RESPONSE_BUFFER_SIZE];
  const char *pers = "cf_ddns_mbedtls_client";

  // 1) Inicializar PSA Crypto – obligatorio en mbedTLS 4.0.0+
  //    (“...debes llamar a psa_crypto_init() antes de cualquier operación de TLS”)
  //    :contentReference[oaicite:2]{index=2}
  psa_ret = psa_crypto_init();
  if (psa_ret != PSA_SUCCESS) {
    printf("ERROR: psa_crypto_init() devolvió %d. Asegúrate de que PSA esté bien configurado.\n", psa_ret);
    return 1;
  }

  // 2) Leer variables de entorno obligatorias
  const char *zone_id   = getenv("ZONE_ID");
  const char *api_key   = getenv("API_KEY");
  const char *subdomain = getenv("SUBDOMAIN");
  const char *ip_v4     = getenv("IP_V4");
  const char *proxied   = getenv("PROXIED"); // “true” o “false”
  if (!proxied) {
    proxied = "false";
  }

  printf("=== CLOUDFLARE DNS CLIENT (mbedTLS 4.0 + PSA Crypto) ===\n");
  printf("mbedTLS versión: %s\n\n", MBEDTLS_VERSION_STRING);

  if (!zone_id || !api_key || !subdomain || !ip_v4) {
    printf("ERROR: Faltan variables de entorno requeridas:\n");
    printf("  ZONE_ID, API_KEY, SUBDOMAIN, IP_V4\n");
    printf("Ejemplo:\n");
    printf("  export ZONE_ID=\"tu_zone_id\"\n");
    printf("  export API_KEY=\"tu_api_token\"\n");
    printf("  export SUBDOMAIN=\"nuevo.ejemplo.com\"\n");
    printf("  export IP_V4=\"1.2.3.4\"\n");
    printf("  export PROXIED=\"false\"  # opcional\n");
    return 1;
  }

  printf("Creando registro DNS:\n");
  printf("  Zone: %s\n", zone_id);
  printf("  Subdomain: %s\n", subdomain);
  printf("  IP: %s\n", ip_v4);
  printf("  Proxied: %s\n\n", proxied);

  // 3) Inicializar estructuras mbedTLS
  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  // 4) Semillar CTR-DRBG usando la fuente por defecto (PSA Crypto)
  //    (no se usa mbedtls_ssl_conf_rng(), ya que fue removida en v4.0.0) :contentReference[oaicite:3]{index=3}
  printf("Sembrando el CSPRNG con PSA Crypto como fuente de entropía...\n");
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
                              mbedtls_entropy_func,
                              &entropy,
                              (const unsigned char *) pers,
                              strlen(pers));
  if (ret != 0) {
    print_mbedtls_error("mbedtls_ctr_drbg_seed", ret);
    goto cleanup;
  }
  printf("CSPRNG inicializado exitosamente.\n");

  // 5) Conectar al servidor Cloudflare
  printf("\n=== Estableciendo conexión ===\n");
  printf("Conectando a %s:%s (HTTPS)...\n", SERVER_NAME, SERVER_PORT);
  ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP);
  if (ret != 0) {
    print_mbedtls_error("mbedtls_net_connect", ret);
    goto cleanup;
  }
  printf("Conexión TCP establecida correctamente.\n");

  // 6) Configurar SSL/TLS con valores por defecto
  printf("\n=== Configurando SSL/TLS ===\n");
  ret = mbedtls_ssl_config_defaults(&conf,
                                    MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    print_mbedtls_error("mbedtls_ssl_config_defaults", ret);
    goto cleanup;
  }

  // No verificaremos el certificado CA en este ejemplo (modo NO vértigo)
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
  mbedtls_ssl_conf_read_timeout(&conf, 10000); // 10 segundos de timeout lectura

  // En mbedTLS 4.0+, no existe mbedtls_ssl_conf_rng(). El DRBG ya usa PSA Crypto. :contentReference[oaicite:4]{index=4}

  ret = mbedtls_ssl_setup(&ssl, &conf);
  if (ret != 0) {
    print_mbedtls_error("mbedtls_ssl_setup", ret);
    goto cleanup;
  }

  ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME);
  if (ret != 0) {
    print_mbedtls_error("mbedtls_ssl_set_hostname", ret);
    goto cleanup;
  }

  mbedtls_ssl_set_bio(&ssl,
                      &server_fd,
                      mbedtls_net_send,
                      mbedtls_net_recv,
                      NULL);

  // 7) Realizar Handshake TLS
  printf("\n=== Iniciando handshake SSL/TLS ===\n");
  int handshake_intentos = 0;
  const int HANDSHAKE_MAX = 100;

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    handshake_intentos++;
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      if (handshake_intentos > HANDSHAKE_MAX) {
        printf("ERROR: Timeout en handshake tras %d intentos\n", handshake_intentos);
        ret = -1;
        goto cleanup;
      }
      continue;
    }
    print_mbedtls_error("mbedtls_ssl_handshake", ret);
    goto cleanup;
  }
  printf("Handshake completado con éxito en %d intentos.\n", handshake_intentos);
  printf("Versión TLS usada: %s\n", mbedtls_ssl_get_version(&ssl));
  printf("Cipher suite: %s\n", mbedtls_ssl_get_ciphersuite(&ssl));

  // 8) Preparar y enviar petición HTTPS (POST) a Cloudflare
  printf("\n=== Enviando petición HTTPS ===\n");
  char json_payload[512];
  snprintf(json_payload, sizeof(json_payload),
           "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\",\"ttl\":1,\"proxied\":%s}",
           subdomain, ip_v4, proxied);

  char request[1024];
  int content_length = (int)strlen(json_payload);
  int req_len = snprintf(request, sizeof(request),
                         "POST /client/v4/zones/%s/dns_records HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Authorization: Bearer %s\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         zone_id, SERVER_NAME, api_key, content_length, json_payload);

  if (req_len <= 0 || req_len >= (int)sizeof(request)) {
    fprintf(stderr, "ERROR: Buffer de request demasiado pequeño.\n");
    ret = 1;
    goto cleanup;
  }

  printf("JSON payload: %s\n", json_payload);

  int bytes_enviados = 0;
  while (bytes_enviados < req_len) {
    ret = mbedtls_ssl_write(&ssl,
                            (const unsigned char *)(request + bytes_enviados),
                            (size_t)(req_len - bytes_enviados));
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      continue;
    }
    if (ret < 0) {
      print_mbedtls_error("mbedtls_ssl_write", ret);
      goto cleanup;
    }
    bytes_enviados += ret;
  }

  printf("Petición enviada correctamente (%d bytes).\n", bytes_enviados);

  // 9) Leer respuesta HTTPS
  printf("\n=== Leyendo respuesta HTTPS ===\n");
  memset(response_buffer, 0, sizeof(response_buffer));

  int total_recibido = 0;
  int read_intentos = 0;
  const int READ_MAX = 200;

  do {
    ret = mbedtls_ssl_read(&ssl,
                           (unsigned char *)(response_buffer + total_recibido),
                           (size_t)(sizeof(response_buffer) - (size_t)total_recibido - 1));
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      read_intentos++;
      if (read_intentos > READ_MAX) {
        printf("WARNING: Timeout de lectura tras %d intentos\n", read_intentos);
        break;
      }
      continue;
    }
    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
      printf("Conexión cerrada limpiamente por el servidor.\n");
      break;
    }
    if (ret < 0) {
      print_mbedtls_error("mbedtls_ssl_read", ret);
      break;
    }
    if (ret == 0) {
      // EOF
      break;
    }
    total_recibido += ret;
    printf("Recibidos %d bytes (total: %d)\n", ret, total_recibido);
    read_intentos = 0;
  } while (ret > 0 && total_recibido < (int)(sizeof(response_buffer) - 1));

  response_buffer[total_recibido] = '\0';

  // 10) Mostrar y analizar resultado
  printf("\n========== RESPUESTA API CLOUDFLARE ==========\n");
  printf("%s\n", response_buffer);
  printf("========== FIN DE RESPUESTA ==========\n");

  if (strstr(response_buffer, "\"success\":true") != NULL) {
    printf("\n✓ SUCCESS: ¡Registro DNS creado con éxito!\n");
    ret = 0;
  } else if (strstr(response_buffer, "\"success\":false") != NULL) {
    printf("\n✗ FAILED: Falló la creación del registro. Revisa la respuesta anterior.\n");
    ret = 1;
  } else {
    printf("\n? UNKNOWN: No se pudo determinar correctamente el estado.\n");
    ret = 1;
  }

  // Cerrar conexión TLS
  printf("\nCerrando conexión SSL/TLS...\n");
  mbedtls_ssl_close_notify(&ssl);

  cleanup:
  // Liberar recursos mbedTLS
  mbedtls_net_free(&server_fd);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  printf("\n=== Programa finalizado ===\n");
  if (ret != 0) {
    printf("Código de salida: %d\n", ret);
  } else {
    printf("Código de salida: 0 (éxito)\n");
  }

  return ret;
}
