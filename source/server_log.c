/**
 * Projeto Etapa 4: Sistemas Distribuídos 2025/2026
 *
 * Grupo SD-22
 *
 * Autores:
 * - Guilherme Diniz, Nº 61824
 * - Henrique Pinto, Nº 61805
 * - Maria Leonor, Nº 61779
 */
#include "server_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/file.h>
#include <pthread.h>
#include <string.h>
#include "list_server-private.h"

static FILE *logf = NULL;

static long timestamp_seconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void log_init()
{
    char *ext = ".log";
    size_t size = strlen(server_info.local_ip_port) + strlen(ext) + 1;
    char *filename = malloc(size);
    if (!filename)
    {
        perror("malloc");
        exit(1);
    }
    snprintf(filename, size, "%s%s", server_info.local_ip_port, ext);
    logf = fopen(filename, "w");
    if (!logf)
    {
        free(filename);
        perror("server.log");
        exit(1);
    };
    free(filename);
}

static void write_line(const char *line)
{
    int fd = fileno(logf);

    flock(fd, LOCK_EX);
    fputs(line, logf);
    fflush(logf);
    flock(fd, LOCK_UN);
}

void log_connect(struct sockaddr_in *client)
{
    char line[256];
    snprintf(line, sizeof(line),
             "%ld %s:%d CONNECT\n",
             timestamp_seconds(),
             inet_ntoa(client->sin_addr),
             ntohs(client->sin_port));
    write_line(line);
}

void log_close(struct sockaddr_in *client)
{
    char line[256];
    snprintf(line, sizeof(line),
             "%ld %s:%d CLOSE\n",
             timestamp_seconds(),
             inet_ntoa(client->sin_addr),
             ntohs(client->sin_port));
    write_line(line);
}

static const char *opcode_to_str(int op)
{
    switch (op)
    {
    case MESSAGE_T__OPCODE__OP_ADD:
        return "OP_ADD";
    case MESSAGE_T__OPCODE__OP_GET:
        return "OP_GET";
    case MESSAGE_T__OPCODE__OP_DEL:
        return "OP_DEL";
    case MESSAGE_T__OPCODE__OP_SIZE:
        return "OP_SIZE";
    case MESSAGE_T__OPCODE__OP_GETMODELS:
        return "OP_GETMODELS";
    case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
        return "OP_GETLISTBYTEAR";
    case MESSAGE_T__OPCODE__OP_ORDER:
        return "OP_ORDER";
    case MESSAGE_T__OPCODE__OP_ERROR:
        return "OP_ERROR";
    case MESSAGE_T__OPCODE__OP_BUSY:
        return "OP_BUSY";
    case MESSAGE_T__OPCODE__OP_READY:
        return "OP_READY";
    default:
        return "OP_BAD";
    }
}

static const char *ctype_to_str(int ct)
{
    switch (ct)
    {
    case MESSAGE_T__C_TYPE__CT_DATA:
        return "CT_DATA";
    case MESSAGE_T__C_TYPE__CT_MARCA:
        return "CT_MARCA";
    case MESSAGE_T__C_TYPE__CT_YEAR:
        return "CT_YEAR";
    case MESSAGE_T__C_TYPE__CT_MODEL:
        return "CT_MODEL";
    case MESSAGE_T__C_TYPE__CT_RESULT:
        return "CT_RESULT";
    case MESSAGE_T__C_TYPE__CT_LIST:
        return "CT_LIST";
    case MESSAGE_T__C_TYPE__CT_NONE:
        return "CT_NONE";
    default:
        return "CT_BAD";
    }
}

static const char *marca_to_str(int m)
{
    switch (m)
    {
    case MARCA__MARCA_TOYOTA:
        return "TOYOTA";
    case MARCA__MARCA_BMW:
        return "BMW";
    case MARCA__MARCA_RENAULT:
        return "RENAULT";
    case MARCA__MARCA_AUDI:
        return "AUDI";
    case MARCA__MARCA_MERCEDES:
        return "MERCEDES";
    default:
        return "NODATA";
    }
}

static const char *comb_to_str(int c)
{
    switch (c)
    {
    case COMBUSTIVEL__COMBUSTIVEL_GASOLINA:
        return "GASOLINA";
    case COMBUSTIVEL__COMBUSTIVEL_GASOLEO:
        return "GASOLEO";
    case COMBUSTIVEL__COMBUSTIVEL_ELETRICO:
        return "ELETRICO";
    case COMBUSTIVEL__COMBUSTIVEL_HIBRIDO:
        return "HIBRIDO";
    default:
        return "NODATA";
    }
}

static void format_argument(MessageT *msg, char *out, size_t len)
{
    out[0] = '\0';

    switch (msg->c_type)
    {

    case MESSAGE_T__C_TYPE__CT_DATA:
        snprintf(out, len, "%d %.2f %s %s %s",
                 msg->data->ano,
                 msg->data->preco,
                 marca_to_str(msg->data->marca),
                 msg->data->modelo,
                 comb_to_str(msg->data->combustivel));
        break;

    case MESSAGE_T__C_TYPE__CT_MARCA:
        snprintf(out, len, "%s", marca_to_str(msg->result));
        break;

    case MESSAGE_T__C_TYPE__CT_YEAR:
        snprintf(out, len, "%d", msg->result);
        break;

    case MESSAGE_T__C_TYPE__CT_MODEL:
        snprintf(out, len, "%s", msg->models[0]);
        break;

    case MESSAGE_T__C_TYPE__CT_RESULT:
        snprintf(out, len, "%d", msg->result);
        break;

    case MESSAGE_T__C_TYPE__CT_LIST:
        snprintf(out, len, "[%ld items]", msg->n_models);
        break;

    case MESSAGE_T__C_TYPE__CT_NONE:
    default:
        memset(out, 0, len);
        break;
    }
}

void log_request(struct sockaddr_in *client, MessageT *msg)
{
    char arg[512];
    format_argument(msg, arg, sizeof(arg));

    char line[1024];
    snprintf(line, sizeof(line),
             "%ld %s:%d REQUEST %s %s %s\n",
             timestamp_seconds(),
             inet_ntoa(client->sin_addr),
             ntohs(client->sin_port),
             opcode_to_str(msg->opcode),
             ctype_to_str(msg->c_type),
             arg);

    write_line(line);
}

void log_shutdown()
{
    if (logf)
        fclose(logf);
    logf = NULL;
}
