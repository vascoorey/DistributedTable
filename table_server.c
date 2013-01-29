/*
 * File:   table_server.c
 *
 * Inicia o servidor num porto e com um número de listas definido
 * pelo utilizador.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "table_skel.h"
#include "utils.h"
#include "message.h"
#include "remote_table.h"

#define GARBAGE_COLLECTION_TIME 300

int shutdownServer = 1;
int relogioLogico = 0;

void signalHandler(sig_t sig);
int server_send_receive(struct pollfd connection);

void signalHandler(sig_t sig) {
	signal(SIGINT, (__sighandler_t)signalHandler);
	if(shutdownServer) {
		printf("\nSinal apanhado, iremos desligar o servidor assim que possivel!\n");
		shutdownServer = 0;
	} else {
		printf("OK you're the boss... But the leaks are your fault.\n");
		exit(EXIT_SUCCESS);
	}
}

void *getInAddress(struct sockaddr *address) {
	if(address->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)address)->sin_addr);
	} else {
		return &(((struct sockaddr_in6*)address)->sin6_addr);
	}
}

int main(int argc, char **argv) {
	int socketFileDescriptor, newSocket = -1, numBytes, yes = 1, retVal, pollCounter, temp;
	struct addrinfo hints, *serverInfo, *anAddress;
	struct sockaddr_storage connectorAddress;
	socklen_t socketSize;
	struct sigaction signalAction;
	struct pollfd ufd[10]; //
	char addressDescription[INET6_ADDRSTRLEN], *buffer = NULL;
	uint32_t bufferLength;
	struct message_t *message = NULL;
	
	if(argc != 4) {
		fprintf(stderr, "usage: server <port> <num lists> <filename>\n");
		exit(-1);
	}
	
	// Aqui montamos o tratamento de sinais
	signal(SIGINT, (__sighandler_t)signalHandler);
	signal(SIGPIPE, SIG_IGN);
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // Use current IP
	
	if((retVal = getaddrinfo(NULL, argv[1], &hints, &serverInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
		return -1;
	}
	
	for(anAddress = serverInfo; anAddress; anAddress = anAddress->ai_next) {
		if((socketFileDescriptor = socket(anAddress->ai_family, anAddress->ai_socktype, anAddress->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		
		if(setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		
		if(bind(socketFileDescriptor, anAddress->ai_addr, anAddress->ai_addrlen) == -1) {
			close(socketFileDescriptor);
			perror("server: bind");
			continue;
		}
		
		// Se chegamos aqui conseguimos fazer um bind com sucesso.
		break;
	}
	
	// Vamos testar a ver se conseguimos bind com sucesso...
	if(!anAddress) {
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}
	
	freeaddrinfo(anAddress); // Ja nao precisamos
	
	// Por o socket a non-blocking
	// Precisamos de error checking aqui ?
	fcntl(socketFileDescriptor, F_SETFL, O_NONBLOCK);
	
	memset(ufd, 0, sizeof(ufd));
	
	ufd[0].fd = socketFileDescriptor;
	ufd[0].events = POLLIN; //| POLLOUT;
	for(pollCounter = 1; pollCounter < 10; pollCounter++)
		ufd[pollCounter].fd = -1;
	
	// Apenas podem ficar 10 em fila ?
	if(listen(ufd[0].fd, 10) == -1) {
		perror("listen");
		close(socketFileDescriptor);
		exit(-1);
	}
	
	printf("server: waiting..\n");
	
	// Inicializar a tabela.
	if(table_skel_init(atoi(argv[2]), argv[3]) == -1) {
		perror("table_skel_init");
		exit(-1);
	}

	time_t lastTime = time(NULL), currentTime;
	
	// O 0 no poll é o timeout, queremos ficar escutando.
	while((retVal = poll(ufd, 10, 0)) != -1 && shutdownServer) {
		// Poll!
		if(ufd[0].revents & POLLIN) {
			// Temos dados para ler
			printf("Nova ligação detectada!\n");
			socketSize = sizeof(connectorAddress);
			if((newSocket = accept(socketFileDescriptor, (struct sockaddr*)&connectorAddress, &socketSize)) == -1) {
				perror("accept");
				continue;
			} else {
				// Adiciona o cliente a ufd
				for(pollCounter = 1; ufd[pollCounter].fd != -1; pollCounter++);
				ufd[pollCounter].fd = newSocket;
				ufd[pollCounter].events = POLLIN;
				printf("A ligação foi estabelecida com sucesso. (%d)\n", pollCounter);
			}
		}
		// Agora tratamos de ufd[1].
		for(pollCounter = 1; pollCounter < 10; pollCounter ++) {
			if(ufd[pollCounter].revents & POLLHUP) {
				printf("A fechar ligação %d\n", pollCounter);
				close(ufd[pollCounter].fd);
				ufd[pollCounter].fd = -1;
				ufd[pollCounter].revents = -1;
				ufd[pollCounter].events = -1;
			}
			else if(ufd[pollCounter].fd > 0 && ufd[pollCounter].revents & POLLIN) {
				if(server_send_receive(ufd[pollCounter]) == -1) {
					printf("servidor: ligacao perdida, a fechar: %d...\n", ufd[pollCounter].fd);
					close(ufd[pollCounter].fd);
					ufd[pollCounter].fd = -1;
					ufd[pollCounter].revents = -1;
					ufd[pollCounter].events = -1;
				}
			}
		}
		currentTime = time(NULL);
		if((currentTime - lastTime) > GARBAGE_COLLECTION_TIME) {
			printf("*** Collecting Garbage ***\n");
			table_skel_collect();
			lastTime = currentTime;
		}
	}

	// Iremos encerrar o servidor ! Fecha tudo no ufd e libera tudo o que foi alocado.
	for(pollCounter = 0; pollCounter < 10; pollCounter ++) {
		if(ufd[pollCounter].fd > 0) {
			close(ufd[pollCounter].fd);
		}
	}
	// Fechar a table_skel
	if(table_skel_destroy() == -1) {
		perror("table_skel_destroy");
	}
	if(buffer) {
		printf("Libertando buffer...\n");
		free(buffer);
		buffer = NULL;
	}
	if(message) {
		printf("Libertando mensagem...\n");
		free_message(message);
	}	
	if(socketFileDescriptor) {
		close(socketFileDescriptor);
	}
	if(newSocket > 0) {
		close(newSocket);
	}
	
	return 0;
}

int server_send_receive(struct pollfd connection) {
	int numBytes, temp, retVal = -1, bufferSize;
	uint32_t bufferLength;
	char *buffer;
	struct message_t *message;
	
	// Primeiro lemos o tamanho da mensagem
	if((numBytes = (int)read(connection.fd, &bufferLength, sizeof(uint32_t))) > 0) {
		// Recebemos o tamanho do buffer que o cliente vai enviar
		temp = (int)ntohl(bufferLength);
		//printf("Tamanho recebido: %d -> %d\n", bufferLength, temp);
		if(!(buffer = (char*)malloc(temp + 1))) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		if((numBytes = (int)read(connection.fd, buffer, temp)) < temp) {
			// Perdemos a ligação a meio do envio... Retry aqui
			perror("read buffer");
			free(buffer);
			//printf("buffer lido < numBytes... Ignorando...\n");
		} else {
			buffer[numBytes] = '\0';
			if(!(message = string_to_message(buffer))) {
				perror("string_to_message");
				// Recebemos uma mensagem invalida, mandamos de volta uma mensagem de erro.
				printf("servidor recebeu uma mensagem invalida...\n");
				printf("criando mensagem de erro para enviar de volta...\n");
				// Criar mensagem de erro.
				if(!(message = (struct message_t*)malloc(sizeof(struct message_t)))) {
					perror("malloc message!");
					exit(EXIT_FAILURE);
				}
				message->opcode = OP_RT_ERROR;
				message->c_type = CT_RESULT;
				message->content.result = -1;
				free(buffer);
				if((numBytes = message_to_string(message, &buffer)) <= 0) {
					perror("message_to_string...\n");
					exit(EXIT_FAILURE);
				}
			} else {
				printf("Mensagem %d: %hd %hd\n", relogioLogico, message->opcode, message->c_type);
				relogioLogico ++;
				// Mensagem é valida, invoke
				invoke(message);
				free(buffer);
				if((numBytes = message_to_string(message, &buffer)) <= 0) {
					perror("message_to_string...\n");
					exit(EXIT_FAILURE);
				}
				free_message(message);
			}
			bufferLength = htonl(numBytes);
			if(write(connection.fd, &bufferLength, sizeof(bufferLength)) == -1) {
				perror("write bufferLength");
			} else {
				if(write(connection.fd, buffer, numBytes) == -1) {
					perror("write buffer");
				} else {
					//printf("Resposta: %s ... ", buffer);
					printf("Resposta enviada com sucesso.\n");
				}
				free(buffer);
			}
			//free(buffer);
			retVal = 0;
		}
	}
	return retVal;
}
