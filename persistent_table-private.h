/* 
 * File:   persistent_table-private.h
 * 
 * Define a estrutura de uma tabela persistente.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#ifndef _PERSISTENT_TABLE_PRIVATE_H
#define	_PERSISTENT_TABLE_PRIVATE_H

#include "table.h"
#include "table-private.h"
#include "persistence_manager.h"
#include "persistence_manager-private.h"

/*
 * Define a estrutura de persistent_table.
 *
 * struct table_t *table => apontador para a tabela que vai manter todos os dados
 * struct pmanager_t *pmanager => apontador para o persistence manager
 */
struct ptable_t {
    struct table_t *table;
    struct pmanager_t *pmanager;
};

#endif

