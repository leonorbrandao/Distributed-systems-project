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
#include <unistd.h>

#include "data.h"
#include "list.h"
#include "list_skel.h"
#include "sdmessage.pb-c.h"

/* Inicia o skeleton da lista.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct list_t *list_skel_init()
{
    struct list_t *list = list_create();
    if (list == NULL)
        return NULL;
    return list;
}

/* Liberta toda a memória ocupada pela lista e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_skel_destroy(struct list_t *list)
{
    if (list == NULL)
        return -1;
    list_destroy(list);
    return 0;
}

/* Executa na lista a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int invoke(MessageT *msg, struct list_t *list)
{
    if (!msg || !list)
    {
        return -1;
    }

    switch (msg->opcode)
    {
    case MESSAGE_T__OPCODE__OP_ADD:
        if (!msg->data)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        {
            struct data_t *data = data_create(
                msg->data->ano,
                msg->data->preco,
                msg->data->marca,
                msg->data->modelo,
                msg->data->combustivel);
            if (!data || list_add(list, data) < 0)
            {
                if (data)
                    free(data);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            msg->opcode = MESSAGE_T__OPCODE__OP_ADD + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }

    case MESSAGE_T__OPCODE__OP_DEL:
        if (!msg->models || msg->n_models < 1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        if (list_remove_by_model(list, msg->models[0]) < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        return 0;

    case MESSAGE_T__OPCODE__OP_GET:
        if (msg->result < 0 || msg->result > 4)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        {
            struct data_t *car = list_get_by_marca(list, msg->result);
            if (!car)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            msg->data = malloc(sizeof(Data));
            if (msg->data == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            data__init(msg->data);
            msg->data->ano = car->ano;
            msg->data->preco = car->preco;
            msg->data->marca = car->marca;
            msg->data->modelo = strdup(car->modelo);
            msg->data->combustivel = car->combustivel;

            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            return 0;
        }

    case MESSAGE_T__OPCODE__OP_SIZE:
    {
        int size = list_size(list);
        if (size < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = size;
        return 0;
    }

    case MESSAGE_T__OPCODE__OP_ORDER:
        if (list_order_by_year(list) < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_ORDER + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return 0;

    case MESSAGE_T__OPCODE__OP_GETMODELS:
        char **models = list_get_model_list(list);
        if (models == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        int count = 0;
        while (models[count] != NULL)
        {
            count++;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_GETMODELS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_MODEL;
        msg->n_models = count;
        msg->models = malloc(count * sizeof(char *));
        for (int i = 0; i < count; i++)
        {
            msg->models[i] = strdup(models[i]);
            free(models[i]);
        }
        free(models);
        return 0;

    case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
    {

        struct data_t **cars;
        if (msg->result == -1)
        {
            msg->n_cars = list_size(list);
            cars = list_get_all(list);
            if (cars == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        }

        else
        {
            cars = list_get_by_year(list, msg->result);
            if (cars == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            int count = 0;
            while (cars[count] != NULL)
            {
                count++;
            }
            msg->n_cars = count;
        }

        msg->cars = malloc(msg->n_cars * sizeof(Data *));
        if (msg->cars == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        for (int i = 0; i < msg->n_cars; i++)
        {
            msg->cars[i] = malloc(sizeof(Data));
            if (msg->cars[i] == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            data__init(msg->cars[i]);
            msg->cars[i]->ano = cars[i]->ano;
            msg->cars[i]->preco = cars[i]->preco;
            msg->cars[i]->marca = cars[i]->marca;
            msg->cars[i]->modelo = strdup(cars[i]->modelo);
            msg->cars[i]->combustivel = cars[i]->combustivel;
        }
        free(cars);

        msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_YEAR;
        return 0;
    }

    default:
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }
}