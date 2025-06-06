#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

// Hardcoded JSON payload
#define JSON_PAYLOAD "{\"field1\":\"value1\",\"field2\":42}"

void post_json(const char *host, const char *port, const char *path, const char *json_body) {
  struct addrinfo hints, *res, *rp;
  int sockfd;
  char request[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  int bytes_sent, bytes_received;

  size_t body_len = strlen(json_body);

  // Build HTTP/1.0 POST request with JSON payload
  int req_len = snprintf(request, sizeof(request),
                         "POST %s HTTP/1.0\r\n"
                         "Host: %s:%s\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         path, host, port, body_len, json_body);

  if (req_len >= sizeof(request)) {
    fprintf(stderr, "Request buffer too small\n");
    return;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port, &hints, &res) != 0) {
    perror("getaddrinfo");
    return;
  }

  // Connect to the server
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(sockfd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not connect to %s:%s\n", host, port);
    freeaddrinfo(res);
    return;
  }

  // Send HTTP request
  size_t total_sent = 0;
  while (total_sent < (size_t)req_len) {
    bytes_sent = write(sockfd, request + total_sent, req_len - total_sent);
    if (bytes_sent <= 0) {
      perror("write");
      close(sockfd);
      freeaddrinfo(res);
      return;
    }
    total_sent += bytes_sent;
  }

  // Read and print response
  while ((bytes_received = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
    buffer[bytes_received] = '\0';
    fputs(buffer, stdout);
  }

  close(sockfd);
  freeaddrinfo(res);
}

int main(void) {
  // Send JSON POST to 10.0.7.9:8000 at path "/"
  post_json("10.0.7.9", "8000", "/", JSON_PAYLOAD);
  return 0;
}
