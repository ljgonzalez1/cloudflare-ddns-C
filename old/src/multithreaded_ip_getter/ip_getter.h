#pragma once

/* Descarga el cuerpo de una URL (HTTP o HTTPS) con timeout en milisegundos.
 * Devuelve buffer dinámico o NULL. El llamador debe free().                 */
char *get_url_body(const char *url, int timeout_ms);