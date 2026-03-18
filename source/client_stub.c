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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_stub-private.h"
#include "client_stub.h"
#include "data.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"

/* Remote list, deve conter as informações necessárias para comunicar
 * com o servidor. A definir pelo grupo em client_stub-private.h
 */

struct rlist_t; /* Definida em client_stub-private.h */

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rlist preenchida, ou NULL em caso de erro.
 */
struct rlist_t *rlist_connect(char *address_port)
{
    if (address_port == NULL)
        return NULL;

    struct rlist_t *rlist = malloc(sizeof(struct rlist_t));
    if (rlist == NULL)
        return NULL;

    char *buf = strdup(address_port);
    if (buf == NULL)
    {
        free(rlist);
        return NULL;
    }

    char *address = strtok(buf, ":");
    char *port = strtok(NULL, ":");
    if (address == NULL || port == NULL)
    {
        free(buf);
        free(rlist);
        return NULL;
    }

    rlist->server_address = strdup(address);
    if (rlist->server_address == NULL)
    {
        free(buf);
        free(rlist);
        return NULL;
    }
    rlist->server_port = atoi(port);
    rlist->sockfd = -1;

    free(buf);

    if (network_connect(rlist) < 0)
    {
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }

    return rlist;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */

int rlist_disconnect(struct rlist_t *rlist)
{
    if (rlist == NULL)
        return -1;

    int ret = network_close(rlist);

    free(rlist->server_address);
    free(rlist);

    return (ret < 0) ? -1 : 0;
}

/* Adiciona um novo carro à lista remota.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_add(struct rlist_t *rlist, struct data_t *car)
{
    if (rlist == NULL || car == NULL)
    {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;

    Data data = DATA__INIT;
    data.ano = car->ano;
    data.preco = car->preco;
    data.marca = car->marca;
    data.modelo = strdup(car->modelo);
    data.combustivel = car->combustivel;

    msg.data = &data;

    MessageT *res = network_send_receive(rlist, &msg);

    free(data.modelo);

    if (res == NULL)
    {
        return -1;
    }

    int ret = (res->opcode == msg.opcode + 1) ? 0 : -1;

    message_t__free_unpacked(res, NULL);

    return ret;
}

/* Remove da lista remota o primeiro carro que corresponda ao modelo indicado.
 * Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
 */
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo)
{
    if (rlist == NULL || modelo == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;

    msg.n_models = 1;
    msg.models = malloc(sizeof(char *));
    if (msg.models == NULL)
    {
        return -1;
    }
    msg.models[0] = strdup(modelo);
    if (msg.models[0] == NULL)
    {
        free(msg.models);
        return -1;
    }

    MessageT *res = network_send_receive(rlist, &msg);

    if (res == NULL)
    {
        free(msg.models[0]);
        free(msg.models);
        return -1;
    }

    int ret;
    if (res->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        ret = -1;
    }
    else if (res->opcode == msg.opcode + 1)
    {
        ret = 0;
    }

    message_t__free_unpacked(res, NULL);
    free(msg.models[0]);
    free(msg.models);
    return ret;
}

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca)
{
    if (rlist == NULL)
        return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;

    msg.result = marca;

    MessageT *res = network_send_receive(rlist, &msg);
    if (res == NULL)
        return NULL;

    struct data_t *out = NULL;
    if (res->opcode == msg.opcode + 1 && res->data != NULL)
    {
        out = malloc(sizeof(struct data_t));
        out->ano = res->data->ano;
        out->preco = res->data->preco;
        out->marca = res->data->marca;
        out->modelo = strdup(res->data->modelo);
        out->combustivel = res->data->combustivel;
    }

    message_t__free_unpacked(res, NULL);
    return out;
}

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano)
{
    if (rlist == NULL)
    {
        return NULL;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;

    msg.result = ano;

    MessageT *res = network_send_receive(rlist, &msg);

    if (res == NULL)
    {
        return NULL;
    }

    struct data_t **cars = NULL;
    if (res->opcode == msg.opcode + 1 && res->n_cars > 0)
    {
        cars = calloc(res->n_cars + 1, sizeof(struct data_t *));

        if (cars == NULL)
        {
            message_t__free_unpacked(res, NULL);
            return NULL;
        }

        for (int i = 0; i < res->n_cars; i++)
        {
            cars[i] = malloc(sizeof(struct data_t));
            cars[i]->ano = res->cars[i]->ano;
            cars[i]->preco = res->cars[i]->preco;
            cars[i]->marca = res->cars[i]->marca;
            cars[i]->modelo = strdup(res->cars[i]->modelo);
            cars[i]->combustivel = res->cars[i]->combustivel;
        }
        cars[res->n_cars] = NULL;
    }

    message_t__free_unpacked(res, NULL);
    return cars;
}

/* Ordena a lista remota de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_order_by_year(struct rlist_t *rlist)
{
    if (rlist == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ORDER;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *res = network_send_receive(rlist, &msg);

    if (res == NULL)
        return -1;

    int ret = (res->opcode == msg.opcode + 1) ? 0 : -1;

    message_t__free_unpacked(res, NULL);

    return ret;
}

/* Retorna o número de carros na lista remota ou -1 em caso de erro.
 */
int rlist_size(struct rlist_t *rlist)
{
    if (rlist == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *res = network_send_receive(rlist, &msg);

    if (res == NULL)
        return -1;

    int size = res->result;

    message_t__free_unpacked(res, NULL);

    return size;
}

/* Constrói um array de strings com os modelos dos carros na lista remota.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **rlist_get_model_list(struct rlist_t *rlist)
{
    if (rlist == NULL)
        return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *res = network_send_receive(rlist, &msg);

    if (res == NULL)
        return NULL;

    char **models = NULL;
    if (res->opcode == msg.opcode + 1 && res->n_models > 0)
    {
        models = calloc(res->n_models + 1, sizeof(char *));
        if (models == NULL)
        {
            message_t__free_unpacked(res, NULL);
            return NULL;
        }
        for (int i = 0; i < res->n_models; i++)
        {
            models[i] = strdup(res->models[i]);
        }
        models[res->n_models] = NULL;
    }

    message_t__free_unpacked(res, NULL);

    return models;
}

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_free_model_list(char **models)
{
    if (models == NULL)
        return -1;

    for (int i = 0; models[i] != NULL; i++)
    {
        free(models[i]);
    }
    free(models);
    return 0;
}