/*
 * File:   network-client.c
 *
 * Este ficheiro vai codificar a mensagem utilizando a função
 * string_to_buffer, enviá-la ao servidor e esperar pela resposta. Todos os
 * mecanismos de tolerância a falhas, são concretizados neste módulo.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 *
 * Created on 18 de Outubro de 2011, 19:56
 */

#include "utils.h"
#include "table.h"
#include "table-private.h"
#include "message.h"
#include "message-private.h"
#include "remote_table.h"
#include "remote_table-private.h"
#include "network_client.h"
#include "network_client-private.h"

/*
 * Esta função deve:
 * - obter o endereço do servidor (struct sockaddr_in), a base da informação
 *   guardada na estrutura rtable
 * - estabelecer a ligação com o servidor
 * - guardar toda a informação necessária (e.g., descritor do socket) na
 *   estrutura rtable
 * - retornar 0 (OK) ou -1 (erro)
 */
int network_connect(struct rtable_t *rtable) {
	
    //verifica se rtable aponta para NULL
    if(rtable == NULL) {
        ERROR("network_client: NULL rtable");
        return -1;
    }
	
    struct addrinfo hints, *serverInfo, *anAddress;
    int returnValue;
    	
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;            // IPv4 ou IPv6
    hints.ai_socktype = SOCK_STREAM;
	
    if((returnValue = getaddrinfo(rtable->ip, rtable->porto, &hints, &serverInfo)) != 0) {
        fprintf(stderr, "network_client: getaddrinfo > %s\n", gai_strerror(returnValue));
        return 1;
    }
	
    for(anAddress = serverInfo; anAddress; anAddress = anAddress->ai_next) {
        if((rtable->socket = socket(anAddress->ai_family,
                                    anAddress->ai_socktype,
                                    anAddress->ai_protocol)) == -1) {
            ERROR("network_client: socket");
            continue;
        }
		
        if(connect(rtable->socket, anAddress->ai_addr, anAddress->ai_addrlen) == -1) {
            close(rtable->socket);
            //ERROR("network_client: connect");
            continue;
        }
		
        break;  //conseguida a ligação sai do ciclo
    }
	
    //verifica se a ligação é válida e
    if(!anAddress) {
        //fprintf(stderr, "network_client: failed to connect\n");
		freeaddrinfo(serverInfo);
        return -1;
    }
    freeaddrinfo(serverInfo);

    //em caso de sucesso
    return 0;
	
}

/*
 * Esta função deve:
 * - obter o descritor da ligação (socket) da estrutura rtable
 * - enviar a mensagem msg ao servidor
 * - receber uma resposta do servidor
 * - retornar a mensagem obtida como resposta ou NULL em caso de erro
 */
struct message_t *network_send_receive(struct rtable_t *rtable, struct message_t *msg) {
	
    //verifica se remote ou msg apontam para NULL
    if(rtable == NULL || msg == NULL) {
        return NULL;
    }
	
    char *buffer;           //envia a mensagem inicial codificada
    int bufferSize;         //guarda o tamanho inicial do buffer
    char *bufferReceived = NULL;   //envia a mensagem codificada
    int bufferReceivedSize; //guarda o tamanho do buffer
    uint32_t convertedSize; //guarda o tamanho do buffer convertido
    struct message_t *rsp;  //a mensagem com a resposta do servidor
	
    //codifica a mensagem e verifica se a operação foi bem sucedida
    if((bufferSize = message_to_string(msg, &buffer)) <= 0) {
        ERROR("network_client: message_tostring");
        return NULL;
    }
	
	//printf("enviar: %s\n", buffer);
	
    //converte o tamanho do buffer para ser enviado
    convertedSize = htonl(bufferSize);
    bool falhou = false;    //regista uma falha de envio
	
    //envia o tamanho do buffer a ser enviado
    if((write(rtable->socket, &convertedSize, sizeof(uint32_t))) != sizeof(uint32_t)) {
        ERROR("network_client: send bufferSize > retry...");
        falhou = true;
    }
    else {
        //envia o buffer
        if((write(rtable->socket, buffer, bufferSize)) != bufferSize) {
            ERROR("network_client: send buffer > retry...");
            falhou = true;
        }
        else {
            //caso não tenha falhado prepara a recepção
            if(read(rtable->socket, &convertedSize, sizeof(uint32_t)) != sizeof(uint32_t)) {
                ERROR("network_client: recv buffer > retry...");
                falhou = true;
            }
            else {
                bufferReceivedSize = ntohl(convertedSize);
				
                //aloca memória para o buffer a ser recebido
                if((bufferReceived = (char *) malloc(bufferReceivedSize + 1)) == NULL) {
                    free(buffer);
                    ERROR("network_client: NULL bufferReceived");
                    return NULL;
                }
			
                //recebe o buffer
                if(read(rtable->socket, bufferReceived, bufferReceivedSize) != bufferReceivedSize) {
                    free(buffer);
                    free(bufferReceived);
                    perror("network_client: recv bufferReceived > retry...");
                    falhou = true;
                }
                bufferReceived[bufferReceivedSize] = '\0';
            }
        }
    }
	
    //caso o envio ou recpção da mensagem tenha falhado
    if(falhou) {
        sleep(RETRY_TIME);
        rsp = retry(rtable->socket, buffer, bufferSize);
    }
    else {
        //descodifica a mensagem e verifica-a
        if((rsp = string_to_message(bufferReceived)) == NULL) {
            ERROR("network_client: string_to_message");
        }
    }
	
    //liberta memória e retorna a resposta
	if(buffer) {
		free(buffer);
	}
	if(bufferReceived) {
		free(bufferReceived);
	}
    return rsp;
	
}

/*
 * Esta função repete os mesmo passos de comunicação que a função
 * network_send_receive. Em caso de falha em algum destes passos retorna NULL.
 * Em caso de sucesso descodifica e retorna a mensagem.
 */
struct message_t *retry(int fd, char *buffer, int size) {
	
    int bufferSize = size;  //guarda o tamanho do buffer
    uint32_t convertedSize; //guarda o tamanho do buffer convertido para
    struct message_t *rsp;  //a mensagem com a resposta do servidor
	char *bufferReceived;
    
    //converte o tamanho do buffer para ser enviado
    convertedSize = htonl(bufferSize);
	
    //envia o tamanho do buffer a ser enviado
    if(write(fd, &convertedSize, sizeof(uint32_t)) != sizeof(uint32_t)) {
        ERROR("network_client: resend bufferSize");
        return NULL;
    }
	
    //envia o buffer
    if(write(fd, buffer, bufferSize) != bufferSize) {
        ERROR("network_client: resend buffer");
        return NULL;
    }
	
    //caso não tenha falhado prepara a recepção
    if(read(fd, &convertedSize, sizeof(uint32_t)) != sizeof(uint32_t)) {
        ERROR("network_client: rerecv buffer");
        return NULL;
    }
	
    bufferSize = ntohl(convertedSize);
	
    //aloca memória para o buffer a ser recebido
    if((bufferReceived = (char *) malloc(bufferSize)) == NULL) {
        ERROR("network_client: NULL buffer");
        return NULL;
    }
	
    //recebe o buffer
    if(read(fd, bufferReceived, bufferSize) != bufferSize) {
        ERROR("network_client: rerecv buffer");
        return NULL;
    }
	
    //descodifica a mensagem e verifica-a
    if((rsp = string_to_message(bufferReceived)) == NULL) {
        ERROR("network_client: string_to_message");
        return NULL;
    }
	
	free(bufferReceived);
    return rsp;
	
}

/*
 * A função network_close deve fechar a ligação estabelecida por
 * network_connect(). Se network_connect() alocou memória, a função
 * network_close() deve libertar essa memória.
 */
int network_close(struct rtable_t *rtable) {
	
    //verifica se remote aponta NULL
    if(rtable == NULL) {
        return -1;
    }

    //verifica se a ligação é fechada sem erros
    if((close(rtable->socket)) == -1) {
        ERROR("network_client: close");
        return -1;
    }
    
    //em caso de sucesso
    return 0;
	
}
