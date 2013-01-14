#ifndef _NETWORK_CLIENT_H
#define _NETWORK_CLIENT_H

#include "remote_table.h"
#include "message.h"

#define RETRY_TIME 5 /* seconds */

/* 
 * Esta função deve:
 * - obter o endereço do servidor (struct sockaddr_in), a base da informação
 *   guardada na estrutura rtable
 * - estabelecer a ligação com o servidor
 * - guardar toda a informação necessária (e.g., descritor do socket) na
 *   estrutura rtable
 * - retornar 0 (OK) ou -1 (erro)
 */
int network_connect(struct rtable_t *rtable);

/* 
 * Esta função deve:
 * - obter o descritor da ligação (socket) da estrutura rtable
 * - enviar a mensagem msg ao servidor
 * - receber uma resposta do servidor
 * - retornar a mensagem obtida como resposta ou NULL em caso de erro
 */
struct message_t *network_send_receive(struct rtable_t *rtable, struct message_t *msg);

/* 
 * A função network_close deve fechar a ligação estabelecida por
 * network_connect(). Se network_connect() alocou memória, a função
 * network_close() deve libertar essa memória.
 */
int network_close(struct rtable_t *rtable);

#endif