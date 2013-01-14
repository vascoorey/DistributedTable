#ifndef _PERSISTENCE_MANAGER_H
#define _PERSISTENCE_MANAGER_H

#include "table.h"

#define MODE_ASYNC 0 /* Escritas assincronas no log */
#define MODE_FSYNC 1 /* Escritas sincronas no log com integridade de ficheiros */
#define MODE_DSYNC 2 /* Escritas sincronas no log com integridade de dados */

struct pmanager_t; /* A definir pelo grupo em persistence_manager-private.h */

/* 
 * Cria um gestor de persist�ncia que armazena logs em filename+".log" e o
 * estado do sistema em filename+".ckp". O par�metro logsize define o tamanho
 * m�ximo em bytes que o ficheiro de log pode ter enquanto mode define o modo
 * de escrita dos logs (MODE_ASYNC, MODE_FSYNC e MODE_DSYNC).
 * Retorna o pmanager criado ou NULL em caso de erro.
 */
struct pmanager_t *pmanager_create(char *filename, int logsize, int mode);

/* 
 * Destroi este gestor de persist�ncia. Retorna 0 se tudo estiver OK ou -1
 * em caso de erro.
 * Note que esta fun��o n�o limpa os ficheiros de log e ckp do sistema.
 */
int pmanager_destroy(struct pmanager_t *pmanager);

/* 
 * Apaga os ficheiros de log e ckp geridos por este gestor de persist�ncia
 * e o destroi. Retorna 0 se tudo estiver OK ou -1 em caso de erro.
 */
int pmanager_destroy_clear(struct pmanager_t *pmanager);

/* 
 * Retorna 1 caso existam dados nos ficheiros de log e/ou checkpoint e 0
 * caso contr�rio.
 */
int pmanager_have_data(struct pmanager_t *pmanager);

/* 
 * Escreve uma string msg no fim do ficheiro de log associado a pmanager.
 * Retorna o n�mero de bytes escritos no log ou -1 em caso de problemas na
 * escrita (e.g., erro no write()) ou caso o tamanho do ficheiro de log ap�s
 * o armazenamento da mensagem seja maior que logsize (neste caso msg n�o �
 * escrita no log).
 */
int pmanager_log(struct pmanager_t *pmanager, char *msg);

/* 
 * Cria um ficheiro filename+".stt" com o estado de table. Retorna
 * o tamanho do ficheiro criado ou -1 em caso de erro.
 */
int pmanager_store_table(struct pmanager_t *pmanager, struct table_t *table);

/* 
 * Limpa o ficheiro ".log" e copia o ficheiro ".stt" para ".ckp". Retorna 0
 * se tudo correr bem ou -1 em caso de erro.
 */
int pmanager_rotate_log(struct pmanager_t *pmanager);

/* 
 * Mete o estado contido nos ficheiros .log, .stt ou .ckp na tabela passada
 * como argumento.
 */
int pmanager_fill_state(struct pmanager_t *pmanager, struct table_t *table);

#endif