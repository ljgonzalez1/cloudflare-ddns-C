#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

void fetch_and_print(const char *host, const char *path) {
  struct addrinfo hints, *res, *rp;
  int sockfd;
  char request[256];
  char buffer[BUFFER_SIZE];
  int bytes_sent, bytes_received;

  // Build simple HTTP/1.0 GET request
  snprintf(request, sizeof(request),
           "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
           path, host);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, "80", &hints, &res) != 0) {
    perror("getaddrinfo");
    return;
  }

  // Iterate through addresses and connect
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(sockfd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not connect to %s\n", host);
    freeaddrinfo(res);
    return;
  }

  // Send HTTP request
  write(sockfd, request, strlen(request));

  // Read response and skip HTTP headers
  int header_passed = 0;
  while ((bytes_received = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
    buffer[bytes_received] = '\0';
    char *p = buffer;
    if (!header_passed) {
      char *header_end = strstr(buffer, "\r\n\r\n");
      if (header_end) {
        p = header_end + 4;
        header_passed = 1;
      } else {
        continue;
      }
    }
    fputs(p, stdout);
  }

  close(sockfd);
  freeaddrinfo(res);
}

int main(void) {
  // Fetch from ipinfo.io
  fetch_and_print("ipinfo.io", "/ip");
  printf("\n");
  // Fetch from api.ipify.org
  fetch_and_print("api.ipify.org", "/");
  printf("\n");
  // Fetch from ipv4.icanhazip.com
  fetch_and_print("ipv4.icanhazip.com", "/");
  printf("\n");
  return 0;
}
