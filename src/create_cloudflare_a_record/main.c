/*
 * main.c
 * • Descarga dinámicamente el DER de ISRG Root X1 (HTTP:80 → letsencrypt.org)
 * • Usa ese DER como trust anchor para BearSSL en el POST a Cloudflare.
 * • Asume que se compila con:
 *     gcc -std=gnu11 main.c -o create_cloudflare_a_record \
 *         -static -static-libgcc -lbearssl
 * • No dependemos de OpenSSL; solo BearSSL.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <bearssl.h>

#define BUFFER_SIZE 4096

// Variables de entorno
#define ENV_ZONE_ID   "ZONE_ID"
#define ENV_API_KEY   "API_KEY"
#define ENV_PROXIED   "PROXIED"     // "true" o "false"
#define ENV_IP_V4     "IP_V4"
#define ENV_SUBDOMAIN "SUBDOMAIN"

// ---------------------------------------------------------------------
// 1) fetch_der: descarga un binario (DER) saltándose cabeceras HTTP/1.0
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

  // Preparar GET <path> HTTP/1.0
  int rq = snprintf(request, sizeof(request),
                    "GET %s HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    path, host);
  if (rq < 0 || rq >= (int)sizeof(request)) {
    fprintf(stderr, "fetch_der: overflow en request\n");
    return -1;
  }

  // Resolver host:port
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host, port, &hints, &res) != 0) {
    perror("fetch_der: getaddrinfo");
    return -1;
  }

  // Conectar al primer addr válido
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

  // Enviar petición
  ssize_t total_sent = 0;
  while (total_sent < rq) {
    ssize_t w = write(sockfd, request + total_sent, (size_t)(rq - total_sent));
    if (w <= 0) {
      perror("fetch_der: write");
      close(sockfd);
      return -1;
    }
    total_sent += w;
  }

  // Leer respuesta: primera parte busca cabeceras, luego acumula cuerpo
  while (1) {
    n = read(sockfd, tmpbuf, sizeof(tmpbuf));
    if (n < 0) {
      perror("fetch_der: read");
      free(buf);
      close(sockfd);
      return -1;
    }
    if (n == 0) {
      // EOF
      break;
    }
    if (!header_parsed) {
      // Buscar fin de cabeceras \r\n\r\n
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
      // Si no se encontró el fin de cabeceras, descartamos tmpbuf y seguimos
    } else {
      // Append directo a buf
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
    fprintf(stderr, "fetch_der: no se halló cabecera HTTP completa\n");
    free(buf);
    return -1;
  }
  // Ajustar tamaño final
  unsigned char *ret = (unsigned char *)realloc(buf, buf_len);
  if (ret == NULL && buf_len > 0) ret = buf;
  *out_buf = ret;
  *out_len = buf_len;
  return 0;
}

// ---------------------------------------------------------------------
// 2) SSL Helper: manejo del motor SSL con la API correcta
// ---------------------------------------------------------------------
static int ssl_write_all(br_ssl_engine_context *engine, int sockfd,
                         const unsigned char *data, size_t len)
{
  size_t total_sent = 0;

  while (total_sent < len) {
    unsigned state = br_ssl_engine_current_state(engine);

    // Enviar registros SSL si hay pendientes
    if (state & BR_SSL_SENDREC) {
      unsigned char *sendrec_buf;
      size_t sendrec_len;

      sendrec_buf = br_ssl_engine_sendrec_buf(engine, &sendrec_len);
      if (sendrec_buf && sendrec_len > 0) {
        ssize_t written = write(sockfd, sendrec_buf, sendrec_len);
        if (written <= 0) {
          fprintf(stderr, "Error escribiendo datos SSL al socket\n");
          return -1;
        }
        br_ssl_engine_sendrec_ack(engine, (size_t)written);
      }
      continue;
    }

    // Escribir datos de aplicación
    if (state & BR_SSL_SENDAPP) {
      unsigned char *sendapp_buf;
      size_t sendapp_len;

      sendapp_buf = br_ssl_engine_sendapp_buf(engine, &sendapp_len);
      if (sendapp_buf && sendapp_len > 0) {
        size_t to_write = len - total_sent;
        if (to_write > sendapp_len) to_write = sendapp_len;

        memcpy(sendapp_buf, data + total_sent, to_write);
        br_ssl_engine_sendapp_ack(engine, to_write);
        total_sent += to_write;
      }
      continue;
    }

    if (state & BR_SSL_CLOSED) {
      fprintf(stderr, "Conexión SSL cerrada durante escritura\n");
      return -1;
    }

    // Estado inesperado
    fprintf(stderr, "Estado SSL inesperado durante escritura: %u\n", state);
    return -1;
  }

  // Asegurar que todos los datos se envíen
  br_ssl_engine_flush(engine, 0);

  return 0;
}

static int ssl_read_response(br_ssl_engine_context *engine, int sockfd)
{
  unsigned char sockbuf[BUFFER_SIZE];

  for (;;) {
    unsigned state = br_ssl_engine_current_state(engine);

    // Leer datos del socket si el motor los necesita
    if (state & BR_SSL_RECVREC) {
      unsigned char *recvrec_buf;
      size_t recvrec_len;

      recvrec_buf = br_ssl_engine_recvrec_buf(engine, &recvrec_len);
      if (recvrec_buf && recvrec_len > 0) {
        ssize_t nread = read(sockfd, sockbuf, sizeof(sockbuf));
        if (nread < 0) {
          perror("Error leyendo del socket");
          return -1;
        }
        if (nread == 0) {
          // EOF
          return 0;
        }

        size_t to_copy = (size_t)nread;
        if (to_copy > recvrec_len) to_copy = recvrec_len;

        memcpy(recvrec_buf, sockbuf, to_copy);
        br_ssl_engine_recvrec_ack(engine, to_copy);
      }
      continue;
    }

    // Enviar registros SSL si hay pendientes
    if (state & BR_SSL_SENDREC) {
      unsigned char *sendrec_buf;
      size_t sendrec_len;

      sendrec_buf = br_ssl_engine_sendrec_buf(engine, &sendrec_len);
      if (sendrec_buf && sendrec_len > 0) {
        ssize_t written = write(sockfd, sendrec_buf, sendrec_len);
        if (written <= 0) {
          fprintf(stderr, "Error escribiendo datos SSL al socket\n");
          return -1;
        }
        br_ssl_engine_sendrec_ack(engine, (size_t)written);
      }
      continue;
    }

    // Leer datos de aplicación si están disponibles
    if (state & BR_SSL_RECVAPP) {
      unsigned char *recvapp_buf;
      size_t recvapp_len;

      recvapp_buf = br_ssl_engine_recvapp_buf(engine, &recvapp_len);
      if (recvapp_buf && recvapp_len > 0) {
        // Escribir datos al stdout
        fwrite(recvapp_buf, 1, recvapp_len, stdout);
        br_ssl_engine_recvapp_ack(engine, recvapp_len);
      }
      continue;
    }

    if (state & BR_SSL_CLOSED) {
      return 0;
    }

    // Estado inesperado
    fprintf(stderr, "Estado SSL inesperado durante lectura: %u\n", state);
    return -1;
  }
}

// ---------------------------------------------------------------------
// 3) main: descarga certificado raíz + TLS + POST a Cloudflare
// ---------------------------------------------------------------------
int main(void) {
  unsigned char *root_der = NULL;
  size_t root_len = 0;

  // 3.1) Descargar DER de ISRG Root X1
  if (fetch_der("letsencrypt.org", "80", "/certs/isrgrootx1.der",
                &root_der, &root_len) != 0)
  {
    fprintf(stderr, "Error: no se pudo obtener el DER de ISRG Root X1\n");
    return 1;
  }
  if (root_len == 0) {
    fprintf(stderr, "Error: longitud de DER es cero\n");
    free(root_der);
    return 1;
  }

  // 3.2) Leer variables de entorno
  const char *zone_id   = getenv(ENV_ZONE_ID);
  const char *api_key   = getenv(ENV_API_KEY);
  const char *proxied   = getenv(ENV_PROXIED);
  const char *ip_v4     = getenv(ENV_IP_V4);
  const char *subdomain = getenv(ENV_SUBDOMAIN);
  if (!zone_id || !api_key || !proxied || !ip_v4 || !subdomain) {
    fprintf(stderr,
            "Falta variable: %s, %s, %s, %s o %s\n",
            ENV_ZONE_ID, ENV_API_KEY, ENV_PROXIED, ENV_IP_V4, ENV_SUBDOMAIN);
    free(root_der);
    return 1;
  }

  // 3.3) Construir JSON body
  char body[BUFFER_SIZE];
  int body_len = snprintf(body, sizeof(body),
                          "{"
                          "\"type\":\"A\","
                          "\"name\":\"%s\","
                          "\"content\":\"%s\","
                          "\"ttl\":1,"
                          "\"proxied\":%s"
                          "}",
                          subdomain, ip_v4, proxied
  );
  if (body_len < 0 || body_len >= (int)sizeof(body)) {
    fprintf(stderr, "Error construyendo JSON\n");
    free(root_der);
    return 1;
  }

  // 3.4) Construir petición HTTP/1.1 a Cloudflare
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
                         path,
                         api_key,
                         body_len,
                         body
  );
  if (req_len < 0 || req_len >= (int)sizeof(request)) {
    fprintf(stderr, "Request HTTP demasiado grande\n");
    free(root_der);
    return 1;
  }

  // 3.5) Conectar TCP a api.cloudflare.com:443
  struct addrinfo hints, *res, *rp;
  int sockfd = -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo("api.cloudflare.com", "443", &hints, &res) != 0) {
    perror("getaddrinfo(api.cloudflare.com)");
    free(root_der);
    return 1;
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
    fprintf(stderr, "No se pudo conectar a api.cloudflare.com:443\n");
    free(root_der);
    return 1;
  }

  // 3.6) Inicializar BearSSL TLS
  br_ssl_client_context   sc;
  br_x509_minimal_context xc;
  unsigned char           iobuf[BR_SSL_BUFSIZE_BIDI];

  // Crear trust anchor desde el DER descargado
  br_x509_trust_anchor anchor;
  anchor.dn.data = root_der;
  anchor.dn.len = root_len;
  anchor.flags = BR_X509_TA_CA;

  // Decodificar la clave pública del certificado DER
  br_x509_decoder_context dc;
  br_x509_decoder_init(&dc, 0, 0);
  br_x509_decoder_push(&dc, root_der, root_len);

  // Obtener la clave pública
  br_x509_pkey *pkey = br_x509_decoder_get_pkey(&dc);
  if (!pkey) {
    fprintf(stderr, "Error: no se pudo extraer clave pública del certificado\n");
    close(sockfd);
    free(root_der);
    return 1;
  }
  anchor.pkey = *pkey;

  // Inicializar contexto X.509 mínimo
  br_x509_minimal_init(&xc, &br_sha256_vtable, &anchor, 1);

  // Inicializar contexto SSL cliente
  br_ssl_client_init_full(&sc, &xc, &anchor, 1);

  // Configurar buffers
  br_ssl_engine_set_buffer(&sc.eng, iobuf, sizeof(iobuf), 1);

  // 3.7) Handshake TLS
  if (br_ssl_client_reset(&sc, "api.cloudflare.com", 0) != 0) {
    fprintf(stderr, "Falló handshake TLS con BearSSL\n");
    close(sockfd);
    free(root_der);
    return 1;
  }

  // 3.8) Realizar handshake inicial leyendo/escribiendo según el estado
  for (;;) {
    unsigned state = br_ssl_engine_current_state(&sc.eng);

    if (state & BR_SSL_CLOSED) {
      fprintf(stderr, "Conexión SSL cerrada durante handshake\n");
      close(sockfd);
      free(root_der);
      return 1;
    }

    if (state & BR_SSL_SENDREC) {
      unsigned char *sendrec_buf;
      size_t sendrec_len;

      sendrec_buf = br_ssl_engine_sendrec_buf(&sc.eng, &sendrec_len);
      if (sendrec_buf && sendrec_len > 0) {
        ssize_t written = write(sockfd, sendrec_buf, sendrec_len);
        if (written <= 0) {
          fprintf(stderr, "Error escribiendo durante handshake\n");
          close(sockfd);
          free(root_der);
          return 1;
        }
        br_ssl_engine_sendrec_ack(&sc.eng, (size_t)written);
      }
      continue;
    }

    if (state & BR_SSL_RECVREC) {
      unsigned char *recvrec_buf;
      size_t recvrec_len;
      unsigned char tmpbuf[1024];

      recvrec_buf = br_ssl_engine_recvrec_buf(&sc.eng, &recvrec_len);
      if (recvrec_buf && recvrec_len > 0) {
        ssize_t nread = read(sockfd, tmpbuf, sizeof(tmpbuf));
        if (nread <= 0) {
          fprintf(stderr, "Error leyendo durante handshake\n");
          close(sockfd);
          free(root_der);
          return 1;
        }

        size_t to_copy = (size_t)nread;
        if (to_copy > recvrec_len) to_copy = recvrec_len;

        memcpy(recvrec_buf, tmpbuf, to_copy);
        br_ssl_engine_recvrec_ack(&sc.eng, to_copy);
      }
      continue;
    }

    if (state & BR_SSL_SENDAPP) {
      // Handshake completado, podemos enviar datos de aplicación
      break;
    }
  }

  // 3.9) Enviar petición HTTP
  if (ssl_write_all(&sc.eng, sockfd, (const unsigned char*)request, (size_t)req_len) != 0) {
    fprintf(stderr, "Error enviando petición HTTP\n");
    close(sockfd);
    free(root_der);
    return 1;
  }

  // 3.10) Leer respuesta
  if (ssl_read_response(&sc.eng, sockfd) != 0) {
    fprintf(stderr, "Error leyendo respuesta HTTP\n");
    close(sockfd);
    free(root_der);
    return 1;
  }

  close(sockfd);
  free(root_der);
  return 0;
}