#ifndef _PERSISTENT_TABLE_H
#define _PERSISTENT_TABLE_H

#include "data.h"
#include "table.h"
#include "persistence_manager.h"
#include "persistent_table-private.h"

/* 
 * Abre o acesso a uma tabela persistente, passando como parâmetro a tabela
 * a ser mantida em memória e o gestor de persistência a ser usado para manter
 * logs e checkpoints. Retorna a tabela persistente criada ou NULL em caso de
 * erro.
 */
struct ptable_t *ptable_open(struct table_t *table, struct pmanager_t *pmanager);

/* 
 * Fecha o acesso a esta tabela persistente. Todas as operações em table
 * devem falhar apos um close.
 */
void ptable_close(struct ptable_t *table);

/* 
 * Liberta toda memória e apaga todos os ficheiros utilizados pela tabela.
 */
void ptable_destroy(struct ptable_t *table);

/* 
 * Função para adicionar um elemento na tabela.
 * A função vai *COPIAR* a key (string) e os dados num espaco de memória
 * alocado por malloc().
 * Se a key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory).
 */
int ptable_put(struct ptable_t *table, char *key, struct data_t *data);

/* 
 * Função para obter um elemento da tabela.
 * O argumento key indica a key da entrada da tabela. A função
 * aloca memória para armazenar uma *CÓPIA* dos dados da tabela e
 * retorna este endereço. O programa a usar essa função deve
 * desalocar (utilizando free()) esta memória.
 * Em caso de erro, devolve NULL.
 */
struct data_t *ptable_get(struct ptable_t *table, char *key);

/* 
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação ptabel_put().
 * Devolve: 0 (ok), -1 (key not found).
 */
int ptable_del(struct ptable_t *table, char *key);

/* 
 * Devolve o número de elementos da tabela.
 */
int ptable_size(struct ptable_t *table);

/* 
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **ptable_get_keys(struct ptable_t *table);

/* 
 * Liberta a memória alocada por ptable_get_keys().
 */
void ptable_free_keys(char **keys);



/***************** Funções aplicadas a partir do projecto 4 *****************/

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave nao encontrada devolve 0.
 */
long ptable_get_ts(struct ptable_t *ptable, char *key);


#endif