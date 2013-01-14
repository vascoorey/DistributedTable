/* 
 * File:   persistent_table.c
 *
 * Implementação de funções que trabalham sobre uma tabela e um persistence
 * manager.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "base64.h"
#include "persistent_table.h"
#include "persistent_table-private.h"
#include "message.h"
#include "message-private.h"
#include "utils.h"

/*
 * Abre o acesso a uma tabela persistente, passando como parâmetro a tabela
 * a ser mantida em memória e o gestor de persistência a ser usado para manter
 * logs e checkpoints. Retorna a tabela persistente criada ou NULL em caso de
 * erro.
 */
struct ptable_t *ptable_open(struct table_t *table, struct pmanager_t *pmanager) {

    //varifica a validade dos parâmetros
    if(table == NULL && pmanager == NULL) {
        ERROR("persistent_table: NULL table or NULL pmanager");
        return NULL;
    }

    //aloca memória e cria a nova tabela persistente
    struct ptable_t *ptable;
    if((ptable = (struct ptable_t *) malloc(sizeof(struct ptable_t))) == NULL) {
        ERROR("persistent_table: malloc ptable");
        return NULL;
    }
    
    ptable->table = table;
    ptable->pmanager = pmanager;

    //verifica se existem dados no log e caso existam, actualiza a tabela
    if(pmanager_have_data(ptable->pmanager)) {

        //preenche a tabela com os dados
        if(pmanager_fill_state(ptable->pmanager, ptable->table) != 0) {
            ERROR("persistent_table: pmanager_fill_state");
        }

        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_store_table(ptable->pmanager, ptable->table) < 0) {
            ERROR("persistent_table: pmanager_store_table");
            return NULL;
        }

        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_rotate_log(ptable->pmanager) != 0) {
            ERROR("persistent_table: pmanager_rotate_log");
            return NULL;
        }

    }

    //em caso de sucesso
    return ptable;

}

/*
 * Fecha o acesso a esta tabela persistente. Todas as operações em table
 * devem falhar apos um close.
 */
void ptable_close(struct ptable_t *table) {
    if(table) {
        //destroi a tabela
        table_destroy(table->table);

        //destroi o gestor de persistência
        pmanager_destroy(table->pmanager);
		
		free(table);
    }
    
}

/*
 * Liberta toda memória e apaga todos os ficheiros utilizados pela tabela.
 */
void ptable_destroy(struct ptable_t *table) {

    if(table) {
        //destroi a tabela
        table_destroy(table->table);

        //destroi o gestor de persistência e limpa os ficheiros
	pmanager_destroy_clear(table->pmanager);
		
		free(table);
    }

}

/*
 * Função para adicionar um elemento na tabela.
 * A função vai *COPIAR* a key (string) e os dados num espaco de memória
 * alocado por malloc().
 * Se a key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory).
 */
int ptable_put(struct ptable_t *table, char *key, struct data_t *data) {

	struct timeval start, end;
	long secDiff, usecDiff;
	
    //verifica a validade dos parâmetros
    if(table == NULL || key == NULL || data == NULL) {
        ERROR("persistent_table: NULL table or key or data");
        return -1;
    }
    
    //insere a entrada na tabela
    if(table_put(table->table, key, data) != 0) {
        ERROR("persistent_table: table_put auxKey, auxData");
        return -1;
    }

    //prepara a mensagem
    char *encodedData;
    int encodedSize;
    if(data->data) {
		printf("Data tem dados...\n");
		if((encodedSize = base64_encode_alloc(data->data, data->datasize, &encodedData)) < 0) {
			ERROR("persistent_table: base64_encode_alloc");
			return -1;
		}
    } else if((encodedSize = base64_encode_alloc("0", 1, &encodedData)) < 0) {
		ERROR("persistent_table: base64_encode_alloc");
		return -1;
	}
	size_t encodedTsSize;
	char *ts = NULL;
	encode_timestamp(data->timestamp, &encodedTsSize, &ts);
	
	if(encodedTsSize <= 0) {
		ERROR("encode_timestamp");
		return -1;
	}

    char *msg;                  //formato: "put TS-BASE64 key DATA-BASE64"
    if((msg = (char *) malloc(sizeof(char) * (7 + strlen(key) + encodedSize + strlen(ts)))) == NULL) {
        ERROR("persistent_table: malloc msg");
        return -1;
    }

    sprintf(msg, "put %s %s %s", ts, key, encodedData);
	free(ts);
	printf("Log: %s\n", msg);
	if(PRINT_LATENCIES) {
		gettimeofday(&start, NULL);
	}
    //regista a operação no log
    if((pmanager_log(table->pmanager, msg)) < 0) {
        //ERROR("persistent_table: pmanager_log");

        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_store_table(table->pmanager, table->table) < 0) {
            ERROR("persistent_table: pmanager_store_table");
            return -1;
        }

        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_rotate_log(table->pmanager) != 0) {
            ERROR("persistent_table: pmanager_rotate_log");
            return -1;
        }
	if((pmanager_log(table->pmanager, msg)) < 0) {
        	free(msg);
       		return -1;
	}
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

    //em caso de sucesso
    free(msg);
	free(encodedData);
    return 0;

}

/*
 * Função para obter um elemento da tabela.
 * O argumento key indica a key da entrada da tabela. A função
 * aloca memória para armazenar uma *CÓPIA* dos dados da tabela e
 * retorna este endereço. O programa a usar essa função deve
 * desalocar (utilizando free()) esta memória.
 * Em caso de erro, devolve NULL.
 */
struct data_t *ptable_get(struct ptable_t *table, char *key) {

    //verifica a validade dos parâmetros
    if(table == NULL || key == NULL) {
        ERROR("persistent_table: NULL table or key");
        return NULL;
    }

    return table_get(table->table, key);

}

/*
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação ptabel_put().
 * Devolve: 0 (ok), -1 (key not found).
 */
int ptable_del(struct ptable_t *table, char *key) {

    //verifica a validade dos parâmetros
    if(table == NULL || key == NULL) {
        ERROR("persistent_table: NULL table or key");
        return -1;
    }

    //insere a entrada na tabela
    if(table_del(table->table, key) != 0) {
        ERROR("persistent_table: table_del");
        return -1;
    }

    //prepara a mensagem
    char *msg;                  //formato: "del key"
    if((msg = (char *) malloc(sizeof(char) * (5 + strlen(key)))) == NULL) {
        ERROR("persistent_table: malloc msg");
        return -1;
    }

    sprintf(msg, "del %s", key);

    //regista a operação no log
    if((pmanager_log(table->pmanager, msg)) < 0) {
        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_store_table(table->pmanager, table->table) < 0) {
            ERROR("persistent_table: pmanager_store_table");
            return -1;
        }

        //cria um ficheiro temporário com o estado da tabela
        if(pmanager_rotate_log(table->pmanager) != 0) {
            ERROR("persistent_table: pmanager_rotate_log");
            return -1;
        }
	if((pmanager_log(table->pmanager, msg)) < 0) {
        	free(msg);
       		return -1;
	}
    }

    //em caso de sucesso
    free(msg);
    return 0;

}

/*
 * Devolve o número de elementos da tabela.
 */
int ptable_size(struct ptable_t *table) {

    if(table) {
        return table_size(table->table);
    }

    //em caso de erro
    return -1;
    
}

/*
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **ptable_get_keys(struct ptable_t *table) {

    if(table) {
        return table_get_keys(table->table);
    }

    //em caso de erro
    return NULL;
}

/*
 * Liberta a memória alocada por ptable_get_keys().
 */
void ptable_free_keys(char **keys) {

    if(keys) {
        table_free_keys(keys);
    }

}

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave nao encontrada devolve 0.
 */
long ptable_get_ts(struct ptable_t *ptable, char *key) {

    // Verifica os parâmetros
    if(ptable == NULL || key == NULL) {
        ERROR("NULL ptable or key");
        return -1;
    }

    // Procura a entrada na ptabela
    long ts;
    if((ts = table_get_ts(ptable->table, key)) < 0) {
        ERROR("table_get_ts");
        return ts;
    }
	
	printf("Timestamp encontrado: %ld\n", ts);
    // Em caso de sucesso
    return ts;

}
