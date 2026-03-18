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

#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <arpa/inet.h>
#include "sdmessage.pb-c.h"


/* Inicializa o sistema de logging, criando o ficheiro de log.
 * Deve ser chamada uma vez, no início do servidor.
 */
void log_init();

/* Regista no log a conexão de um cliente.
 * O argumento client indica o endereço do cliente que se conectou.
 */
void log_connect(struct sockaddr_in *client);

/* Regista no log o encerramento da conexão de um cliente.
 * O argumento client indica o endereço do cliente que se desconectou.
 */
void log_close(struct sockaddr_in *client);

/* Regista no log um pedido recebido de um cliente.
 * O argumento client indica o endereço do cliente que fez o pedido.
 * O argumento msg é a mensagem recebida do cliente.
 */
void log_request(struct sockaddr_in *client, MessageT *msg);

/* Encerra o sistema de logging, fechando o ficheiro de log.
 * Deve ser chamada uma vez, no final do servidor.
 */
void log_shutdown();

#endif
