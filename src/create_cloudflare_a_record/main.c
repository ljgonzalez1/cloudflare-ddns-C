#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 8192

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
                         "POST %s HTTP/1.0\r\n"
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

  struct addrinfo hints, *res, *rp;
  int sockfd;
  char buffer[BUFFER_SIZE];
  int bytes_sent, bytes_received;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo("api.cloudflare.com", "80", &hints, &res) != 0) {
    perror("getaddrinfo");
    return 1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(sockfd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not connect to api.cloudflare.com\n");
    freeaddrinfo(res);
    return 1;
  }
  freeaddrinfo(res);

  // Send the request
  int total_sent = 0;
  while (total_sent < req_len) {
    bytes_sent = write(sockfd, request + total_sent, req_len - total_sent);
    if (bytes_sent <= 0) {
      perror("write");
      close(sockfd);
      return 1;
    }
    total_sent += bytes_sent;
  }

  // Read and print the response
  while ((bytes_received = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_received] = '\0';
    fputs(buffer, stdout);
  }

  close(sockfd);
  return 0;
}
