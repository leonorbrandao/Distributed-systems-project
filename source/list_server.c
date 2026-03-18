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
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server_log.h"
#include "list_skel.h"
#include "network_server.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include "list_server-private.h"

#include "zookeeper/zookeeper.h"

#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;

extern int active_clients;
extern pthread_mutex_t nclient;

struct server_info server_info;

void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *ctx)
{
    struct server_info *srv = (struct server_info *)ctx;

    if (state != ZOO_CONNECTED_STATE || type != ZOO_CHILD_EVENT)
    {
        return;
    }

    zoo_string children;
    if (ZOK != zoo_wget_children(zh, "/chain", child_watcher, srv, &children))
    {
        fprintf(stderr, "Error setting watch at /chain!\n");
        return;
    }

    char *min_sucessor = NULL;
    for (int i = 0; i < children.count; i++)
    {
        int seq1 = atoi(children.data[i] + 11);
        int seq2 = atoi(srv->znode_path + 11);
        if (seq1 > seq2)
        {
            if (!min_sucessor || atoi(children.data[i] + 11) < atoi(min_sucessor + 11))
            {
                min_sucessor = children.data[i];
            }
        }
    }
    if (min_sucessor)
    {
        char succ_path[256];
        snprintf(succ_path, sizeof(succ_path), "/chain/%s", min_sucessor);
        int zdata_len = ZDATALEN;
        char succ_ip[ZDATALEN];
        if (ZOK == zoo_get(srv->zh, succ_path, 0, succ_ip, &zdata_len, NULL))
        {
            succ_ip[zdata_len] = '\0';
            if (srv->next_server)
            {
                rlist_disconnect(srv->next_server);
            }
            printf("Atualizando sucessor para: %s\n", succ_ip);
            srv->next_server = rlist_connect(succ_ip);
            printf("Sucessor atualizado: %s\n", succ_ip);
        }
    }
    else
    {
        if (srv->next_server)
        {
            rlist_disconnect(srv->next_server);
            srv->next_server = NULL;
            printf("Não há sucessor. Este servidor é a cauda.\n");
        }
    }
    free(children.data);
}

void synchronize_from_predecessor(const char *pred_ip_port)
{
    if (!pred_ip_port)
        return;

    printf("Sincronizando lista a partir do predecessor: %s\n", pred_ip_port);

    struct rlist_t *pred = rlist_connect((char *)pred_ip_port);
    if (!pred)
    {
        fprintf(stderr, "Falha ao conectar ao predecessor\n");
        return;
    }

    struct data_t **resp = rlist_get_by_year(pred, -1);
    if (!resp)
    {
        fprintf(stderr, "Lista do predecessor vazia\n");
        rlist_disconnect(pred);
        return;
    }

    if (resp != NULL)
    {
        for (int i = 0; resp[i] != NULL; i++)
        {
            struct data_t *car = data_create(
                resp[i]->ano, resp[i]->preco, resp[i]->marca,
                resp[i]->modelo, resp[i]->combustivel);
            list_add(server_info.list, car);
            data_destroy(resp[i]);
        }
        free(resp);
    }
    rlist_disconnect(pred);
    pthread_mutex_unlock(&server_info.list_lock);

    printf("Sincronização concluída.\n");
}

/*
 * Função para tratar o sinal SIGINT (Ctrl+C) e fechar a ligação com o servidor antes de sair.
 */
void handle_sigint(int sig)
{
    network_server_request_shutdown();
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <porto_servidor> <ip:port_zookeeper>\n", argv[0]);
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);

    int port = atoi(argv[1]);
    if (port <= 0)
    {
        fprintf(stderr, "Porto inválido.\n");
        return -1;
    }

    server_info.list = list_skel_init();
    if (!server_info.list)
    {
        fprintf(stderr, "Erro ao inicializar lista local.\n");
        return -1;
    }

    pthread_mutex_init(&server_info.list_lock, NULL);
    server_info.next_server = NULL;
    server_info.head = 1;

    char host_port[256];
    snprintf(host_port, sizeof(host_port), "127.0.0.1:%d", port);
    server_info.local_ip_port = strdup(host_port);

    int sockfd = network_server_init(port);
    if (sockfd < 0)
    {
        fprintf(stderr, "Erro ao inicializar servidor TCP.\n");
        list_skel_destroy(server_info.list);
        return -1;
    }

    zoo_set_debug_level((ZooLogLevel)0);
    server_info.zh = zookeeper_init(argv[2], NULL, 2000, 0, NULL, 0);
    if (!server_info.zh)
    {
        fprintf(stderr, "Erro ao conectar ao ZooKeeper\n");
        list_skel_destroy(server_info.list);
        return -1;
    }

    struct Stat stat;
    int exists = zoo_exists(server_info.zh, "/chain", 0, &stat);
    if (exists != ZOK)
    {
        printf("Znode /chain não existe. A criar...\n");
        char buffer[10];
        int buffer_len = sizeof(buffer);

        int rc = zoo_create(server_info.zh, "/chain", "", 0,
                            &ZOO_OPEN_ACL_UNSAFE, 0, buffer, buffer_len);
        if (rc != ZOK && rc != ZNODEEXISTS)
        {
            fprintf(stderr, "Erro ao criar o znode pai /chain: %s\n", zerror(rc));
            zookeeper_close(server_info.zh);
            list_skel_destroy(server_info.list);
            return -1;
        }
        printf("Znode /chain criado com sucesso.\n");
    }

    char znode_name[256];
    snprintf(znode_name, sizeof(znode_name), "/chain/node");
    char path_buffer[256];
    if (ZOK != zoo_create(server_info.zh, znode_name, server_info.local_ip_port,
                          strlen(server_info.local_ip_port), &ZOO_OPEN_ACL_UNSAFE,
                          ZOO_EPHEMERAL | ZOO_SEQUENCE,
                          path_buffer, sizeof(path_buffer)))
    {
        fprintf(stderr, "Erro ao criar ZNode efémero sequencial\n");
        zookeeper_close(server_info.zh);
        list_skel_destroy(server_info.list);
        return -1;
    }
    server_info.znode_path = strdup(path_buffer);
    printf("ZNode criado: %s\n", server_info.znode_path);

    zoo_string children;
    if (ZOK != zoo_wget_children(server_info.zh, "/chain", child_watcher, &server_info, &children))
    {
        fprintf(stderr, "Erro ao obter filhos de /chain\n");
        zookeeper_close(server_info.zh);
        list_skel_destroy(server_info.list);
        return -1;
    }

    char *predecessor = NULL;
    server_info.next_server = NULL;
    char *min_sucessor = NULL;
    for (int i = 0; i < children.count; i++)
    {
        int seq1 = atoi(children.data[i] + 11);
        int seq2 = atoi(server_info.znode_path + 11);
        if (seq1 < seq2)
        {
            if (!predecessor || seq1 > atoi(predecessor + 11))
            {
                predecessor = children.data[i];
            }
        }
        else if (seq1 > seq2)
        {
            if (!min_sucessor || seq1 < atoi(min_sucessor + 11))
            {
                min_sucessor = children.data[i];
            }
        }
    }

    if (predecessor)
    {
        server_info.head = 0;
        char pred_path[256];
        snprintf(pred_path, sizeof(pred_path), "/chain/%s", predecessor);

        int zdata_len = ZDATALEN;
        char pred_ip[ZDATALEN];
        if (ZOK == zoo_get(server_info.zh, pred_path, 0, pred_ip, &zdata_len, NULL))
        {
            pred_ip[zdata_len] = '\0';
            synchronize_from_predecessor(pred_ip);
        }
    }

    if (min_sucessor)
    {
        char succ_path[256];
        snprintf(succ_path, sizeof(succ_path), "/chain/%s", min_sucessor);
        int zdata_len = ZDATALEN;
        char succ_ip[ZDATALEN];
        if (ZOK == zoo_get(server_info.zh, succ_path, 0, succ_ip, &zdata_len, NULL))
        {
            succ_ip[zdata_len] = '\0';
            server_info.next_server = rlist_connect(succ_ip);
            printf("Sucessor definido: %s\n", succ_ip);
        }
    }

    free(children.data);

    int ret = network_main_loop(sockfd, server_info.list);
    if (ret < 0)
    {
        fprintf(stderr, "Erro no ciclo principal do servidor.\n");
    }

    while (1)
    {
        pthread_mutex_lock(&server_info.list_lock);
        int ativos = active_clients;
        pthread_mutex_unlock(&server_info.list_lock);
        if (ativos == 0)
            break;
        sleep(1);
    }

    printf("Servidor fechado\n");
    if (server_info.next_server)
        rlist_disconnect(server_info.next_server);
    list_skel_destroy(server_info.list);
    log_shutdown();
    zookeeper_close(server_info.zh);
    free(server_info.znode_path);
    free(server_info.local_ip_port);
    return 0;
}