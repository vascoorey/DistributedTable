#ifndef _REMOTE_TABLE_H
#define _REMOTE_TABLE_H

#define OP_RT_PUT	10
#define OP_RT_GET	20
#define	OP_RT_DEL	30
#define OP_RT_SIZE	40
#define OP_RT_GETKEYS	50
#define OP_RT_GETTS     60
/* opcode da resposta a um pedido e igual a op+1 */

#define OP_RT_ERROR     99


#include "data.h"
#include "entry.h"

struct rtable_t; /* A definir pelo grupo em rtable-private.h */

/*
 * Função para estabelecer uma associação com uma tabela num servidor.
 * address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
int rtable_connect(struct rtable_t *table);
struct rtable_t *rtable_open(const char *address_port);

/* 
 * Fecha a ligação com o servidor, desaloca toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *table);

/* 
 * Função para adicionar um elemento na tabela.
 * Se key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory).
 */
int rtable_put(struct rtable_t *table, struct entry_t *entry);

/* 
 * Função para obter um elemento da tabela.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtable_get(struct rtable_t *table, char *key);

/* 
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação rtable_put()
 * Devolve: 0 (ok), -1 (key not found).
 */
int rtable_del(struct rtable_t *table, char *key);

/* 
 * Devolve o número de elementos da tabela.
 */
int rtable_size(struct rtable_t *table);

/* 
 * Devolve um array de char* com a cópia de todas as keys da tabela,
 * e um último elemento NULL.
 */
char **rtable_get_keys(struct rtable_t *table);



/***************** Funções aplicadas a partir do projecto 4 *****************/

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave não encontrada devolve 0.
 */
long rtable_get_ts(struct rtable_t *table, char *key);

#endif
