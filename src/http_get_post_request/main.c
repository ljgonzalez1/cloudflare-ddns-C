// src/http_get_post_request/main.c

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_HEADERS 64

/**
 * @brief Parsea una URL HTTPS en host y path.
 *
 * Asume que `url` comienza con "https://". Separa el host y el path.
 * Ejemplo:
 *   https://ejemplo.com/ruta -> host="ejemplo.com", path="/ruta"
 *   https://ejemplo.com       -> host="ejemplo.com", path="/"
 */
static int parse_url(const char *url, char **host, char **path) {
  if (strncmp(url, "https://", 8) != 0) {
    fprintf(stderr, "ERROR: La URL debe comenzar con \"https://\"\n");
    return 0;
  }
  const char *p = url + 8;
  const char *slash = strchr(p, '/');
  if (!slash) {
    *host = strdup(p);
    if (!*host) { perror("strdup"); return 0; }
    *path = strdup("/");
    if (!*path) { perror("strdup"); free(*host); return 0; }
  } else {
    size_t host_len = slash - p;
    *host = malloc(host_len + 1);
    if (!*host) { perror("malloc"); return 0; }
    strncpy(*host, p, host_len);
    (*host)[host_len] = '\0';
    *path = strdup(slash);
    if (!*path) { perror("strdup"); free(*host); return 0; }
  }
  return 1;
}

/**
 * @brief Crea un socket TCP y se conecta a host:port.
 *
 * @param host  Nombre de host o IP.
 * @param port  Puerto como cadena (por ejemplo, "443").
 * @return      Descriptor de socket conectado, o -1 en error.
 */
static int create_socket(const char *host, const char *port) {
  struct addrinfo hints, *res, *rp;
  int sock = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port, &hints, &res) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock < 0) continue;
    if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
    close(sock);
    sock = -1;
  }

  freeaddrinfo(res);
  return sock;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr,
            "Uso:\n"
            "  %s get  \"https://ejemplo.com/ruta\" [-H \"Header: valor\" ...]\n"
            "  %s post \"https://ejemplo.com/ruta\" [-H \"Header: valor\" ...] -d '{...json...}'\n",
            argv[0], argv[0]);
    return 1;
  }

  const char *method = argv[1];
  const char *url = argv[2];

  if (strcmp(method, "get") != 0 && strcmp(method, "post") != 0) {
    fprintf(stderr, "ERROR: Método no soportado. Solo 'get' o 'post'.\n");
    return 1;
  }

  char *host = NULL, *path = NULL;
  if (!parse_url(url, &host, &path)) {
    return 1;
  }

  // Parsear headers y data desde argv
  char *headers[MAX_HEADERS];
  int header_count = 0;
  char *data = NULL;
  size_t data_len = 0;

  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-H") == 0 && i + 1 < argc) {
      if (header_count < MAX_HEADERS) {
        headers[header_count++] = argv[++i];
      }
    } else if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
      data = argv[++i];
      data_len = strlen(data);
    }
  }

  int sock = create_socket(host, "443");
  if (sock < 0) {
    fprintf(stderr, "ERROR: No se pudo conectar a %s\n", host);
    free(host);
    free(path);
    return 1;
  }

  SSL_library_init();
  SSL_load_error_strings();
  const SSL_METHOD *method_ssl = TLS_client_method();
  if (!method_ssl) {
    fprintf(stderr, "ERROR: TLS_client_method falló\n");
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  SSL_CTX *ctx = SSL_CTX_new(method_ssl);
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    close(sock);
    free(host);
    free(path);
    return 1;
  }
  // Forzar mínimo TLS 1.2
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  // No verificamos certificados en este ejemplo
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  // SNI: enviar nombre del servidor
  if (!SSL_set_tlsext_host_name(ssl, host)) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  SSL_set_fd(ssl, sock);
  if (SSL_connect(ssl) != 1) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  // Construir la petición HTTP
  char request[4096];
  int offset = 0;

  if (strcmp(method, "get") == 0) {
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "GET %s HTTP/1.1\r\n"
                       "Host: %s\r\n", path, host);
  } else {
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "POST %s HTTP/1.1\r\n"
                       "Host: %s\r\n", path, host);
  }

  // Añadir headers provistos
  for (int i = 0; i < header_count; i++) {
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "%s\r\n", headers[i]);
  }

  if (strcmp(method, "post") == 0) {
    // Siempre añadir Content-Length (0 si no hay data)
    offset += snprintf(request + offset, sizeof(request) - offset,
                       "Content-Length: %zu\r\n", data ? data_len : 0);
  }

  // Añadir Connection: close y fin de headers
  offset += snprintf(request + offset, sizeof(request) - offset,
                     "Connection: close\r\n\r\n");

  // Enviar encabezados
  if (SSL_write(ssl, request, offset) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  // Enviar cuerpo si es POST y hay data
  if (strcmp(method, "post") == 0 && data_len > 0) {
    if (SSL_write(ssl, data, data_len) <= 0) {
      ERR_print_errors_fp(stderr);
      SSL_shutdown(ssl);
      SSL_free(ssl);
      SSL_CTX_free(ctx);
      close(sock);
      free(host);
      free(path);
      return 1;
    }
  }

  // Leer respuesta y volcar a stdout
  char buffer[4096];
  int bytes;
  while ((bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
    fwrite(buffer, 1, bytes, stdout);
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  close(sock);
  free(host);
  free(path);
  return 0;
}
