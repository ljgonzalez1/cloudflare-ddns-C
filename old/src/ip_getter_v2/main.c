// get_requests/main.c
// Cliente HTTPS GET autocontenido usando mbedTLS 4.0.0+ para binarios estáticos
// Compatible con contenedores scratch - sin dependencias del sistema

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>

// mbedTLS includes en el orden correcto para compilación estática
#include "psa/crypto.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#define MAX_RESPONSE_SIZE 65536
#define MAX_HOST_SIZE 256
#define MAX_PATH_SIZE 512
#define RECV_TIMEOUT_MS 10000

// Estructura para componentes de URL
typedef struct {
  char host[MAX_HOST_SIZE];
  char path[MAX_PATH_SIZE];
  int port;
  int is_https;
} UrlComponents;

// Función estática para parsear URL
static int parse_url(const char* url, UrlComponents* comp) {
  if (!url || !comp) return -1;

  memset(comp, 0, sizeof(UrlComponents));
  strcpy(comp->path, "/");

  // Detectar protocolo
  if (strncmp(url, "https://", 8) == 0) {
    comp->is_https = 1;
    comp->port = 443;
    url += 8;
  } else if (strncmp(url, "http://", 7) == 0) {
    comp->is_https = 0;
    comp->port = 80;
    url += 7;
  } else {
    // Por defecto HTTPS
    comp->is_https = 1;
    comp->port = 443;
  }

  // Parsear host y path
  const char* slash = strchr(url, '/');
  const char* colon = strchr(url, ':');

  // Si hay puerto especificado
  if (colon && (!slash || colon < slash)) {
    size_t host_len = colon - url;
    if (host_len >= MAX_HOST_SIZE) return -1;

    strncpy(comp->host, url, host_len);
    comp->host[host_len] = '\0';

    comp->port = atoi(colon + 1);

    // Buscar path después del puerto
    const char* port_end = colon + 1;
    while (*port_end && isdigit(*port_end)) port_end++;
    if (*port_end == '/') {
      strncpy(comp->path, port_end, MAX_PATH_SIZE - 1);
    }
  } else {
    // Sin puerto especificado
    size_t host_len;
    if (slash) {
      host_len = slash - url;
      strncpy(comp->path, slash, MAX_PATH_SIZE - 1);
    } else {
      host_len = strlen(url);
    }

    if (host_len >= MAX_HOST_SIZE) return -1;
    strncpy(comp->host, url, host_len);
    comp->host[host_len] = '\0';
  }

  return 0;
}

// Función estática para extraer body de respuesta HTTP
static char* extract_http_body(const char* response) {
  if (!response) return NULL;

  const char* body_start = strstr(response, "\r\n\r\n");
  if (!body_start) return NULL;

  body_start += 4; // Saltar "\r\n\r\n"

  size_t body_len = strlen(body_start);
  char* body = malloc(body_len + 1);
  if (!body) return NULL;

  strcpy(body, body_start);
  return body;
}

// Función estática para HTTP simple
static char* http_get(const UrlComponents* comp) {
  struct addrinfo hints, *res, *rp;
  int sockfd = -1;
  char port_str[16];
  char request[1024];
  char* response = malloc(MAX_RESPONSE_SIZE);

  if (!response) return NULL;

  snprintf(port_str, sizeof(port_str), "%d", comp->port);

  // Construir request
  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: StaticClient/1.0\r\n"
           "Accept: */*\r\n"
           "Connection: close\r\n"
           "\r\n",
           comp->path, comp->host);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(comp->host, port_str, &hints, &res) != 0) {
    free(response);
    return NULL;
  }

  // Conectar
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1) continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
    close(sockfd);
    sockfd = -1;
  }

  freeaddrinfo(res);

  if (sockfd == -1) {
    free(response);
    return NULL;
  }

  // Enviar request
  if (send(sockfd, request, strlen(request), 0) < 0) {
    close(sockfd);
    free(response);
    return NULL;
  }

  // Recibir respuesta
  size_t total = 0;
  ssize_t received;

  while (total < MAX_RESPONSE_SIZE - 1 &&
         (received = recv(sockfd, response + total, MAX_RESPONSE_SIZE - total - 1, 0)) > 0) {
    total += received;
  }

  response[total] = '\0';
  close(sockfd);

  // Extraer body
  char* body = extract_http_body(response);
  free(response);
  return body;
}

// Función estática para HTTPS con mbedTLS
static char* https_get(const UrlComponents* comp) {
  mbedtls_net_context server_fd;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;

  char port_str[16];
  char request[1024];
  char* response = malloc(MAX_RESPONSE_SIZE);
  char* result = NULL;
  int ret = 0;

  if (!response) return NULL;

  // Inicializar PSA Crypto
  if (psa_crypto_init() != PSA_SUCCESS) {
    free(response);
    return NULL;
  }

  // Inicializar contextos
  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  // Seed RNG
  const char *pers = "static_https_client";
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)pers, strlen(pers));
  if (ret != 0) goto cleanup;

  // Conectar
  snprintf(port_str, sizeof(port_str), "%d", comp->port);
  ret = mbedtls_net_connect(&server_fd, comp->host, port_str, MBEDTLS_NET_PROTO_TCP);
  if (ret != 0) goto cleanup;

  // Configurar SSL
  ret = mbedtls_ssl_config_defaults(&conf,
                                    MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) goto cleanup;

  // Sin verificación de certificados para máxima compatibilidad
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
  mbedtls_ssl_conf_read_timeout(&conf, RECV_TIMEOUT_MS);

  ret = mbedtls_ssl_setup(&ssl, &conf);
  if (ret != 0) goto cleanup;

  ret = mbedtls_ssl_set_hostname(&ssl, comp->host);
  if (ret != 0) goto cleanup;

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  // Handshake SSL
  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      goto cleanup;
    }
  }

  // Construir y enviar request
  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: StaticClient/1.0\r\n"
           "Accept: */*\r\n"
           "Connection: close\r\n"
           "\r\n",
           comp->path, comp->host);

  size_t len = strlen(request);
  size_t written = 0;

  while (written < len) {
    ret = mbedtls_ssl_write(&ssl, (const unsigned char *)(request + written), len - written);
    if (ret < 0) {
      if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        continue;
      }
      goto cleanup;
    }
    written += ret;
  }

  // Leer respuesta
  size_t total = 0;
  int attempts = 0;
  const int max_attempts = 100;

  while (total < MAX_RESPONSE_SIZE - 1 && attempts < max_attempts) {
    ret = mbedtls_ssl_read(&ssl, (unsigned char *)(response + total),
                           MAX_RESPONSE_SIZE - total - 1);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      attempts++;
      usleep(50000); // 50ms
      continue;
    }

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == 0) {
      break;
    }

    if (ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
      continue;
    }

    if (ret < 0) {
      break;
    }

    total += ret;
    attempts = 0;

    // Si no hay más datos disponibles, intentar una vez más
    if (mbedtls_ssl_get_bytes_avail(&ssl) == 0) {
      usleep(100000); // 100ms
      ret = mbedtls_ssl_read(&ssl, (unsigned char *)(response + total),
                             MAX_RESPONSE_SIZE - total - 1);
      if (ret > 0) {
        total += ret;
      }
      break;
    }
  }

  response[total] = '\0';

  // Extraer body
  result = extract_http_body(response);

  mbedtls_ssl_close_notify(&ssl);

  cleanup:
  mbedtls_net_free(&server_fd);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  free(response);

  return result;
}

// Función principal de GET request
static char* get_url(const char* url) {
  UrlComponents comp;

  if (parse_url(url, &comp) != 0) {
    return NULL;
  }

  if (comp.is_https) {
    return https_get(&comp);
  } else {
    return http_get(&comp);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <url>\n", argv[0]);
    fprintf(stderr, "Ejemplo: %s https://ipinfo.io/\n", argv[0]);
    return 1;
  }

  char* response = get_url(argv[1]);

  if (response) {
    printf("%s", response);

    // Agregar newline si no termina en uno
    size_t len = strlen(response);
    if (len > 0 && response[len - 1] != '\n') {
      printf("\n");
    }

    free(response);
    return 0;
  } else {
    return 1;
  }
}