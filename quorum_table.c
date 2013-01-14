/* 
 * File:   quorum_table.c
 *
 * Define a estrutura e o comportamento de uma quorum table.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "utils.h"
#include "data.h"
#include "quorum_table.h"
#include "quorum_table-private.h"
#include "quorum_access.h"
#include "quorum_access-private.h"

/* 
 * Função para estabelecer uma associação com uma tabela e um array de n
 * servidores. addresses_ports é um array de strings e n é o tamanho
 * deste array.
 * Retorna NULL caso não consiga criar o qtable_t.
 */
struct qtable_t *qtable_connect(const char **addresses_ports, int n, int id) {
	
    int i,                  // Usado em ciclos
	failedConnections = 0;  // Regista o total de ligações falhadas
	// Se forem superiores a (n-1)/2 servidores devolve NULL
	
    // Verifica os parâmetros
    if(addresses_ports == NULL || n <= 0) {
        ERROR("NULL addresses_ports or n <= 0");
        return NULL;
    }
	
    // Aloca memória para uma qtable e verifica-a
    struct qtable_t *qtable = NULL;
    if((qtable = (struct qtable_t *) malloc(sizeof(struct qtable_t))) == NULL) {
        ERROR("malloc qtable");
        return NULL;
    }
	
    // Aloca memória para o vector de servidores disponíveis e verifica-a
    /*if((qtable->serversAdrress = (char **) malloc(sizeof(char *) * n)) == NULL) {
        ERROR("malloc qtable->serversAddress");
        qtable_disconnect(qtable);
        return NULL;
    }*/
	
	qtable->numServers = n;
	
    // Aloca memória para cada string <ip:porto> dos servidores e copia-a
    for(i = 0; i < n; i++) {
		
        // Verifica a cópia de cada string
		printf("%s\n", addresses_ports[i]);
        /*if(!(qtable->serversAdrress[i] = strdup(addresses_ports[i]))) {
		 ERROR("strdup addresses_ports");
		 qtable_disconnect(qtable);
		 return NULL;
		 }*/
		qtable->serversAdrress = (char**)addresses_ports;
    }
	
    // Aloca memória para o vector de servidores disponíveis e verifica-a
    if((qtable->servers = (struct rtable_t **) malloc(
													  sizeof(struct rtable_t*) * n)) == NULL) {
        ERROR("malloc qtable->servers");
        qtable_disconnect(qtable);
        return NULL;
    }
	
    // Inicia uma ligação a todos os servidores
    for(i = 0; i < n; i++) {
        if((qtable->servers[i] = rtable_open(qtable->serversAdrress[i])) == NULL) {
            failedConnections++;    // Não deixa de tentar as restantes ligações
        }
    }
	
    // Verifica que o número de ligações é superior a (n-1)/2 servidores
    if(failedConnections > ((n-1)/2)) {
        ERROR("failedConnections > ((n-1)/2)");
        return NULL;
    }
	if(init_quorum_access(qtable->servers, n) == -1) {
		ERROR("init_quorum_access");
		return NULL;
	}
	qtable->id = id;
    //em caso de sucesso
    return qtable;
    
}

/* 
 * Fecha a ligação com os servidores do sistema e liberta a memória alocada
 * para qtable.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int qtable_disconnect(struct qtable_t *qtable) {
	
    int i,              // Usado em ciclos
	falhou = 0;     // Regista a falha do fecho da liagação a uma tabela remota
	
	printf("A desligar a qtable...\n");
    if(qtable == NULL) {
        ERROR("quorum_table: qtable_disconnect");
        return -1;
    }
    else {
		destroy_quorum_access(); // Inclusive todas as threads, então vai ser seguro chamar o rtable_disconnect
        if(qtable->servers) {
            // Desliga a ligação a todas as tabelas remotas
            for(i = 0; i < qtable->numServers; i++) {
                if(qtable->servers[i]) {
                    if(rtable_disconnect(qtable->servers[i]) != 0) {
                        ERROR("quorum_table: rtable_disconnect");
                        falhou = -1;    // Não deixa de tentar desligar as restantes
                    }
                }
            }
			
            // Liberta a memória do vector de tabelas remotas
            /*for(i = 0; i < qtable->numServers; i++) {
                if(qtable->servers[i]) {
                    free(qtable->servers[i]);
                }
            }*/
            free(qtable->servers);
        }
        
        // Liberta a memória do vector de <IP's:portos>
		// Não é preciso! E o argv...
        /*if(qtable->serversAdrress) {
            for(i = 0; i < qtable->numServers; i++) {
                free(qtable->serversAdrress[i]);
            }
            free(qtable->serversAdrress);
        }*/
        free(qtable);
    }
	
    // O resultado
    return falhou;
	
}

/* 
 * Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Note que o timestamp em data será atribuído internamente a esta função,
 * como definido no protocolo de escrita.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_put(struct qtable_t *qtable, char *key, struct data_t *data) {

    int i;              // Usado em ciclos
    long maxTS = 0;     // Regista o maior timestamp

    // Verifica os parâmetros
    if(qtable == NULL || key == NULL || data == NULL) {
        ERROR("NULL qtable or key or data");
        return -1;
    }

    // Aloca memória para a operação
    struct quorum_op_t *op;
    if((op = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t)))== NULL) {
        ERROR("malloc op");
        return -1;
    }

    // Duplica a chave recebida
    char *tempKey;
    if((tempKey = strdup(key)) == NULL) {
        ERROR("strdup key");
        free(op);
        return -1;
    }

    // Configura a operação para obter o timestamp
    op->id = 0;
    op->sender = 0;
    op->opcode = OP_RT_GETTS;
    op->content.key = tempKey;

    // Recebe as respostas dos vários servidores
    struct quorum_op_t **ret;
    if((ret = quorum_access(op, (qtable->numServers/2 + 1))) == NULL) {
        ERROR("quorum_access OP_RT_GETTS");
        free(op);
        free(tempKey);
        return -1;
    }

    // Compara os valores recebidos na resposta
    for(i = 0; i < qtable->numServers; i++) {
        if(ret[i]) { //&& (ret[i]->content.timestamp > maxTS)) {
			//printf("Timestamp %d: %ld\n", i, ret[i]->content.timestamp);
            maxTS = ret[i]->content.timestamp;
        }
    }
	
	// Verifica se o maxTS foi alterado
    if(maxTS > 0) {
        // Incrementa o timestamp e adiciona o id do cliente
        maxTS = update_timestamp(maxTS, qtable->id);
    }
    else {
        // Inicia um novo timestamp com o id do cliene
        maxTS = 1000 + qtable->id;
    }

    // Duplica a data e actualiza o timestamp
    struct data_t *tempData;
    if((tempData = data_dup(data)) == NULL) {
        ERROR("data_dup");
        free(op);
        free(tempKey);
        qtable_free_quorum_op_t(ret, qtable->numServers);
        return -1;
    }
    tempData->timestamp = maxTS;

    // Cria uma entrada nova com a key e a data actualizada
    struct entry_t *tempEntry;
    if((tempEntry = entry_create(tempKey, tempData)) == NULL) {
        ERROR("entry_create");
        free(op);
        free(tempKey);
        qtable_free_quorum_op_t(ret, qtable->numServers);
        data_destroy(tempData);
        return -1;
    }

    // Configura a operação para inserir a entrada
    op->opcode = OP_RT_PUT;
    op->content.entry = tempEntry;

    // Limpa a ret antes de receber as novas respostas
    qtable_free_quorum_op_t(ret, qtable->numServers);

    // Recebe as respostas dos vários servidores
    if((ret = quorum_access(op, (qtable->numServers/2 + 1))) == NULL) {
        ERROR("quorum_access OP_RT_PUT");
        free(op);
        free(tempKey);
        qtable_free_quorum_op_t(ret, qtable->numServers);
        data_destroy(tempData);
        entry_destroy(tempEntry);
        return -1;
    }

    // Verifica as respostas
    for(i = 0; i < qtable->numServers; i++) {
        if(ret[i] && (ret[i]->content.result != 0)) {
            /*ERROR("invalid put");
            free(op);
            free(tempKey);
            qtable_free_quorum_op_t(ret, qtable->numServers);
            data_destroy(tempData);
            entry_destroy(tempEntry);
            return -1;*/
			printf("tivemos um erro.\n");
        }
    }

    // Em caso de sucesso
    free(op);
    //free(tempKey);
    qtable_free_quorum_op_t(ret, qtable->numServers);
    //data_destroy(tempData);
    entry_destroy(tempEntry);
	printf("qtable_put: done!\n");
    return 0;

}

/* 
 * Função para obter um elemento da tabela.
 * Em caso de erro ou elemento não existente, devolve NULL.
 */
struct data_t *qtable_get(struct qtable_t *qtable, char *key) {

    int i = -1;              // Usado em ciclos
    long maxTS = -1;     // Regista o maior timestamp
    int diff = -1;      // Regista uma divergência de valores
    int index = -1;          // Regista o índice de uma data com o maior timestamp

    // Verifica os parâmetros
    if(qtable == NULL || key == NULL) {
        ERROR("NULL qtable or key");
        return NULL;
    }

    // Aloca memória para a operação
    struct quorum_op_t *op;
    if((op = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t)))== NULL) {
        ERROR("malloc op");
        return NULL;
    }

    // Duplica a chave recebida
    char *tempKey; 
    if((tempKey = strdup(key)) == NULL) {
        ERROR("strdup key");
        free(op);
        return NULL;
    }

    // Configura a operação para obter o timestamp
    op->id = 0;
    op->sender = 0;
    op->opcode = OP_RT_GET;
    op->content.key = tempKey;

    // Recebe as respostas dos vários servidores
    struct quorum_op_t **ret;
    if((ret = quorum_access(op, (qtable->numServers/2) + 1)) == NULL) {
        ERROR("quorum_access OP_RT_GET");
        free(op);
        free(tempKey);
        return NULL;
    }

    // Compara os valores recebidos na resposta
    for(i = 0; i < qtable->numServers; i++) {
        if(ret[i] && (ret[i]->content.timestamp > maxTS)) {
			//printf("********** %ld, %ld\n", maxTS, ret[i]->content.timestamp);
            maxTS = ret[i]->content.timestamp;
            diff++;
            index = i;
        }
    }
	struct data_t *data;
	if(index >= 0) {
		// Duplica a data
		
		if(ret[index]->content.value && (data = data_dup(ret[index]->content.value)) == NULL) {
			//ERROR("data_dup");
			// Entrada não existe
			free(op);
			free(tempKey);
			qtable_free_quorum_op_t(ret, qtable->numServers);
			return NULL;
		}
	} else {
		return NULL;
	}

	//printf("Diff: %d\n", diff);
    // Verifica a divergência de valores na resposta
    if(diff > 0) {

        // Duplica a data para ser usada na entrada a ser enviada aos servidores
        struct data_t *tempData;
        if((tempData = data_dup(ret[index]->content.value)) == NULL) {
            ERROR("data_dup");
            free(op);
            free(tempKey);
            qtable_free_quorum_op_t(ret, qtable->numServers);
            data_destroy(data);
            return NULL;
        }
		tempData->timestamp = maxTS;
		//printf("TIMESTAMP: %ld\n", tempData->timestamp);
        // Cria uma entrada nova
        struct entry_t *tempEntry;
        if((tempEntry = entry_create(tempKey, tempData)) == NULL) {
            ERROR("entry_create");
            free(op);
            free(tempKey);
            qtable_free_quorum_op_t(ret, qtable->numServers);
            data_destroy(data);
            data_destroy(tempData);
            return NULL;
        }

        // Configura a operação para inserir a entrada
        op->opcode = OP_RT_PUT;
        op->content.entry = tempEntry;

        // Limpa a ret antes de receber as novas respostas
        qtable_free_quorum_op_t(ret, qtable->numServers);

        // Recebe as respostas dos vários servidores
        if((ret = quorum_access(op, ceil((qtable->numServers - 1)/2))) == NULL) {
            ERROR("quorum_access OP_RT_PUT");
            free(op);
            free(tempKey);
            qtable_free_quorum_op_t(ret, qtable->numServers);
            data_destroy(data);
            data_destroy(tempData);
            entry_destroy(tempEntry);
            return NULL;
        }

        // Verifica as respostas
        for(i = 0; i < qtable->numServers; i++) {
            if(ret[i] && (ret[i]->content.result != 0)) {
                ERROR("invalid put");
                free(op);
                free(tempKey);
                qtable_free_quorum_op_t(ret, qtable->numServers);
                data_destroy(data);
                data_destroy(tempData);
                entry_destroy(tempEntry);
                return NULL;
            }
        }
        //data_destroy(tempData);
        entry_destroy(tempEntry);
    }

    // Em caso de sucesso
    free(op);
    //free(tempKey);
    qtable_free_quorum_op_t(ret, qtable->numServers);
    return data;
    
}

/*
 * Função para remover um elemento da tabela. É equivalente a execução
 * put(k,NULL) se a chave existir. Se a chave não existir, nada acontece.
 * Devolve 0 (ok), -1 (chave não encontrada).
 */
int qtable_del(struct qtable_t *qtable, char *key) {
	
	int i;              // Usado em ciclos
	long maxTS = 0;     // Regista o maior timestamp
	
	// Verifica os parâmetros
	if(qtable == NULL || key == NULL) {
		ERROR("NULL qtable or key");
		return -1;
	}
	
	// Aloca memória para a operação
	struct quorum_op_t *op;
	if((op = (struct quorum_op_t *) malloc(sizeof(struct
												  quorum_op_t)))== NULL) {
		ERROR("malloc op");
		return -1;
	}
	
	// Duplica a chave recebida
	char *tempKey;
	if((tempKey = strdup(key)) == NULL) {
		ERROR("strdup key");
		free(op);
		return -1;
	}
	
	// Configura a operação para obter o timestamp
	op->id = 0;
	op->sender = 0;
	op->opcode = OP_RT_GETTS;
	op->content.key = tempKey;
	
	// Recebe as respostas dos vários servidores
	struct quorum_op_t **ret;
	if((ret = quorum_access(op, qtable->numServers/2 + 1)) == NULL) {
		ERROR("quorum_access OP_RT_GETTS");
		free(op);
		free(tempKey);
		return -1;
	}
	
	// Compara os valores recebidos na resposta
	for(i = 0; i < qtable->numServers; i++) {
		if(ret[i] && (ret[i]->content.timestamp > maxTS)) {
			maxTS = ret[i]->content.timestamp;
		}
	}
	
	// Verifica se o maxTS foi alterado
	if(maxTS > 0) {
		// Incrementa o timestamp e adiciona o id do cliente
		maxTS = update_timestamp(maxTS, qtable->id);
	}
	
	// Cria uma data com o value a NULL e actualiza o timestamp
	struct data_t *tempData;
	if((tempData = data_create(0)) == NULL) {
		ERROR("data_dup");
		free(op);
		free(tempKey);
		qtable_free_quorum_op_t(ret, qtable->numServers);
		return -1;
	}
	tempData->timestamp = maxTS;
	
	// Cria uma entrada nova com a key e a data actualizada
	struct entry_t *tempEntry;
	if((tempEntry = entry_create(tempKey, tempData)) == NULL) {
		ERROR("entry_create");
		free(op);
		free(tempKey);
		qtable_free_quorum_op_t(ret, qtable->numServers);
		data_destroy(tempData);
		return -1;
	}
	
	// Configura a operação para inserir a entrada
	op->opcode = OP_RT_PUT;
	op->content.entry = tempEntry;
	
	// Limpa a ret antes de receber as novas respostas
	qtable_free_quorum_op_t(ret, qtable->numServers);
	
	// Recebe as respostas dos vários servidores
	if((ret = quorum_access(op, (qtable->numServers/2) + 1)) == NULL) {
		ERROR("quorum_access OP_RT_PUT");
		free(op);
		free(tempKey);
		qtable_free_quorum_op_t(ret, qtable->numServers);
		data_destroy(tempData);
		entry_destroy(tempEntry);
		return -1;
	}
	
	// Verifica as respostas
	for(i = 0; i < qtable->numServers; i++) {
		if(ret[i] && (ret[i]->content.result != 0)) {
			/*ERROR("invalid put");
			 free(op);
			 free(tempKey);
			 qtable_free_quorum_op_t(ret, qtable->numServers);
			 data_destroy(tempData);
			 entry_destroy(tempEntry);
			 return -1;*/
			printf("tivemos um erro.\n");
		}
	}
	
	// Em caso de sucesso
	free(op);
	//free(tempKey);
	qtable_free_quorum_op_t(ret, qtable->numServers);
	//data_destroy(tempData);
	entry_destroy(tempEntry);
	//printf("qtable_put: done!\n");
	return 0;
	
}

/* Devolve numero (aproximado) de elementos da tabela ou -1 em caso de
 * erro.
 */
int qtable_size(struct qtable_t *qtable) {
	
	int i;              // Usado em ciclos
	int numReplies = 0;   // Regista o maior número de elementos
	int result = 0;
	
	// Verifica os parâmetros
	if(qtable == NULL) {
		ERROR("NULL qtable");
		return -1;
	}
	
	// Aloca memória para a operação
	struct quorum_op_t *op;
	if((op = (struct quorum_op_t *) malloc(sizeof(struct
												  quorum_op_t)))== NULL) {
		ERROR("malloc op");
		return -1;
	}
	
	// Configura a operação para obter o número de elementos
	op->id = 0;
	op->sender = 0;
	op->opcode = OP_RT_SIZE;
	op->content.result = 0;
	
	// Recebe as respostas dos vários servidores
	struct quorum_op_t **ret;
	if((ret = quorum_access(op, (qtable->numServers/2 + 1))) == NULL) {
		ERROR("quorum_access OP_RT_GETTS");
		free(op);
		return -1;
	}
	free(op);
	
	// Verifica as respostas
	for(i = 0; i < qtable->numServers; i++) {
		if(ret[i] && (ret[i]->content.result < 0)) {
			ERROR("invalid size");
			free(op);
			qtable_free_quorum_op_t(ret, qtable->numServers);
			return -1;
			printf("tivemos um erro no qtable_size.\n");
		}
	}
	
	// Compara os valores recebidos na resposta
	for(i = 0; i < qtable->numServers; i ++) {
		if(ret[i] && ret[i]->content.result >= 0) {
			result += ret[i]->content.result;
			numReplies ++;
		}
	}
	result /= numReplies;
	
	qtable_free_quorum_op_t(ret, qtable->numServers);
	// Em caso de sucesso
	return result;
	
}

/* Devolve um array de char* com a copia de todas as keys da tabela,
 * e um ultimo elemento a NULL. Esta funcao nao deve retornar as
 * chaves removidas, i.e., a NULL.
 */
char **qtable_get_keys(struct qtable_t *qtable) {
	
	int i, j;           // Usado em ciclos
	int nElem = 0;      // Usado para registar os elementos de cada array
	int maxElem = 0;    // Usado para registar o maior numero de keys
	int index = 0;      // Usado para registar o array com maior número de keys
	
	// Verifica os parâmetros
	if(qtable == NULL) {
		ERROR("NULL qtable");
		return NULL;
	}
	
	// Aloca memória para a operação
	struct quorum_op_t *op;
	if((op = (struct quorum_op_t *) malloc(sizeof(struct
												  quorum_op_t)))== NULL) {
		ERROR("malloc op");
		return NULL;
	}
	
	// Configura a operação para obter o número de elementos
	op->id = 0;
	op->sender = 0;
	op->opcode = OP_RT_GETKEYS;
	op->content.result = 0;
	
	// Recebe as respostas dos vários servidores
	struct quorum_op_t **ret;
	if((ret = quorum_access(op, qtable->numServers/2 +1)) == NULL) {
		ERROR("quorum_access OP_RT_GETTS");
		free(op);
		return NULL;
	}
	
	// Verifica as respostas
	for(i = 0; i < qtable->numServers; i++) {
		if(ret[i]) {
			if(ret[i]->content.keys[0]) {
				// Para cada resposta do servidor detrmina o número de elementos
				for(j = 0; ret[i]->content.keys[j]; j++) {
					nElem++;
				}
				
				// Compara com o número máximo de elementos verificados até ao momento
				if(nElem > maxElem) {
					maxElem = nElem;
					index = i;
				}
				nElem = 0;
			}
		}
		else {
			ERROR("invalid keys");
			free(op);
			qtable_free_quorum_op_t(ret, qtable->numServers);
			return NULL;
		}
		
	}
	
	// Aloca memória para as chaves a retornar
	char **keys;
	if((keys = (char **) malloc(sizeof(char *) * maxElem)) == NULL) {
		ERROR("malloc keys");
		free(op);
		qtable_free_quorum_op_t(ret, qtable->numServers);
		return -1;
	}
	
	// Duplica cada chave
	for(i = 0; i < maxElem; i++) {
		
		if((keys[i] = strdup(ret[index]->content.keys[i])) == NULL) {
			ERROR("malloc keys");
			free(op);
			qtable_free_quorum_op_t(ret, qtable->numServers);
			qtable_free_keys(keys);
			return NULL;
		}
		
	}
	
	// Em caso de sucesso
	free(op);
	qtable_free_quorum_op_t(ret, qtable->numServers);
	return keys;
	
}

/* Desaloca a memoria alocada por qtable_get_keys().
 */
void qtable_free_keys(char **keys) {
	
	if(keys) {
		
		int counter = 0;
		while(keys[counter]) {
			free(keys[counter]);
			counter++;
		}
		free(keys);
		
	}
	
}

void qtable_free_quorum_op_t(struct quorum_op_t **ret, int n) {
	
    if(ret) {
        int i;
        for(i = 0; i < n; i++) {
            if(ret[i]) {
                free(ret[i]);
            }
        }
        free(ret);
    }
	
}

long update_timestamp(long ts, int id) {
	
    long timestamp = (long) (ts / 1000);
    timestamp++;
    timestamp *= 1000;
    timestamp += id;
	return timestamp;
}





