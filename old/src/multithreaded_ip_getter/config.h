#pragma once
/* ───────── políticas de reintento / timeout ───────── */
#define MAX_THREAD_GET_ATTEMPTS     5           /* veces por URL          */
#define THREAD_TASK_RETRY_TIME_MS   3000        /* 3 s entre intentos     */
#define HTTP_REQUEST_TIMEOUT_MS     15000       /* 15 s por petición      */
