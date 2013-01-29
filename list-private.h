/*
 * File:   list-private.h
 *
 * Define a estrutura de uma lista duplamente ligada e dos nós a usar na
 * mesma.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#ifndef _LISTA_PRIVATE_H
#define _LISTA_PRIVATE_H

#include "utils.h"
#include "data.h"
#include "entry.h"

/*
 * Define a estrutura de um nó a usar na lista.
 *
 * struct node_t *prev => apontador para o nó anterior
 * struct node_t *next => apontador para o nó seguinte
 * struct entry_t *entry => apontador para conteúdo do nó
 */
struct node_t {
    struct node_t *prev;
    struct node_t *next;
    struct entry_t *entry;
};

/*
 * Define a estrutura da lista.
 *
 * struct node_t *head => apontador para o nó da cabeça da lista
 * struct node_t *tail => apontador para o nó da cauda da lista
 * int elements => número de elementos da lista
 */
struct list_t {
    struct node_t *head;
    struct node_t *tail;
    int elements;
};

/*
 * Cria e devolve um novo nó e mantem a referência da entry passada.
 * Devolve NULL em caso de erro.
 */
struct node_t *node_create(struct entry_t *entry);

/*
 * Destroí o nó passado e desaloca toda a sua memória.
 */
void node_destroy(struct node_t *node);

/*
 * Retorna todas as entries dessa lista.
 */
struct entry_t **list_get_entries(struct list_t *list);

#endif