/* 
 * File:   quorum_table-private.h
 *
 * Define a estrutura de uma quorum table.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#ifndef _QUORUM_TABLE_PRIVATE_H
#define	_QUORUM_TABLE_PRIVATE_H

#include "remote_table.h"
#include "remote_table-private.h"
#include "quorum_access.h"
#include "quorum_access-private.h"

/*
 * Define a estrutura de uma quorum table.
 *
 * int id => identifica o processo cliente
 * int numServers => o numero servidores disponíveis
 * char **serversAddress => os ip's e portos dos servidores disponíveis
 * struct rtable_t **servers => a ligação às tabelas remotas de cada servidor
 */
struct qtable_t {
    int id;
    int numServers;
    char **serversAdrress;
    struct rtable_t **servers;
};

void qtable_free_quorum_op_t(struct quorum_op_t **ret, int n);

long update_timestamp(long ts, int id);

#endif