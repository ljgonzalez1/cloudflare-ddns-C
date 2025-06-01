#pragma once

#include <stdlib.h>
#include <stdio.h>

/**
 * Retrieves the value of an environment variable.
 *
 * @param name The name of the environment variable to retrieve.
 * @return A pointer to the value of the environment variable as a null-terminated string,
 *         or NULL if the variable is not defined.
 *
 * @note The returned pointer must not be modified or freed. It is managed by the system.
 */
const char *get_env_var(const char *variable_name);


