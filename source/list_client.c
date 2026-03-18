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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_stub.h"
#include "data.h"
#include "zookeeper/zookeeper.h"

/* ZooKeeper Znode Data Length */
#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;

static zhandle_t *zh;
static char *head_node = NULL;
static char *tail_node = NULL;
struct rlist_t *rlist_head;
struct rlist_t *rlist_tail;

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string children_list;
    const char *zoo_chain = "/chain";
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {

            if (ZOK != zoo_wget_children(zh, zoo_chain, child_watcher, NULL, &children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", zoo_chain);
            }
            if (children_list.count == 0)
            {
                fprintf(stderr, "\nNo nodes found in %s!\n", zoo_chain);
                free(children_list.data);
                return;
            }
            char *min = NULL;
            char *max = NULL;
            for (int i = 0; i < children_list.count; i++)
            {
                char *node = children_list.data[i];
                if (min == NULL || strcmp(node, min) < 0)
                {
                    min = node;
                }
                if (max == NULL || strcmp(node, max) > 0)
                {
                    max = node;
                }
            }
            if (strcmp(head_node, min) != 0)
            {
                free(head_node);
                head_node = strdup(min);
                char head_path[256];
                snprintf(head_path, sizeof(head_path), "%s/%s", zoo_chain, head_node);
                int zdata_len_head = ZDATALEN;
                char *head_ip = calloc(ZDATALEN + 1, 1);
                if (head_ip == NULL)
                {
                    fprintf(stderr, "Memory allocation error!\n");
                    free(children_list.data);
                    return;
                }
                if (zoo_get(zh, head_path, 0, head_ip, &zdata_len_head, NULL) != ZOK)
                {
                    fprintf(stderr, "Error getting content from head node!\n");
                    free(head_ip);
                    free(children_list.data);
                    return;
                }
                if (zdata_len_head >= ZDATALEN)
                    zdata_len_head = ZDATALEN - 1;
                head_ip[zdata_len_head] = '\0';
                rlist_disconnect(rlist_head);
                rlist_head = rlist_connect(head_ip);
                printf("\nNova head: %s com IP %s\n", head_node, head_ip);
                printf("Comando: ");
                fflush(stdout);
                free(head_ip);
            }

            if (strcmp(tail_node, max) != 0)
            {
                free(tail_node);
                tail_node = strdup(max);
                char tail_path[256];
                snprintf(tail_path, sizeof(tail_path), "%s/%s", zoo_chain, tail_node);
                int zdata_len_tail = ZDATALEN;
                char *tail_ip = malloc(ZDATALEN);
                if (tail_ip == NULL)
                {
                    fprintf(stderr, "Memory allocation error!\n");
                    free(children_list.data);
                    return;
                }
                if (zoo_get(zh, tail_path, 0, tail_ip, &zdata_len_tail, NULL) != ZOK)
                {
                    fprintf(stderr, "Error getting content from tail node!\n");
                    free(tail_ip);
                    free(children_list.data);
                    return;
                }
                if (zdata_len_tail >= ZDATALEN)
                    zdata_len_tail = ZDATALEN - 1;
                tail_ip[zdata_len_tail] = '\0';
                rlist_disconnect(rlist_tail);
                rlist_tail = rlist_connect(tail_ip);

                printf("\nNova tail: %s com IP %s\n", tail_node, tail_ip);
                printf("Comando: ");
                fflush(stdout);
                free(tail_ip);
            }
        }
    }
    free(children_list.data);
}

/*
 * Função para tratar o sinal SIGINT (Ctrl+C) e fechar a ligação com o servidor antes de sair.
 */
void handle_sigint(int sig)
{
    rlist_disconnect(rlist_head);
    rlist_disconnect(rlist_tail);
    free(head_node);
    free(tail_node);
    zookeeper_close(zh);
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);
    if (argc != 2)
    {
        printf("Argumentos inválidos!\n Uso: ./%s <server>:<port>", argv[0]);
        return 1;
    }

    const char *host_port = argv[1];
    const char *zoo_chain = "/chain";
    zoo_string children_list;

    zoo_set_debug_level((ZooLogLevel)0);
    zh = zookeeper_init(host_port, NULL, 2000, 0, NULL, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        exit(EXIT_FAILURE);
    }

    if (zoo_wget_children(zh, zoo_chain, child_watcher, NULL, &children_list) != ZOK)
    {
        fprintf(stderr, "Error retrieving znode from path %s!\n", zoo_chain);
        free(children_list.data);
        zookeeper_close(zh);
        exit(EXIT_FAILURE);
    }
    if (children_list.count == 0)
    {
        fprintf(stderr, "No nodes found in %s!\n", zoo_chain);
        free(children_list.data);
        zookeeper_close(zh);
        exit(EXIT_FAILURE);
    }
    char *min = NULL;
    char *max = NULL;
    for (int i = 0; i < children_list.count; i++)
    {
        char *node = children_list.data[i];
        if (min == NULL || strcmp(node, min) < 0)
        {
            min = node;
        }
        if (max == NULL || strcmp(node, max) > 0)
        {
            max = node;
        }
    }
    head_node = strdup(min);
    tail_node = strdup(max);
    free(children_list.data);
    char head_path[256];
    char tail_path[256];
    snprintf(head_path, sizeof(head_path), "%s/%s", zoo_chain, head_node);
    snprintf(tail_path, sizeof(tail_path), "%s/%s", zoo_chain, tail_node);

    int zdata_len_head = ZDATALEN;
    char *head_ip = calloc(ZDATALEN + 1, 1);
    if (head_ip == NULL)
    {
        fprintf(stderr, "Memory allocation error!\n");
        free(head_node);
        free(tail_node);
        zookeeper_close(zh);
        exit(EXIT_FAILURE);
    }
    int zdata_len_tail = ZDATALEN;
    char *tail_ip = malloc(ZDATALEN);
    if (tail_ip == NULL)
    {
        fprintf(stderr, "Memory allocation error!\n");
        free(head_ip);
        free(head_node);
        free(tail_node);
        zookeeper_close(zh);
        exit(EXIT_FAILURE);
    }

    if (zoo_get(zh, head_path, 0, head_ip, &zdata_len_head, NULL) != ZOK || zoo_get(zh, tail_path, 0, tail_ip, &zdata_len_tail, NULL) != ZOK)
    {
        fprintf(stderr, "Error getting content from nodes!\n");
        free(head_ip);
        free(tail_ip);
        free(head_node);
        free(tail_node);
        zookeeper_close(zh);
        exit(EXIT_FAILURE);
    }
    rlist_head = rlist_connect(head_ip);
    rlist_tail = rlist_connect(tail_ip);
    if (rlist_head == NULL || rlist_tail == NULL)
    {
        free(head_ip);
        free(tail_ip);
        free(head_node);
        free(tail_node);
        zookeeper_close(zh);
        return 1;
    }

    if (zdata_len_head >= 1024 * 1024)
        zdata_len_head = 1024 * 1024 - 1;
    head_ip[zdata_len_head] = '\0';
    if (zdata_len_tail >= 1024 * 1024)
        zdata_len_tail = 1024 * 1024 - 1;
    tail_ip[zdata_len_tail] = '\0';
    printf("Ligado a %s na head e a %s na tail\n", head_ip, tail_ip);
    free(head_ip);
    free(tail_ip);

    printf("Comandos disponíveis:\n  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n  remove <modelo>\n  get_by_marca <marca:0-4>\n  get_by_year <ano>\n  get_list_ordered_by_year\n  size\n  get_model_list\n  help\n  quit\n");
    int n = 256;
    char command[256];
    while (1)
    {
        printf("Command: ");
        fgets(command, n, stdin);
        if (strcmp(command, "\n") == 0)
        {
            continue;
        }
        command[strcspn(command, "\n")] = 0;
        char *token = strtok(command, " ");
        if (strcmp(token, "add") == 0)
        {
            char *modelo = strtok(NULL, " ");
            char *ano_str = strtok(NULL, " ");
            char *preco_str = strtok(NULL, " ");
            char *marca_str = strtok(NULL, " ");
            char *combustivel_str = strtok(NULL, " ");

            if (modelo == NULL || ano_str == NULL || preco_str == NULL || marca_str == NULL || combustivel_str == NULL)
            {
                printf("Uso: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
                continue;
            }
            combustivel_str[strcspn(combustivel_str, "\n")] = '\0';

            char *endptr;
            int ano = (int)strtol(ano_str, &endptr, 10);
            if (*endptr != '\0')
            {
                printf("Erro: <ano> invállido: %s\n", ano_str);
                continue;
            }

            float preco = strtof(preco_str, &endptr);
            if (*endptr != '\0')
            {
                printf("Erro: <preco> inválido (use ponto decimal): %s\n", preco_str);
                continue;
            }

            int marca = (int)strtol(marca_str, &endptr, 10);
            if (*endptr != '\0' || marca < 0 || marca > 4)
            {
                printf("Erro: <marca> deve estar em [0..4]\n");
                continue;
            }

            int combustivel = (int)strtol(combustivel_str, &endptr, 10);
            if (*endptr != '\0' || combustivel < 0 || combustivel > 3)
            {
                printf("Erro: <combustivel> deve estar em [0..3]\n");
                continue;
            }

            struct data_t *car = data_create(ano, preco, marca, modelo, combustivel);
            int ret = rlist_add(rlist_head, car);
            if (ret == 0)
            {
                printf("Carro adicionado com sucesso.\n");
            }
            else
            {
                printf("Erro ao adicionar carro.\n");
            }
            data_destroy(car);
        }
        else if (strcmp(token, "remove") == 0)
        {
            char *modelo = strtok(NULL, " ");
            if (modelo == NULL)
            {
                printf("Uso: remove <modelo>\n");
                continue;
            }
            int ret = rlist_remove_by_model(rlist_head, modelo);
            if (ret == 0)
            {
                printf("Carro removido.\n");
            }
            else
            {
                printf("Erro ao remover carro.\n");
            }
        }
        else if (strcmp(token, "get_by_marca") == 0)
        {
            char *marca_str = strtok(NULL, " ");
            if (marca_str == NULL)
            {
                printf("Uso: get_by_marca <marca:0-4>\n");
                continue;
            }

            char *endptr;
            int marca = (int)strtol(marca_str, &endptr, 10);
            if (*endptr != '\0' || marca < 0 || marca > 4)
            {
                printf("Uso: get_by_marca <marca:0-4>\n");
                continue;
            }
            struct data_t *car = rlist_get_by_marca(rlist_tail, marca);
            if (car != NULL)
            {
                printf("Modelo: %s\nAno: %d\nPreço: %.2f\nMarca: %d\nCombustível: %d\n", car->modelo, car->ano, car->preco, car->marca, car->combustivel);
            }
            else
            {
                printf("Carro não encontrado.\n");
            }
            data_destroy(car);
        }
        else if (strcmp(token, "get_by_year") == 0)
        {
            char *ano_str = strtok(NULL, " ");
            if (ano_str == NULL)
            {
                printf("Uso: get_by_year <ano>\n");
                continue;
            }
            char *endptr;
            int ano = (int)strtol(ano_str, &endptr, 10);
            if (*endptr != '\0')
            {
                printf("Uso: get_by_year <ano>\n");
                continue;
            }
            struct data_t **cars = rlist_get_by_year(rlist_tail, ano);
            if (cars != NULL)
            {
                for (int i = 0; cars[i] != NULL; i++)
                {
                    printf("Modelo: %s\nAno: %d\nPreço: %.2f\nMarca: %d\nCombustível: %d\n", cars[i]->modelo, cars[i]->ano, cars[i]->preco, cars[i]->marca, cars[i]->combustivel);
                    data_destroy(cars[i]);
                }
            }
            else
            {
                printf("Nenhum carro encontrado para o ano especificado.\n");
            }
            free(cars);
        }
        else if (strcmp(token, "get_list_ordered_by_year") == 0)
        {
            if (rlist_order_by_year(rlist_head) == -1)
            {
                printf("Nenhum carro encontrado.\n");
                continue;
            }
            struct data_t **cars = rlist_get_by_year(rlist_tail, -1); // -1 para obter todos os carros
            if (cars != NULL)
            {
                for (int i = 0; cars[i] != NULL; i++)
                {
                    printf("Modelo: %s\nAno: %d\nPreço: %.2f\nMarca: %d\nCombustível: %d\n", cars[i]->modelo, cars[i]->ano, cars[i]->preco, cars[i]->marca, cars[i]->combustivel);
                    data_destroy(cars[i]);
                }
            }
            else
            {
                printf("Nenhum carro encontrado.\n");
            }
            free(cars);
        }
        else if (strcmp(token, "size") == 0)
        {
            int size = rlist_size(rlist_tail);
            if (size != -1)
            {
                printf("List size: %d\n", size);
            }
            else
            {
                printf("Erro ao obter o tamanho da lista.\n");
            }
        }
        else if (strcmp(token, "get_model_list") == 0)
        {
            char **models = rlist_get_model_list(rlist_tail);
            if (models != NULL)
            {
                for (int i = 0; models[i] != NULL; i++)
                    printf("Modelo: %s\n", models[i]);
            }
            else
            {
                printf("Erro ao obter lista de modelos.\n");
            }
            rlist_free_model_list(models);
        }
        else if (strcmp(token, "help") == 0)
        {
            printf("Comandos disponíveis:\n  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n  remove <modelo>\n  get_by_marca <marca:0-4>\n  get_by_year <ano>\n  get_list_ordered_by_year\n  size\n  get_model_list\n  help\n  quit\n");
        }
        else if (strcmp(token, "quit") == 0)
        {
            rlist_disconnect(rlist_head);
            rlist_disconnect(rlist_tail);
            free(head_node);
            free(tail_node);
            zookeeper_close(zh);
            return 0;
        }
        else
        {
            printf("Comando inválido. Escreve 'help' para ajuda.\n");
        }
    }
}