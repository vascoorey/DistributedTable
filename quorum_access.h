#ifndef _QUORUM_ACCESS_H
#define _QUORUM_ACCESS_H

#include "remote_table.h"

/* Operacao a ser executada pelas threads atraves da rtable. */
struct quorum_op_t {
	int id; /* id unico da operacao (mesmo no pedido e na resposta) */
	int sender; /* emissor da operacao, ou da resposa */
	int opcode; /* mesmo usado em message_t.opcode */
	union content_u {
		struct entry_t *entry;
		long timestamp;
		char *key;
		char **keys;
		struct data_t *value;
		int result;
	} content;
	//union content_u content;
};

/* Essa funcao deve criar as threads e as filas de comunicacao para que o
 * cliente invoque operações em um conjunto de tabelas em servidores
 * remotos. Recebe como parametro um array de rtable de tamanho n.
 * Retorna 0 (OK) ou -1 (erro). */
int init_quorum_access(struct rtable_t **rtable, int n);

/* Funcao que envia uma requisicao a um conjunto de servidores e devolve
 * um array com o numero esperado de respostas.
 * O parametro request e uma representacao do pedido enquanto
 * expected_replies <= n representa a quantidade de respostas a ser 
 * esperadas antes da funcao retornar.
 * Note que os campos id e sender da request serão preenchidos dentro da
 * funcao. O array retornado é um array com n posicoes (0 … n-1) sendo a
 * posicao i correspondente a um apontador para a resposta do servidor 
 * i, ou NULL caso a resposta desse servidor não tenha sido recebida.
 * Este array deve ter pelo menos expected_replies posicoes != NULL.
 */
struct quorum_op_t **quorum_access(struct quorum_op_t *request, 
                                   int expected_replies);

/* Liberta a memoria, destroi as rtables usadas e destroi as threads.
 */
int destroy_quorum_access();

#endif

