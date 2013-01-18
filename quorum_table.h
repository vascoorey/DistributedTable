#ifndef _QUORUM_TABLE_H
#define _QUORUM_TABLE_H

#include "data.h"

struct qtable_t; /* A definir pelo grupo em quorum_table-private.h */

/*
 * Função para estabelecer uma associação com uma tabela e um array de n
 * servidores. addresses_ports é um array de strings e n é o tamanho
 * deste array.
 * Retorna NULL caso não consiga criar o qtable_t.
 */
struct qtable_t *qtable_connect(const char **addresses_ports, int n, int id);

/*
 * Fecha a ligação com os servidores do sistema e liberta a memória alocada
 * para qtable.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int qtable_disconnect(struct qtable_t *qtable);

/*
 * Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Note que o timestamp em data será atribuído internamente a esta função,
 * como definido no protocolo de escrita.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_put(struct qtable_t *qtable, char *key, struct data_t *data);

/*
 * Função para obter um elemento da tabela.
 * Em caso de erro ou elemento não existente, devolve NULL.
 */
struct data_t *qtable_get(struct qtable_t *qtable, char *key);

/*
 * Função para remover um elemento da tabela. É equivalente a execução
 * put(k,NULL) se a chave existir. Se a chave não existir, nada acontece.
 * Devolve 0 (ok), -1 (chave não encontrada).
 */
int qtable_del(struct qtable_t *qtable, char *key);

/* Devolve numero (aproximado) de elementos da tabela ou -1 em caso de
 * erro.
 */
int qtable_size(struct qtable_t *qtable);

/* Devolve um array de char* com a copia de todas as keys da tabela,
 * e um ultimo elemento a NULL. Esta funcao nao deve retornar as
 * chaves removidas, i.e., a NULL.
 */
char **qtable_get_keys(struct qtable_t *rtable);

/* Desaloca a memoria alocada por qtable_get_keys().
 */
void qtable_free_keys(char **keys);

#endif

