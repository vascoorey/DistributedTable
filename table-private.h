/*
 * File:   table-private.h
 *
 * Define a estrutura de uma tabela hash.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H

#include "list-private.h"

/*
 * Define a estrutura de uma tabela hash.
 *
 * int hashSize => o tamanho da tabela
 * int numElems => o número de elementos da tablela
 * int numUpdates => o numero de alterações à tabela.
 * struct list_t **table => apontador para a tabela hash, que por sua vez
 *                          tem apontadores para as várias listas que guardam
 *                          as entradas
 */
struct table_t {
	int hashSize;
	int numElems;
	int numUpdates;
	struct list_t **table;	
};

/*
 * A função hash.
 */
int hash(char *string, int mod);

#endif
