/**
 *
 * Projeto Etapa 4: Sistemas Distribuídos 2025/2026
 *
 * Grupo SD-22
 *
 * Autores:
 * - Guilherme Diniz, Nº 61824
 * - Henrique Pinto, Nº 61805
 * - Maria Leonor, Nº 61779
 */
#include <unistd.h>

#include "message-private.h"

int write_all(int sock, void *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = write(sock, (char *)buf + total, len - total);
        if (n <= 0)
            return -1;
        total += n;
    }
    return 0;
}

int read_all(int sock, void *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = read(sock, (char *)buf + total, len - total);
        if (n == 0)
        {
            return 0;
        }
        if (n < 0)
        {
            return -1;
        }
        total += n;
    }
    return 1;
}
