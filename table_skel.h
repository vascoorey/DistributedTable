#ifndef _TABLE_SKEL_H
#define _TABLE_SKEL_H

#include "message.h"

/*
 * Inicia o skeleton da tabela.
 * O main() do servidor deve chamar este método antes de usar a
 * funço invoke(). O parâmetro n_lists define o número de listas a serem
 * usadas pela tabela mantida no servidor. O parâmetro filename corresponde ao
 * nome a ser usado nos ficheiros de log e checkpoint.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY).
 */
int table_skel_init(int n_lists, char *filename);

/*
 * Serve para libertar toda a memória alocada pela função anterior.
 */
int table_skel_destroy();

/*
 * Executar uma função (indicada pelo opcode na msg) e retorna o resultado na
 * própria struct msg.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao inicializada).
 */
int invoke(struct message_t *msg);

#endif