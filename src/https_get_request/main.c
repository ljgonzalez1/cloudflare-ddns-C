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

/**
 * @brief Parsea una URL HTTPS en host y path.
 *
 * Asume que `url` comienza con "https://". Separa el host y el path.
 * Ejemplo:
 *   https://www.ejemplo.com/ruta -> host="www.ejemplo.com", path="/ruta"
 *   https://www.ejemplo.com       -> host="www.ejemplo.com", path="/"
 *
 * @param url  Cadena con la URL completa.
 * @param host Puntero donde se almacenará el host (alocado con strdup/malloc).
 * @param path Puntero donde se almacenará el path (alocado con strdup).
 * @return     1 si todo OK, 0 en caso de error.
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
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <https_url>\n", argv[0]);
    return 1;
  }

  char *host = NULL, *path = NULL;
  if (!parse_url(argv[1], &host, &path)) {
    return 1;
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
  const SSL_METHOD *method = TLS_client_method();
  if (!method) {
    fprintf(stderr, "ERROR: TLS_client_method falló\n");
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    close(sock);
    free(host);
    free(path);
    return 1;
  }
  /* Forzamos mínimo TLS 1.2 */
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  /* Deshabilitamos verificación de certificado para este ejemplo */
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

  /* ---- Aquí agregamos SNI ---- */
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

  char request[1024];
  int n = snprintf(request, sizeof(request),
                   "GET %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Connection: close\r\n"
                   "\r\n",
                   path, host);
  if (n < 0 || n >= (int)sizeof(request)) {
    fprintf(stderr, "ERROR: Request demasiado largo\n");
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  if (SSL_write(ssl, request, strlen(request)) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    free(host);
    free(path);
    return 1;
  }

  /* Leer la respuesta y volcar a stdout */
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
