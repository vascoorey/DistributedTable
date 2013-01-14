#ifndef _TABLE_H
#define _TABLE_H

#include "data.h"

struct table_t; /* A definir pelo grupo em table-private.h */

/*
 * Função para criar/inicializar uma nova tabela hash, com n linhas
 * (módulo da função HASH).
 */
struct table_t *table_create(int n);

/*
 * Eliminar/desalocar toda a memória.
 */
void table_destroy(struct table_t *table);

/*
 * Função para adicionar um elemento na tabela.
 * A função vai copiar a key (string) e os dados num espaço de memória
 * alocado por malloc().
 * Se a key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory)
 */
int table_put(struct table_t *table, char *key, struct data_t *data);

/*
 * Função para obter um elemento da tabela.
 * O argumento key indica a key da entrada da tabela. A função
 * aloca memória para armazenar uma *COPIA* dos dados da tabela e
 * retorna este endereço. O programa a usar essa função deve
 * desalocar (utilizando free()) esta memória.
 * Em caso de erro, devolve NULL.
 */
struct data_t *table_get(struct table_t *table, char *key);

/*
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação tabel_put().
 * Devolve: 0 (ok), -1 (key not found)
 */
int table_del(struct table_t *table, char *key);

/*
 * Devolve número de elementos da tabela.
 */
int table_size(struct table_t *table);

/*
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **table_get_keys(struct table_t *table);

/*
 * Desaloca a memória alocada por table_get_keys()
 */
void table_free_keys(char **keys);



/***************** Funções aplicadas a partir do projecto 3 *****************/

/*
 * Retorna o número de actualizações realizadas na tabela.
 */
int table_get_num_updates(struct table_t *table);

/*
 * Devolve um array de entry_t * com cópias de todas as entries da tabela,
 * e um último elemento a NULL.
 */
struct entry_t **table_get_entries(struct table_t *table);

/*
 * Liberta a memória alocada por table_get_entries().
 */
void table_free_entries(struct entry_t **entries);



/***************** Funções aplicadas a partir do projecto 4 *****************/

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave nao encontrada devolve 0.
 */
long table_get_ts(struct table_t *table, char *key);

#endif
