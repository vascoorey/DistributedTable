/*
 * File:   remote_table.c
 *
 * Este ficheiro concretiza uma função por cada comando do cliente, além de
 * funções para conectar e desconectar a um servidor.
 * Cada função vai preencher uma estrutura message_t com o opcode (tipo da
 * operação) e os campos usados de acordo com o conteúdo requerido pela mensagem
 * e vai passar esta estrutura para o módulo de comunicação, recebendo uma
 * resposta deste.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "data.h"
#include "entry.h"
#include "message.h"
#include "message-private.h"
#include "remote_table.h"
#include "remote_table-private.h"
#include "network_client.h"
#include "network_client-private.h"
#include "table.h"

/*
 * Função para estabelecer uma associação com uma tabela num servidor.
 * address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtable_t *rtable_open(const char *address_port) {

    //verifica se address_port aponta para NULL
    if(address_port == NULL) {
        ERROR("remote_table: NULL address_port");
        return NULL;
    }

    char *host;                     //IP ou nome da máquina
    char *port;                     //porto
    char *endereco;                 //uma cópia do endereço
    struct rtable_t * remoteTable;  //a tabela remota

    //copia o endereço para uma variável temporária
    if((endereco = strdup(address_port)) == NULL) {
        ERROR("remote_table: strdup address_port");
        return NULL;
    }

    //guarda o endereço da máquina
    if((host = strdup((char *) strtok(endereco, ":"))) == NULL) {
        ERROR("remote_table: strdup host");
        free(endereco);
        return NULL;
    }

    //guarda o porto da máquina
    char *tempString;
    if((tempString = strtok(NULL, ":")) == NULL) {
        ERROR("remote_table: strtok port");
        return NULL;
    }

    if((port = strdup(tempString)) == NULL) {
        ERROR("remote_table: strdup port");
        free(host);
        free(endereco);
        return NULL;
    }

    //aloca memória para a tabela remota
    if((remoteTable = (struct rtable_t *) malloc(sizeof(struct rtable_t))) == NULL) {
        ERROR("remote_table: malloc remoteTable");
        free(host);
        free(port);
        free(endereco);
        return NULL;
    }

    //guarda o ip e o porto
    remoteTable->ip = host;
    remoteTable->porto = port;

    free(endereco);

    //em caso de sucesso
    return remoteTable;

}

int rtable_connect(struct rtable_t *table) {
	return network_connect(table);
}


/*
 * Fecha a ligação com o servidor, desaloca toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *table) {

    //verifica se table aponta para NULL
    if(table == NULL) {
        ERROR("remote_table: rtable_disconnect: NULL table");
        return -1;
    }

    //verifica se a ligação é fechada sem erros
    if((network_close(table)) == -1) {
        ERROR("remote_table: network_close");
        return -1;
    }
    free(table->ip);
    free(table->porto);
    free(table);

    //em caso de sucesso
    return 0;

}

/*
 * Função para adicionar um elemento na tabela.
 * Se key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory).
 */
int rtable_put(struct rtable_t *table, struct entry_t *entry) {

    //verifica se table, key ou data apontam para NULL
    if(table == NULL || entry == NULL) {
        ERROR("remote_table: NULL table or key or data");
        return -1;
    }

    //preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_PUT;
    msg.c_type = CT_ENTRY;
    msg.content.entry = entry;

    //envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("remote_table: network_send_receive");
        entry_destroy(entry);
        return -1;
    }

    //verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_PUT + 1)) {
        ERROR("remote_table: invalid message");
        entry_destroy(entry);
        free_message(rsp);
        return -1;
    }
    
    //em caso de sucesso
    free_message(rsp);
	entry_destroy(entry);
    return 0;

}

/*
 * Função para obter um elemento da tabela.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtable_get(struct rtable_t *table, char *key) {

    //verifica se table ou key apontam para NULL
    if(table == NULL || key == NULL) {
        ERROR("remote_table: NULL table or key");
        return NULL;
    }

    //preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_GET;
    msg.c_type = CT_KEY;
    msg.content.key = key;

    //envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("remote_table: network_send_receive");
        return NULL;
    }

    //verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_GET + 1)) {
		if(rsp->opcode == OP_RT_ERROR && rsp->content.result != 0) {
			ERROR("remote_table: invalid message");
		}
		free_message(rsp);
        return NULL;
    }

    //duplica a data recebida na resposta
    struct data_t *data = NULL;
	if(rsp->content.value && (data = data_dup(rsp->content.value)) == NULL) {
        ERROR("remote_table: data_dup");
        free_message(rsp);
        return NULL;
    }

    //em caso de sucesso
    free_message(rsp);
    return data;

}

/*
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação rtable_put()
 * Devolve: 0 (ok), -1 (key not found).
 */
int rtable_del(struct rtable_t *table, char *key) {
    int retVal = 0;

    //verifica se table ou key apontam para NULL
    if(table == NULL || key == NULL) {
        ERROR("remote_table: NULL table or key");
        return -1;
    }

    //preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_DEL;
    msg.c_type = CT_KEY;
    msg.content.key = key;

    //envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("remote_table: network_send_receive");
        return -1;
    }

    //verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_DEL + 1)) {
        ERROR("remote_table: invalid message");
        free_message(rsp);
        return -1;
    }

    //em caso de sucesso
    retVal = rsp->content.result;
    free_message(rsp);
    return retVal;

}

/*
 * Devolve o número de elementos da tabela.
 */
int rtable_size(struct rtable_t *table) {

    //verifica se table aponta para NULL
    if(table == NULL) {
        ERROR("remote_table: NULL table");
        return -1;
    }

    //preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_SIZE;
    msg.c_type = CT_RESULT;
    msg.content.result = 0;

    //envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("remote_table: network_send_receive");
        return -1;
    }

    //verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_SIZE + 1)) {
        ERROR("remote_table: invalid message");
        free_message(rsp);
        return -1;
    }

    //em caso de sucesso
    int size = rsp->content.result;
    free_message(rsp);
    return size;

}

/*
 * Devolve um array de char* com a cópia de todas as keys da tabela,
 * e um último elemento NULL.
 */
char **rtable_get_keys(struct rtable_t *table) {

    //verifica se table aponta para NULL
    if(table == NULL) {
        ERROR("remote_table: NULL table");
        return NULL;
    }

    //preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_GETKEYS;
    msg.c_type = CT_RESULT;
    msg.content.result = 0;

    //envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("remote_table: network_send_receive");
        return NULL;
    }

    //verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_GETKEYS + 1)) {
        ERROR("remote_table: invalid message");
        free_message(rsp);
        return NULL;
    }

    //determina o tamanho da lista de chaves
    int i = 0;
    int nKeys = 0;
    while (rsp->content.keys[i] != NULL) {
        nKeys++;
        i++;
    }
    nKeys ++;   //para colocar na última posição NULL

    //aloca memória para o novo vector de keys
    char **keys;
    if((keys = (char **) malloc(sizeof(char *) * nKeys)) == NULL) {
        ERROR("remote_table: malloc keys");
        free_message(rsp);
        return NULL;
    }

    //duplica todas as chaves da lista original
    i = 0;
    while (rsp->content.keys[i] != NULL) {
        if((keys[i] = strdup(rsp->content.keys[i])) == NULL) {
            ERROR("remote_table: strdup keys");
            table_free_keys(keys);
            free_message(rsp);
            return NULL;
        }
        i++;
    }
    keys[nKeys - 1] = NULL;     //coloca o último elemento da lista a NULL

    //em caso de sucesso
    free_message(rsp);
    return keys;

}

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave não encontrada devolve 0.
 */
long rtable_get_ts(struct rtable_t *table, char *key) {

    // Verifica os parâmetros
    if(table == NULL || key == NULL) {
        ERROR("NULL table or key");
        return -1;
    }

    // Preenche os campos da mensagem
    struct message_t msg;
    msg.opcode = OP_RT_GETTS;
    msg.c_type = CT_KEY;
    msg.content.key = key;

    // Envia a mensagem e recebe a resposta
    struct message_t *rsp;
    if((rsp = network_send_receive(table, &msg)) == NULL) {
        ERROR("network_send_receive");
        return -1;
    }

    // Verifica se a resposta é válida
    if(rsp->opcode != (OP_RT_GETTS + 1)) {
        if(rsp->opcode == OP_RT_ERROR) {
            ERROR("OP_RT_ERROR");
        }
        free_message(rsp);
        return -1;
    }

    // Em caso de sucesso
    long ts = rsp->content.timestamp;
	printf("Timestamp recebido: %ld\n", ts);
    free_message(rsp);
    return ts;

}