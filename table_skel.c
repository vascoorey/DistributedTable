/*
 * File:   table_skel.c
 *
 * Rotina de adaptação do servidor.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "table.h"
#include "table-private.h"
#include "remote_table.h" // Para os opcodes
#include "table_skel.h"
#include "utils.h"
#include "persistent_table.h"

/*
 * MAX_LOG_SIZE: Tamanho máximo que o ficheiro log pode ter.
 */
#define MAX_LOG_SIZE 10000

static struct ptable_t *sharedPtable = NULL;

/*
 * Inicia o skeleton da tabela.
 * O main() do servidor deve chamar este método antes de usar a
 * funço invoke(). O parâmetro n_lists define o número de listas a serem
 * usadas pela tabela mantida no servidor. O parâmetro filename corresponde ao
 * nome a ser usado nos ficheiros de log e checkpoint.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY).
 */
int table_skel_init(int n_lists, char *filename) {

    //verifica a validade dos parâmetros
    if(n_lists <= 0 || filename == NULL) {
        ERROR("table_skel: NULL filename or invalid nlists");
        return -1;
    }

    if(!sharedPtable) {

        //cria e verifica uma nova tabela
        struct table_t *sharedTable;
        if((sharedTable = table_create(n_lists)) == NULL) {
            ERROR("table_skel: table_create");
            return -1;
        }

        //cria e verifica um novo persistence_manager
        struct pmanager_t *sharedPmanager;
        if((sharedPmanager = pmanager_create(filename, MAX_LOG_SIZE, MODE_DSYNC)) == NULL) {
            ERROR("table_skel: pmanager_create");
            table_destroy(sharedTable);
            return -1;
        }

        //cria e verifica um persistent_table
        if((sharedPtable = ptable_open(sharedTable, sharedPmanager)) == NULL) {
            ERROR("table_skel: ptable_open");
            table_destroy(sharedTable);
            pmanager_destroy(sharedPmanager);
            return -1;
        }
    }

    //em caso de sucesso
    return 0;

}

/* 
 * Serve para libertar toda a memória alocada pela função anterior.
 */
int table_skel_destroy() {

    if(sharedPtable) {
        ptable_close(sharedPtable);
    }

    //em caso de sucesso
    return 0;
    
}

/* 
 * Executar uma função (indicada pelo opcode na msg) e retorna o resultado na
 * própria struct msg.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao inicializada).
 */
int invoke(struct message_t *msg) {

    int retVal = 0;
    char *key;
    struct entry_t *entry;

    if(sharedPtable && msg) {
        switch (msg->opcode) {
            case OP_RT_GET:
                // table_get: (struct table_t* char*) -> (struct data_t*)
                if(msg->content.key) {
                    key = msg->content.key;
                    if(!(msg->content.value = ptable_get(sharedPtable, key))) {
                        if(!(msg->content.value = data_create(0))) {
                            msg->opcode = OP_RT_ERROR;
                            msg->c_type = CT_RESULT;
                            msg->content.result = -1;
                        }
                    }
                    if(msg->content.value) {
                        msg->opcode ++; // Incrementa para dizer que esta mensagem contem o resultado
                        msg->c_type = CT_VALUE;
                        free(key);
                    }
                }
                else {
                    msg->opcode = OP_RT_ERROR;
                    msg->c_type = CT_RESULT;
                    msg->content.result = -1;
                }
            break;

            case OP_RT_PUT:
                // table_put: (struct table_t* char* struct data_t*) -> (int)
                if(msg->content.entry) {
                    entry = msg->content.entry;
                    if((retVal = ptable_put(sharedPtable, entry->key, entry->value)) != -1) {
                        msg->content.result = retVal;
                        msg->opcode ++;
                        msg->c_type = CT_RESULT;
                    }
                    else {
						// Deu erro pq ficamos sem espaço de log.
						//cria um ficheiro temporário com o estado da tabela
						if(pmanager_store_table(sharedPtable->pmanager, sharedPtable->table) < 0) {
							ERROR("persistent_table: pmanager_store_table");
							msg->opcode = OP_RT_ERROR;
							msg->c_type = CT_RESULT;
							msg->content.result = -1;
						}
						
						//cria um ficheiro temporário com o estado da tabela
						if(pmanager_rotate_log(sharedPtable->pmanager) != 0) {
							ERROR("persistent_table: pmanager_rotate_log");
							msg->opcode = OP_RT_ERROR;
							msg->c_type = CT_RESULT;
							msg->content.result = -1;
						}
						if((retVal = ptable_put(sharedPtable, entry->key, entry->value)) != -1) {
							msg->content.result = retVal;
							msg->opcode ++;
							msg->c_type = CT_RESULT;
						} else {
							msg->opcode = OP_RT_ERROR;
							msg->c_type = CT_RESULT;
							msg->content.result = -1;
						}
                    }
                    entry_destroy(entry);
                }
                else {
                    msg->opcode = OP_RT_ERROR;
                    msg->c_type = CT_RESULT;
                    msg->content.result = -1;
                }
            break;

            case OP_RT_SIZE:
                // table_size: (struct table_t*) -> (int)
                msg->content.result = ptable_size(sharedPtable);
                msg->opcode ++;
                msg->c_type = CT_RESULT;
                // Nao temos nada para libertar...
            break;
	
            case OP_RT_DEL:
                // table_del: (struct table_t* char*) -> (int)
                if(msg->content.key) {
                    key = msg->content.key;
                    msg->content.key = NULL;
                    retVal = ptable_del(sharedPtable, key);
                    msg->content.result = retVal;
                    msg->opcode ++;
                    msg->c_type = CT_RESULT;
                    free(key);
                }
                else {
                    msg->opcode = OP_RT_ERROR;
                    msg->c_type = CT_RESULT;
                    msg->content.result = -1;
                }
            break;

            case OP_RT_GETKEYS:
                // table_get_keys: (struct table_t*) -> (char**)
                if((msg->content.keys = ptable_get_keys(sharedPtable))) {
                    msg->opcode ++;
                    msg->c_type = CT_KEYS;
                }
                else {
                    msg->opcode = OP_RT_ERROR;
                    msg->c_type = CT_RESULT;
                    msg->content.result = -1;
                }
                // Não temos nada para libertar...
            break;

            case OP_RT_GETTS:
                // table_get: (struct table_t* char*) -> (int)
                if(msg->content.key) {
                    key = msg->content.key;

                    // Verifica a validade da resposta
                    if((msg->content.timestamp = ptable_get_ts(sharedPtable, key)) < 0) {
                            msg->opcode = OP_RT_ERROR;
							msg->c_type = CT_RESULT;
                            msg->content.result = -1;
                    }

                    if(msg->content.timestamp >= 0) {
                        msg->opcode ++; // Incrementa para dizer que esta mensagem contem o resultado
                        msg->c_type = CT_TIMESTAMP;
                        free(key);
                    }
                }
                else {
                    msg->opcode = OP_RT_ERROR;
                    msg->c_type = CT_RESULT;
                    msg->content.result = -1;
                }
            break;

            default:
                ERROR("opcode");
                msg->opcode = OP_RT_ERROR;
                msg->c_type = CT_RESULT;
                msg->content.result = -1;
            break;
        }
    }
    else {
        ERROR("NULL sharedPtable");
        retVal = -1;
    }
    return retVal;
}
