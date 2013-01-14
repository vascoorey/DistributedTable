#ifndef _LIST_H
#define _LIST_H

#include "data.h"
#include "entry.h"

struct list_t; /* A definir pelo grupo em list-private.h */

/*
 * Cria uma nova lista (estrutura list_t pode ser definida pelo grupo
 * no ficheiro list-private.h). Em caso de erro, retorna NULL
 */
struct list_t *list_create();

/*
 * Eliminar uma lista. Desaloca *toda* a memória utilizada pela lista.
 * Retorna 0 (OK) ou -1 (erro).
 */
void list_destroy(struct list_t *list);

/*
 * Adicionar um elemento (no final da lista). Retorna 0 (OK) ou -1 (erro).
 */
int list_add(struct list_t *list, struct entry_t *entry);

/*
 * Elimina da lista um elemento com a chave key (string). Retorna 0 (OK) ou
 * -1 (erro/não encontrado)
 */
int list_remove(struct list_t *list, char *key);

/*
 * Obtem um elemento da lista com a chave key (não uma cópia). Retorna a referência
 * do elemento na lista (ou seja, uma alteração implica alterar o elemento
 * na lista; a função list_remove ou list_destroy vai desalocar a memória
 * ocupada pelo elemento, ou NULL em caso de erro).
 */
struct entry_t *list_get(struct list_t *list, char *key);

/*
 * Retorna o tamanho (número de elementos) da lista (-1: erro).
 */
int list_size(struct list_t *list);

/*
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **list_get_keys(struct list_t *list);

/*
 * Desaloca a memória alocada por list_get_keys
 */
void list_free_keys(char **keys);

#endif
