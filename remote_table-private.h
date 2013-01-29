/*
 * File:   remote_table-private.h
 *
 * Define a estrutura de uma tabela remota.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#ifndef _REMOTE_TABLE_PRIVATE_H
#define _REMOTE_TABLE_PRIVATE_H


/*
 * Define a estrutura de uma tabela remota.
 *
 * int socket => o descritor do socket
 * char *ip => apontador para o ip (string)
 * char *porto => apontador para o porto (string)
 */
struct rtable_t {
	int socket;
	char *ip;
	char *porto;
};

#endif