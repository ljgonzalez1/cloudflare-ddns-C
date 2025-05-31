/*
 * main.c
 * • Descarga dinámicamente el DER de ISRG Root X1 (HTTP:80 → letsencrypt.org)
 * • Usa ese DER como trust anchor para mbedTLS en el POST a Cloudflare.
 * • Asume que mbedTLS fue compilado con `-O3` y está instalado como librería estática.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>

#define BUFFER_SIZE 4096

// Variables de entorno
#define ENV_ZONE_ID   "ZONE_ID"
#define ENV_API_KEY   "API_KEY"
#define ENV_PROXIED   "PROXIED"     // "true" o "false"
#define ENV_IP_V4     "IP_V4"
#define ENV_SUBDOMAIN "SUBDOMAIN"

// ---------------------------------------------------------------------
// fetch_der: descarga el DER de un recurso HTTP/1.0 (salta cabeceras)
// ---------------------------------------------------------------------
static int fetch_der(const char *host, const char *port, const char *path,
                     unsigned char **out_buf, size_t *out_len)
{
  struct addrinfo hints, *res, *rp;
  int sockfd = -1;
  char request[512];
  unsigned char tmpbuf[BUFFER_SIZE];
  ssize_t n;
  int header_parsed = 0;
  unsigned char *buf = NULL;
  size_t buf_cap = 0;
  size_t buf_len = 0;

  int rq = snprintf(request, sizeof(request),
                    "GET %s HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    path, host);
  if (rq < 0 || rq >= (int)sizeof(request)) {
    fprintf(stderr, "fetch_der: request overflow\n");
    return -1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host, port, &hints, &res) != 0) {
    perror("fetch_der: getaddrinfo");
    return -1;
  }
  for (rp = res; rp; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd < 0) continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    close(sockfd);
    sockfd = -1;
  }
  freeaddrinfo(res);
  if (sockfd < 0) {
    fprintf(stderr, "fetch_der: no se pudo conectar a %s:%s\n", host, port);
    return -1;
  }

  ssize_t total_sent = 0;
  while (total_sent < rq) {
    ssize_t w = write(sockfd, request + total_sent, rq - total_sent);
    if (w <= 0) {
      perror("fetch_der: write");
      close(sockfd);
      return -1;
    }
    total_sent += w;
  }

  while (1) {
    n = read(sockfd, tmpbuf, sizeof(tmpbuf));
    if (n < 0) {
      perror("fetch_der: read");
      free(buf);
      close(sockfd);
      return -1;
    }
    if (n == 0) break;

    if (!header_parsed) {
      unsigned char *hdr_end = (unsigned char *)memmem(tmpbuf, (size_t)n, "\r\n\r\n", 4);
      if (hdr_end) {
        int offset = (int)((hdr_end + 4) - tmpbuf);
        int body_bytes = (int)n - offset;
        if (body_bytes > 0) {
          buf_cap = (size_t)body_bytes + 4096;
          buf = (unsigned char *)malloc(buf_cap);
          if (!buf) {
            fprintf(stderr, "fetch_der: malloc falló\n");
            close(sockfd);
            return -1;
          }
          memcpy(buf, tmpbuf + offset, (size_t)body_bytes);
          buf_len = (size_t)body_bytes;
        }
        header_parsed = 1;
      }
    } else {
      if (buf_len + (size_t)n > buf_cap) {
        size_t newcap = buf_cap * 2;
        if (newcap < buf_len + (size_t)n) newcap = buf_len + (size_t)n + 4096;
        unsigned char *p = (unsigned char *)realloc(buf, newcap);
        if (!p) {
          fprintf(stderr, "fetch_der: realloc falló\n");
          free(buf);
          close(sockfd);
          return -1;
        }
        buf = p;
        buf_cap = newcap;
      }
      memcpy(buf + buf_len, tmpbuf, (size_t)n);
      buf_len += (size_t)n;
    }
  }
  close(sockfd);
  if (!header_parsed) {
    fprintf(stderr, "fetch_der: no se halló cabecera HTTP\n");
    free(buf);
    return -1;
  }
  unsigned char *ret = (unsigned char *)realloc(buf, buf_len);
  if (ret == NULL && buf_len > 0) ret = buf;
  *out_buf = ret;
  *out_len = buf_len;
  return 0;
}

int main(void) {
  unsigned char *root_der = NULL;
  size_t root_len = 0;
  int ret;

  // 1) Descargar DER de ISRG Root X1
  if (fetch_der("letsencrypt.org", "80", "/certs/isrgrootx1.der", &root_der, &root_len) != 0) {
    fprintf(stderr, "Error: no se pudo obtener DER\n");
    return 1;
  }
  if (root_len == 0) {
    fprintf(stderr, "Error: DER vacío\n");
    free(root_der);
    return 1;
  }

  // 2) Leer variables de entorno
  const char *zone_id   = getenv(ENV_ZONE_ID);
  const char *api_key   = getenv(ENV_API_KEY);
  const char *proxied   = getenv(ENV_PROXIED);
  const char *ip_v4     = getenv(ENV_IP_V4);
  const char *subdomain = getenv(ENV_SUBDOMAIN);
  if (!zone_id || !api_key || !proxied || !ip_v4 || !subdomain) {
    fprintf(stderr, "Falta variable: %s, %s, %s, %s o %s\n", ENV_ZONE_ID, ENV_API_KEY, ENV_PROXIED, ENV_IP_V4, ENV_SUBDOMAIN);
    free(root_der);
    return 1;
  }

  // 3) Construir JSON
  char body[BUFFER_SIZE];
  int body_len = snprintf(body, sizeof(body),
                          "{"
                          "\"type\":\"A\","
                          "\"name\":\"%s\","
                          "\"content\":\"%s\","
                          "\"ttl\":1,"
                          "\"proxied\":%s"
                          "}", subdomain, ip_v4, proxied);
  if (body_len < 0 || body_len >= (int)sizeof(body)) {
    fprintf(stderr, "Error construyendo JSON\n");
    free(root_der);
    return 1;
  }

  // 4) Construir petición HTTP/1.1
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
    fprintf(stderr, "Error: request demasiado grande\n");
    free(root_der);
    return 1;
  }

  // 5) Inicializar estructuras mbedTLS
  mbedtls_net_context net_ctx;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt cacert;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  const char *pers = "cf_ddns";

  mbedtls_net_init(&net_ctx);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  // 6) Parsear certificado raíz en memoria
  ret = mbedtls_x509_crt_parse_der(&cacert, root_der, root_len);
  if (ret != 0) {
    char errbuf[200];
    mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error mbedtls_x509_crt_parse_der: %s\n", errbuf);
    goto cleanup_all;
  }

  // 7) Seed para DRBG
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *) pers, strlen(pers));
  if (ret != 0) {
    char errbuf[200];
    mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error mbedtls_ctr_drbg_seed: %s\n", errbuf);
    goto cleanup_all;
  }

  // 8) Configuración del SSL/TLS
  ret = mbedtls_ssl_config_defaults(&conf,
                                    MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error ssl_config_defaults: %s\n", errbuf);
    goto cleanup_all;
  }
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

  ret = mbedtls_ssl_setup(&ssl, &conf);
  if (ret != 0) {
    char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error ssl_setup: %s\n", errbuf);
    goto cleanup_all;
  }
  ret = mbedtls_ssl_set_hostname(&ssl, "api.cloudflare.com");
  if (ret != 0) {
    char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error set_hostname: %s\n", errbuf);
    goto cleanup_all;
  }

  // 9) Conectar TCP a api.cloudflare.com:443
  if ((ret = mbedtls_net_connect(&net_ctx, "api.cloudflare.com", "443", MBEDTLS_NET_PROTO_TCP)) != 0) {
    char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error net_connect: %s\n", errbuf);
    goto cleanup_all;
  }
  mbedtls_ssl_set_bio(&ssl, &net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

  // 10) Handshake TLS
  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
      fprintf(stderr, "Error ssl_handshake: %s\n", errbuf);
      goto cleanup_all;
    }
  }

  // 11) Enviar petición HTTP/1.1 sobre TLS
  int written = 0;
  while (written < req_len) {
    ret = mbedtls_ssl_write(&ssl, (const unsigned char *)request + written, req_len - written);
    if (ret <= 0) {
      char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
      fprintf(stderr, "Error ssl_write: %s\n", errbuf);
      goto cleanup_all;
    }
    written += ret;
  }

  // 12) Leer respuesta cifrada y mostrar salida
  unsigned char rdbuf[BUFFER_SIZE];
  do {
    ret = mbedtls_ssl_read(&ssl, rdbuf, sizeof(rdbuf));
    if (ret > 0) {
      fwrite(rdbuf, 1, ret, stdout);
    } else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      continue;
    } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == 0) {
      break;
    } else {
      char errbuf[200]; mbedtls_strerror(ret, errbuf, sizeof(errbuf));
      fprintf(stderr, "Error ssl_read: %s\n", errbuf);
      break;
    }
  } while (1);

  // 13) Cerrar conexión
  mbedtls_ssl_close_notify(&ssl);

  cleanup_all:
  mbedtls_net_free(&net_ctx);
  mbedtls_x509_crt_free(&cacert);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  free(root_der);
  return 0;
}
