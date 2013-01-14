/* 
 * File:   network_client-private.h
 *
 * Funções a serem usadas no módulo network_client.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#ifndef NETWORK_CLIENT_PRIVATE_H
#define	NETWORK_CLIENT_PRIVATE_H

/*
 * Esta função repete os mesmo passos de comunicação que a função
 * network_send_receive. Em caso de falha em algum destes passos retorna NULL.
 * Em caso de sucesso descodifica e retorna a mensagem.
 */
struct message_t *retry(int fd, char *buffer, int size);

#endif