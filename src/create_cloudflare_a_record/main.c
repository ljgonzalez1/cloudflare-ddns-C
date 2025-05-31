#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 8192

int create_ssl_connection(const char *host, const char *port, SSL_CTX **out_ctx, SSL **out_ssl) {
  struct addrinfo hints, *res, *rp;
  int sockfd = -1;
  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;

  // Initialize OpenSSL
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    fprintf(stderr, "Failed to create SSL_CTX\n");
    return -1;
  }

  // Set minimum TLS version (optional)
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port, &hints, &res) != 0) {
    perror("getaddrinfo");
    SSL_CTX_free(ctx);
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(sockfd);
    sockfd = -1;
  }

  freeaddrinfo(res);

  if (sockfd < 0) {
    fprintf(stderr, "Could not connect to %s:%s\n", host, port);
    SSL_CTX_free(ctx);
    return -1;
  }

  ssl = SSL_new(ctx);
  if (!ssl) {
    fprintf(stderr, "SSL_new failed\n");
    close(sockfd);
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL_set_fd(ssl, sockfd);
  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    return -1;
  }

  *out_ctx = ctx;
  *out_ssl = ssl;
  return sockfd;
}

int main(void) {
  const char *zone_id = getenv("ZONE_ID");
  const char *api_key = getenv("API_KEY");
  const char *proxied = getenv("PROXIED");
  const char *ip_v4 = getenv("IP_V4");
  const char *subdomain = getenv("SUBDOMAIN");

  if (!zone_id || !api_key || !proxied || !ip_v4 || !subdomain) {
    fprintf(stderr, "One or more required environment variables not set\n");
    return 1;
  }

  // Construct the JSON body
  char body[BUFFER_SIZE];
  int body_len = snprintf(body, sizeof(body),
                          "{\"type\":\"A\","
                          "\"name\":\"%s\","
                          "\"content\":\"%s\","
                          "\"ttl\":1,"
                          "\"proxied\":%s}",
                          subdomain, ip_v4, proxied);
  if (body_len < 0 || body_len >= (int)sizeof(body)) {
    fprintf(stderr, "Failed to construct JSON body\n");
    return 1;
  }

  // Construct the HTTP request
  char request[BUFFER_SIZE];
  char path[512];
  snprintf(path, sizeof(path), "/client/v4/zones/%s/dns_records", zone_id);

  int req_len = snprintf(request, sizeof(request),
                         "POST %s HTTP/1.1\r\n"
                         "Host: api.cloudflare.com\r\n"
                         "Authorization: Bearer %s\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         path, api_key, body_len, body);
  if (req_len < 0 || req_len >= (int)sizeof(request)) {
    fprintf(stderr, "Request too large\n");
    return 1;
  }

  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;
  int sockfd = create_ssl_connection("api.cloudflare.com", "443", &ctx, &ssl);
  if (sockfd < 0) {
    // Error already printed
    return 1;
  }

  // Send the request over TLS
  int total_sent = 0;
  while (total_sent < req_len) {
    int bytes_sent = SSL_write(ssl, request + total_sent, req_len - total_sent);
    if (bytes_sent <= 0) {
      ERR_print_errors_fp(stderr);
      SSL_shutdown(ssl);
      SSL_free(ssl);
      SSL_CTX_free(ctx);
      close(sockfd);
      return 1;
    }
    total_sent += bytes_sent;
  }

  // Read and print the response
  char buffer[BUFFER_SIZE];
  int bytes_read;
  while ((bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';
    fputs(buffer, stdout);
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  close(sockfd);
  return 0;
}
