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

#include "data.h"

/* Função que cria um novo elemento de dados data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel)
{
    struct data_t *d;
    if (ano <= 0)
        return NULL;
    d = (struct data_t *)malloc(sizeof(struct data_t));
    if (d == NULL)
        return NULL;

    d->ano = ano;
    d->preco = preco;
    d->marca = marca;
    d->combustivel = combustivel;
    d->modelo = strdup(modelo);
    return d;
}

/* Função que elimina um bloco de dados, libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_destroy(struct data_t *data)
{
    if (data == NULL)
        return -1;
    free(data->modelo);
    free(data);
    return 0;
}

/* Função que duplica uma estrutura data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_dup(struct data_t *data)
{
    struct data_t *d;

    if (data == NULL)
        return NULL;
    d = (struct data_t *)malloc(sizeof(struct data_t));
    if (d == NULL)
        return NULL;

    d->ano = data->ano;
    d->preco = data->preco;
    d->marca = data->marca;
    d->combustivel = data->combustivel;
    d->modelo = strdup(data->modelo);

    return d;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel)
{
    if (ano <= 0)
        return -1;

    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->combustivel = combustivel;
    free(data->modelo);
    data->modelo = strdup(modelo);

    return 0;
}
