/* File: src/ip_format_checker/main.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * validate_ipv4:
 *   Verifica si la cadena 'ip' representa una dirección IPv4 válida.
 *   Criterios:
 *     - Debe haber exactamente 4 octetos separados por '.'
 *     - Cada octeto solo puede tener entre 1 y 3 dígitos (0–9)
 *     - Su valor numérico debe estar entre 0 y 255
 *
 * Retorna 1 si es válida, 0 en caso contrario.
 */
int validate_ipv4(const char *ip) {
  if (ip == NULL) {
    return 0;
  }

  size_t length = strlen(ip);
  /* Mínimo 7 caracteres ("0.0.0.0"), máximo 15 ("255.255.255.255") */
  if (length < 7 || length > 15) {
    return 0;
  }

  /* Hacemos copia local para no modificar la cadena original */
  char buffer[16];
  strncpy(buffer, ip, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';

  int parts = 0;
  char *cursor = buffer;
  char *token;

  while ((token = strtok(cursor, ".")) != NULL) {
    cursor = NULL;  /* A partir de ahora strtok recibe NULL */

    parts++;
    if (parts > 4) {
      return 0;  /* Más de cuatro octetos */
    }

    size_t tok_len = strlen(token);
    /* Cada octeto debe tener entre 1 y 3 dígitos */
    if (tok_len == 0 || tok_len > 3) {
      return 0;
    }

    /* Verificar que todos los caracteres sean dígitos y convertir a número */
    int value = 0;
    for (size_t i = 0; i < tok_len; i++) {
      if (!isdigit((unsigned char)token[i])) {
        return 0;
      }
      value = value * 10 + (token[i] - '0');
    }

    /* Cada octeto debe estar entre 0 y 255 */
    if (value < 0 || value > 255) {
      return 0;
    }
  }

  /* Debe haber exactamente cuatro partes separadas por puntos */
  return (parts == 4) ? 1 : 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <direccion_ip>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *ip_address = argv[1];
  if (validate_ipv4(ip_address)) {
    printf("'%s' es una IP válida.\n", ip_address);
    return EXIT_SUCCESS;
  } else {
    printf("'%s' NO es una IP válida.\n", ip_address);
    return EXIT_FAILURE;
  }
}
