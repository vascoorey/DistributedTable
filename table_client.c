/*
 * File:   table_client.c
 *
 * Ficheiro responsável pela criação de uma aplicação interactiva, que aceita
 * vários comandos do utilizador e invoca a respectiva função do stub, imprime a
 * resposta no ecrã, e volta a aceitar o próximo comando.
 * Cada comando vai ser inserido pelo utilizador numa única linha, havendo as
 * seguintes alternativas:
 *      put <key> <data>
 *      get <key>
 *      del <key>
 *      size
 *      getkeys
 *      quit
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "utils.h"
#include "remote_table.h"
#include "remote_table-private.h"
#include "network_client.h"
#include "network_client-private.h"
#include "message-private.h"
#include "quorum_table.h"


/*
 * MAX_LINE_SIZE: Número máximo de caracteres que uma linha de comando pode ter.
 */
#define MAX_LINE_SIZE 1024

struct qtable_t *quorumTable = NULL;

int executeComand(struct qtable_t *remoteTable);

/*void pipe_handler(sig_t sig) {
	printf("> Ligação perdida.\n");
	while (network_connect(remoteTable) == -1) {
		printf("> Tentando de novo em %d segundos...\n", RETRY_TIME);
		sleep(RETRY_TIME);
	}
	printf("> Ligação re-estabelecida.\n");
}*/

/*
 * Recebe como argumento o endereço e porto do servidor com o seguinte formato:
 * <IP/nome_da_máquina>:<porto>. Inicia a ligação do cliente ao servidor com
 * base nesta informação.
 * Retorna 0 (OK), -1 (em caso de erro).
 */
int main(int argc, char** argv) {

    //para permitir a detecção de uma eventual falha do servidor
    //signal(SIGPIPE, (__sighandler_t)pipe_handler);

    //verifica se é passado o número certo de argumentos(dois)
    if(argc < 3) {
        printf("ERRO: Parâmetros inválidos.\n");
        return -1;
    }
	
	//servers = (char**)malloc(sizeof(char*) * numServers);
	//assert(servers);
	
	//for(i = 0; i < numServers; i++) {
	//	assert((servers[i] = strdup(argv[i + 2])));
	//}

    printf("************************* table-client *************************\n");

    //tenta iniciar uma ligação ao servidor
	
    if((quorumTable = qtable_connect((const char**)(argv + 2), argc - 2, atoi(argv[1]))) == NULL) {
        printf("* Ligação não estabelecida. Hosts inválidos ou indisponíveis.\n"
                "**************************************************************"
                "**\n");
        return -1;
    }

    //printf("* IP: %s\n", remoteTable->ip);
	//printf("* Porto: %s\n", remoteTable->porto);

    printf("* Ligação estabelecida. Insira um dos seguintes comandos:"
            "\n*\t- put   <key> <data>\n*\t- get   <key>"
            "\n*\t- del   <key>\n*\t- size"
            "\n*\t- getkeys\n*\t- quit\n********************"
            "********************************************\n");

    //verifica os vários comandos inseridos
    executeComand(quorumTable);

    //em caso de sucessso
    return (0);

}

/*
 * Verifica a recepção dos comandos por stdin, confirma a sua validade e
 * executa-os.
 * Retorna 0 (OK), -1 (em caso de erro).
 */
int executeComand(struct qtable_t *remoteTable) {
	struct timeval start, end;
	long usecDiff, secDiff;
    //aloca memória para receber os comandos
    char *comandLine;
    if((comandLine = (char *) malloc(sizeof(char) * MAX_LINE_SIZE)) == NULL) {
        perror("table-client: malloc comandLine");
        return -1;
    }

    //recebe a primeira sequência de caracteres
    char *comand;

    //inicia um ciclo infinito
    while(1) {

        printf("> ");

        //lê o comando do teclado
        fgets(comandLine, MAX_LINE_SIZE, stdin);

        //apanha a primeira sequência de caracteres...
        comand = strtok(comandLine, " \n");

        //... verifica o tamanho do comando...
        int comandSize = strlen(comand);
        if(comand != NULL && comandSize >= 3 && comandSize <= 7) {
            int i;
            //... e converte-a para lowercase
            for(i = 0; i < comandSize; i++) {
                comand[i] = tolower(comand[i]);
            }
        }
        else{
            printf("> ERRO: Comando Inválido.\n");
            continue;
        }

        //put <key> <data>
        if(strcmp(comand, "put") == 0) {

            //guarda a key
            char *key;
            if((key = strtok(NULL, " \n")) == NULL) {
                printf("> ERRO: Comando Inválido.\n");
                continue;
            }
            
            //guarda o valor de data
            char *dataValue;
            if((dataValue = strtok(NULL, "\n")) == NULL) {
                printf("> ERRO: Comando Inválido.\n");
                continue;
            }
            
            //guarda o tamanho de data
            int dataSize = strlen(dataValue) + 1;
            
            //cria a data
            struct data_t *data;
            if((data = data_create2(dataSize, strdup(dataValue))) == NULL) {
                perror("remote_table: data_create2 data");
                return -1;
            }
			data->timestamp = 0;
			if(PRINT_LATENCIES) {
				gettimeofday(&start, NULL);
			}
            //insere a data com a respectiva chave
            if(qtable_put(remoteTable, key, data) == 0) {
                printf("> Entrada inserida\n");
            }
            else {
				if(data) {
					data_destroy(data);
				}
                printf("> Erro ao inserir entrada\n");
            }
			if(PRINT_LATENCIES) {
				gettimeofday(&end, NULL);
				
				secDiff = end.tv_sec - start.tv_sec;
				if(secDiff > 0) {
					// Como passou um segundo entretanto n podemos fazer somente end - start
					// A diff do start = 1000000 - start.tv_usec
					// A diff do end = end.tv_usec
					// total diff = end.tv_usec + (1000000 - start.tv_usec)
					usecDiff = end.tv_usec + (1000000 - start.tv_usec);
				} else {
					usecDiff = end.tv_usec - start.tv_usec;
				}
				
				//printf("(╯°□°）╯︵ ┻━┻\n");
				printf("*** Time elapsed: %ld us.\n", usecDiff);
			}
			data_destroy(data);

            //em caso de sucesso
            continue;

        }

        //get <key>
        if(strcmp(comand, "get") == 0) {

            //guarda a key
            char *key;
            if((key = strtok(NULL, " \n")) == NULL) {
                printf("> ERRO: Comando Inválido.\n");
                continue;
            }

            //recupera a entrada correspondente à chave
            struct data_t *data;
            if((data = qtable_get(remoteTable, key)) != NULL) {
                printf("> Entrada recuperada\n"
                        ">   - Chave: %s\n>   - Entrada: %s\n",
                        key, (char *) data->data);
            }
            else {
                printf("> Entrada não existente na tabela.\n");
            }

            //em caso de sucesso
			if(data) {
				data_destroy(data);
			}
            continue;

        }

        //del <key>
        if(strcmp(comand, "del") == 0) {

            //guarda a key
            char *key;
            if((key = strtok(NULL, " \n")) == NULL) {
                printf("> ERRO: Comando Inválido.\n");
                continue;
            }

            //elimina a entrada correspondente à chave
            if((qtable_del(remoteTable, key)) == 0) {
                printf("> Entrada eliminada\n");
            }
            else {
                printf("> Erro ao eliminar a entrada\n");
            }

            //em caso de sucesso
            continue;

        }

        //size
        if(strcmp(comand, "size") == 0) {

            //pesquisa pelo número de entradas da tabela
            int size = 0;
            if((size = qtable_size(remoteTable)) >= 0) {
                printf("> Total de entradas: %d\n", size);
            }
            else {
                printf("> Erro ao pesquisar pelo tamanho da tabela.\n");
            }

            //em caso de sucesso
            continue;
            
        }

        //getkeys
        if(strcmp(comand, "getkeys") == 0) {

            //pesquisa pelas chaves da tabela
            char **keys;
            if((keys = qtable_get_keys(remoteTable)) == NULL) {
                printf("> Erro ao pesquisar pelas chaves da tabela.\n");
            }
            else {
                int i = 0;
                if(keys && keys[i]) {
					printf("> Chaves da tabela:\n");
                    while (keys[i] != NULL) {
                        printf(">   - %s\n", keys[i]);
                        free(keys[i]);
                        i++;
                    }
                }
                else {
                    printf("> Tabela sem entradas\n");
                }
            }
            free(keys);
            continue;
        }

        //quit
        if(strcmp(comand, "quit") == 0) {
            qtable_disconnect(remoteTable);
            free(comandLine);
            return 0;
        }
        printf("> ERRO: Comando inválido.\n");
    }

}