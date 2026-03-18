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
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_stub-private.h"
#include "client_stub.h"
#include "message-private.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rlist;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rlist;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rlist_t *rlist)
{
    if (rlist == NULL || rlist->server_address == NULL || rlist->server_port <= 0)
        return -1;

    if ((rlist->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(rlist->server_port);
    if (inet_pton(AF_INET, rlist->server_address, &server.sin_addr) <= 0)
    {
        perror("Erro ao converter endereço IP");
        close(rlist->sockfd);
        return -1;
    }

    if (connect(rlist->sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Error in connect\n");
        close(rlist->sockfd);
        return -1;
    }

    uint16_t size_net;
    if (read_all(rlist->sockfd, &size_net, sizeof(uint16_t)) <= 0)
    {
        perror("Erro ao ler tamanho da resposta");
        close(rlist->sockfd);
        return -1;
    }

    uint16_t size = ntohs(size_net);
    uint8_t *buffer = malloc(size);
    if (buffer == NULL)
    {
        close(rlist->sockfd);
        return -1;
    }

    if (read_all(rlist->sockfd, buffer, size) <= 0)
    {
        perror("Erro ao ler resposta");
        free(buffer);
        close(rlist->sockfd);
        return -1;
    }

    MessageT *msg = message_t__unpack(NULL, size, buffer);
    free(buffer);

    if (msg == NULL)
    {
        printf("Erro ao fazer unpack da resposta.\n");
        close(rlist->sockfd);
        return -1;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_BUSY)
    {
        printf("Server busy. Try again later.\n");
        message_t__free_unpacked(msg, NULL);
        close(rlist->sockfd);
        return -1;
    }

    if (msg->opcode != MESSAGE_T__OPCODE__OP_READY)
    {
        printf("Erro, mensagem inesperada do servidor.\n");
        message_t__free_unpacked(msg, NULL);
        close(rlist->sockfd);
        return -1;
    }

    message_t__free_unpacked(msg, NULL);
    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rlist_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Tratar de forma apropriada erros de comunicação;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg)
{
    if (rlist == NULL || msg == NULL)
        return NULL;

    int sock = rlist->sockfd;

    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *msg_buf = malloc(msg_size);
    if (msg_buf == NULL)
        return NULL;

    message_t__pack(msg, msg_buf);

    uint16_t size_net = htons(msg_size);
    if (write_all(sock, &size_net, sizeof(uint16_t)) < 0)
    {
        if (errno == EPIPE)
            printf("Servidor desligado. Feche a ligação.\n");
        else
            perror("Erro ao enviar tamanho da mensagem");
        free(msg_buf);
        return NULL;
    }

    if (write_all(sock, msg_buf, msg_size) < 0)
    {
        if (errno == EPIPE)
            printf("Servidor desligado. Feche a ligação.\n");
        else
            perror("Erro ao enviar mensagem");
        free(msg_buf);
        return NULL;
    }

    free(msg_buf);

    uint16_t reply_size_net;
    int n = read_all(sock, &reply_size_net, sizeof(uint16_t));
    if (n <= 0)
    {
        if (n == 0)
            printf("Servidor desligado. Feche a ligação.\n");
        else
            perror("Erro ao ler tamanho da resposta");
        return NULL;
    }

    uint16_t reply_size = ntohs(reply_size_net);

    uint8_t *reply_buf = malloc(reply_size);
    if (reply_buf == NULL)
        return NULL;

    n = read_all(sock, reply_buf, reply_size);
    if (n <= 0)
    {
        if (n == 0)
            printf("Servidor desligado. Feche a ligação.\n");
        else
            perror("Erro ao ler resposta");
        free(reply_buf);
        return NULL;
    }

    MessageT *response = message_t__unpack(NULL, reply_size, reply_buf);
    free(reply_buf);

    if (response == NULL)
    {
        printf("Erro ao fazer unpack da resposta.\n");
        return NULL;
    }

    return response;
}

/* Fecha a ligação estabelecida por network_connect().
 * Retorna 0 (OK) ou -1 (erro).
 */
int network_close(struct rlist_t *rlist)
{
    if (close(rlist->sockfd) < 0)
    {
        perror("close");
        return -1;
    }
    return 0;
}