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
#ifndef _LIST_SERVER_PRIVATE_H
#define _LIST_SERVER_PRIVATE_H

#include <pthread.h>
#include <zookeeper/zookeeper.h>

/* Estrutura do servidor */
struct server_info
{
    int head;
    struct list_t *list;
    zhandle_t *zh;
    char *znode_path;
    struct rlist_t *next_server;
    pthread_mutex_t list_lock;
    char *local_ip_port; // ip:port do servidor
};

extern struct server_info server_info;

#endif