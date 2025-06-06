#pragma once
#include <stdbool.h>

/**
 * Busca la primera coincidencia IPv4 (“x.x.x.x” 0-255) dentro del texto `raw`.
 * Devuelve una copia *malloc* de esa IP, o NULL si no encuentra ninguna.
 * Caller debe `free()`.
 */
char *extract_first_ipv4(const char *raw);

/* ↓ siguen disponibles (aún los usa quien quiera) */
char *strip_noise(const char *raw);
bool   is_valid_ipv4(const char *ip);
