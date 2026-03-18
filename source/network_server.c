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
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "list_skel.h"
#include "message-private.h"
#include "network_server.h"
#include "server_log.h"
#include "client_stub.h"
#include "list_server-private.h"

#define MAX_CLIENTS 4

int client_sockets[MAX_CLIENTS] = {0};

int active_clients = 0;
volatile int server_shutdown = 0;
static int listening_fd = -1;
pthread_mutex_t nclient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_args
{
    int client_socket;
    struct list_t *list;
    struct sockaddr_in client_addr;
};

static void termina(void)
{
    pthread_mutex_lock(&nclient);
    active_clients--;
    pthread_mutex_unlock(&nclient);
}

/* Thread de atendimento a um cliente */
static void *client_handler(void *params)
{
    struct thread_args *args = (struct thread_args *)params;
    int socket = args->client_socket;
    struct list_t *list = args->list;

    printf("[Thread] Connection established! Active: %d\n", active_clients);

    MessageT *message;
    while ((message = network_receive(socket)) != 0)
    {
        int response = -1;
        if (server_info.head && server_info.next_server == NULL)
        {
            log_request(&args->client_addr, message);
            pthread_mutex_lock(&list_mutex);
            int rc = invoke(message, list);
            pthread_mutex_unlock(&list_mutex);
            if (rc < 0)
            {
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
        }
        else
        {
            switch (message->opcode)
            {
            case MESSAGE_T__OPCODE__OP_ADD:
                if (server_info.next_server != NULL)
                {
                    struct data_t *car = data_create(
                        message->data->ano,
                        message->data->preco,
                        message->data->marca,
                        message->data->modelo,
                        message->data->combustivel);
                    response = rlist_add(server_info.next_server, car);
                    data_destroy(car);
                }
                if (response == 0)
                {
                    log_request(&args->client_addr, message);
                }
                break;
            case MESSAGE_T__OPCODE__OP_DEL:
                if (server_info.next_server != NULL)
                {
                    response = rlist_remove_by_model(server_info.next_server, message->models[0]);
                }
                if (response == 0)
                {
                    log_request(&args->client_addr, message);
                }
                break;
            case MESSAGE_T__OPCODE__OP_ORDER:
                if (server_info.next_server != NULL)
                {
                    response = rlist_order_by_year(server_info.next_server);
                }
                if (response == 0)
                {
                    log_request(&args->client_addr, message);
                }
                break;
            default:
                if (!server_info.head)
                    log_request(&args->client_addr, message);
                response = -1;
            }
            if (response < 0 && server_info.next_server != NULL && !server_info.head && server_info.next_server != NULL)
            {
                fprintf(stderr, "Erro na propagacao da mensagem!\n");
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
            else
            {
                pthread_mutex_lock(&list_mutex);
                int rc = invoke(message, list);
                pthread_mutex_unlock(&list_mutex);
                if (rc < 0)
                {
                    message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                }
            }
        }

        network_send(socket, message);

        message_t__free_unpacked(message, NULL);
    }

    close(socket);
    log_close(&args->client_addr);
    printf("[Thread] Client disconnected.\n");
    pthread_mutex_lock(&nclient);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == socket)
        {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&nclient);
    termina();
    free(args);
    pthread_exit(NULL);
    return NULL;
}

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port)
{
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket\n");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(sockfd);
        return -1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, MAX_CLIENTS) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }
    log_init();
    listening_fd = sockfd;
    return sockfd;
}

/* A função network_main_loop() deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada
     na lista;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 * A função não deve retornar, a menos que ocorra algum erro. Nesse
 * caso retorna -1.
 */
int network_main_loop(int listening_socket, struct list_t *list)
{
    struct sockaddr_in client;
    socklen_t size_client = sizeof(client);

    printf("Server ready, waiting for connections\n");

    while (!server_shutdown)
    {

        int connfd = accept(listening_socket, (struct sockaddr *)&client, &size_client);
        if (connfd < 0)
        {
            if (server_shutdown)
                break;
            perror("Erro em accept");
            continue;
        }

        pthread_mutex_lock(&nclient);

        if (active_clients < MAX_CLIENTS)
        {

            log_connect(&client);

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = connfd;
                    break;
                }
            }

            MessageT ready_msg = MESSAGE_T__INIT;
            ready_msg.opcode = MESSAGE_T__OPCODE__OP_READY;
            ready_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            network_send(connfd, &ready_msg);

            active_clients++;
            pthread_mutex_unlock(&nclient);

            printf("[Server] Client accepted. Active: %d\n", active_clients);

            struct thread_args *args = malloc(sizeof(struct thread_args));
            if (args == NULL)
            {
                perror("malloc");
                close(connfd);

                pthread_mutex_lock(&nclient);
                active_clients--;
                pthread_mutex_unlock(&nclient);
                continue;
            }

            args->client_socket = connfd;
            args->list = list;
            memcpy(&args->client_addr, &client, sizeof(client));

            pthread_t tid;
            if (pthread_create(&tid, NULL, client_handler, args) != 0)
            {
                perror("pthread_create");
                close(connfd);
                free(args);

                pthread_mutex_lock(&nclient);
                active_clients--;
                pthread_mutex_unlock(&nclient);
                continue;
            }

            pthread_detach(tid);
        }
        else
        {
            pthread_mutex_unlock(&nclient);

            printf("[Server] Client rejected (occupied).\n");

            MessageT busy_msg = MESSAGE_T__INIT;
            busy_msg.opcode = MESSAGE_T__OPCODE__OP_BUSY;
            busy_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            network_send(connfd, &busy_msg);

            close(connfd);
        }
    }
    return 0;
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket)
{
    uint16_t net_msg_size;
    int r = read_all(client_socket, &net_msg_size, sizeof(net_msg_size));
    if (server_shutdown || r <= 0)
        return NULL;

    uint16_t msg_size = ntohs(net_msg_size);
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL)
        return NULL;

    r = read_all(client_socket, buffer, msg_size);
    if (r <= 0)
    {
        free(buffer);
        return NULL;
    }

    MessageT *msg = message_t__unpack(NULL, msg_size, buffer);
    free(buffer);
    return msg;
}

/* A função network_send() deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Enviar a mensagem serializada, através do client_socket.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_send(int client_socket, MessageT *msg)
{
    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL)
        return -1;
    message_t__pack(msg, buffer);

    uint16_t net_msg_size = htons(msg_size);
    if (write_all(client_socket, &net_msg_size, sizeof(net_msg_size)) < 0)
    {
        perror("Erro ao enviar tamanho da mensagem");
        free(buffer);
        return -1;
    }

    if (write_all(client_socket, buffer, msg_size) < 0)
    {
        perror("Erro ao enviar mensagem");
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

/* Liberta os recursos alocados por network_server_init(), nomeadamente
 * fechando o socket passado como argumento.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_server_close(int socket)
{
    if (close(socket) < 0)
    {
        perror("Erro ao fechar socket");
        return -1;
    }
    return 0;
}

/* Sinaliza término e fecha FDs internos (desbloqueia accept/read)*/
void network_server_request_shutdown(void)
{
    server_shutdown = 1;
    pthread_mutex_lock(&nclient);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] > 0)
        {
            shutdown(client_sockets[i], SHUT_RDWR);
            close(client_sockets[i]);
            client_sockets[i] = 0;
        }
    }
    pthread_mutex_unlock(&nclient);
    if (listening_fd >= 0)
    {
        network_server_close(listening_fd);
        listening_fd = -1;
    }
}
