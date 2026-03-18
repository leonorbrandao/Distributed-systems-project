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
#ifndef _LIST_H
#define _LIST_H /* Módulo list */

#include "list.h"
#include "data.h"
#include "list-private.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cria e inicializa uma nova lista de carros.
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create()
{
    struct list_t *list = (struct list_t *)malloc(sizeof(struct list_t));
    if (list == NULL)
        return NULL;
    list->size = 0;
    list->head = NULL;
    return list;
}

/* Elimina a lista, libertando toda a memória ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *list)
{
    if (list == NULL)
        return -1;

    struct car_t *current = list->head;
    struct car_t *next;

    while (current != NULL)
    {
        next = current->next;
        data_destroy(current->data);
        free(current);
        current = next;
    }

    free(list);
    return 0;
}

/* Adiciona um novo carro à lista.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_add(struct list_t *list, struct data_t *car)
{
    if (list == NULL || car == NULL)
        return -1;

    struct car_t *new_car = (struct car_t *)malloc(sizeof(struct car_t));
    if (new_car == NULL)
        return -1;

    new_car->data = car;
    new_car->next = NULL;

    if (list->head == NULL)
    {
        list->head = new_car;
    }
    else
    {
        struct car_t *current = list->head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_car;
    }

    list->size++;
    return 0;
}

/* Remove da lista o primeiro carro que corresponda ao modelo indicado.
 * Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
 */
int list_remove_by_model(struct list_t *list, const char *modelo)
{
    if (list == NULL || modelo == NULL)
        return -1;

    struct car_t *current = list->head;
    struct car_t *last = NULL;

    while (current != NULL)
    {
        if (strcmp(current->data->modelo, modelo) == 0)
        {
            if (last == NULL)
            {
                list->head = current->next;
            }
            else
            {
                last->next = current->next;
            }
            data_destroy(current->data);
            free(current);
            list->size--;
            return 0;
        }
        last = current;
        current = current->next;
    }
    return 1;
}

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca)
{
    if (list == NULL)
        return NULL;

    struct car_t *current = list->head;
    while (current != NULL)
    {
        if (current->data->marca == marca)
        {
            return current->data;
        }
        current = current->next;
    }
    return NULL;
}

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **list_get_by_year(struct list_t *list, int ano)
{
    if (list == NULL || ano <= 0)
        return NULL;

    int count = 0;
    struct car_t *current = list->head;
    while (current != NULL)
    {
        if (current->data->ano == ano)
        {
            count++;
        }
        current = current->next;
    }

    struct data_t **cars_same_year = (struct data_t **)malloc((count + 1) * sizeof(struct data_t *));
    if (cars_same_year == NULL)
        return NULL;

    current = list->head;
    int index = 0;
    while (current != NULL)
    {
        if (current->data->ano == ano)
        {
            cars_same_year[index++] = current->data;
        }
        current = current->next;
    }
    cars_same_year[index] = NULL;

    return cars_same_year;
}

/* Ordena a lista de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_order_by_year(struct list_t *list)
{
    if (list == NULL)
        return -1;

    bool swapped;
    struct car_t *ptr;
    do
    {
        swapped = false;
        ptr = list->head;
        struct car_t *last = NULL;

        while (ptr != NULL && ptr->next != last)
        {
            if (ptr->data->ano > ptr->next->data->ano)
            {
                struct data_t *temp = ptr->data;
                ptr->data = ptr->next->data;
                ptr->next->data = temp;
                swapped = true;
            }
            ptr = ptr->next;
        }
        last = ptr;
    } while (swapped);

    return 0;
}

/* Retorna o número de carros na lista ou -1 em caso de erro.
 */
int list_size(struct list_t *list)
{
    if (list == NULL)
        return -1;
    return list->size;
}

/* Constrói um array de strings com os modelos dos carros na lista.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **list_get_model_list(struct list_t *list)
{
    if (list == NULL)
        return NULL;

    char **models = (char **)malloc((list->size + 1) * sizeof(char *));
    if (models == NULL)
        return NULL;

    struct car_t *current = list->head;
    int index = 0;
    while (current != NULL)
    {
        models[index++] = strdup(current->data->modelo);
        current = current->next;
    }
    models[index] = NULL;

    return models;
}

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_model_list(char **models)
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

/* Devolve um array de ponteiros (terminado em NULL) para TODOS os carros.
 * Cada elemento do array aponta para dados internos da lista (não duplicados).
 * Cabe a quem chama libertar APENAS o array (free(result)), nunca os data_t*.
 * Retorna NULL em caso de erro.
 */
struct data_t **list_get_all(struct list_t *list)
{
    if (list == NULL)
        return NULL;

    struct data_t **all_cars = (struct data_t **)malloc((list->size + 1) * sizeof(struct data_t *));
    if (all_cars == NULL)
        return NULL;

    struct car_t *current = list->head;
    int index = 0;
    while (current != NULL)
    {
        all_cars[index++] = current->data;
        current = current->next;
    }
    all_cars[index] = NULL;

    return all_cars;
}

#endif
