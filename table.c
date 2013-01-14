/*
 * File:   table.c
 *
 * Implementacao da tabela hash.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "list.h"
#include "table.h"
#include "table-private.h"

#include "utils.h"

/*
 * Função para criar/inicializar uma nova tabela hash, com n linhas
 * (módulo da função HASH).
 */
struct table_t *table_create(int n) {
    
    // Reserva memória para a tabela hash
    struct table_t *table = NULL;
    int counter = 0, errorCounter;
    
    if((table = (struct table_t *) malloc(sizeof(struct table_t)))) {

        // Inicializa o tamanho e número de elementos da tabela
        table->hashSize = n;
        table->numElems = 0;
        table->numUpdates = 0;
        
        // Reserva memória para a tabela
        // Cada índice da tabela corresponde a um apontador para uma lista
        table->table = (struct list_t **) malloc (n * sizeof(struct list_t *));
        if(!table->table) {
            ERROR("Malloc table->table");
            free(table);
            table = NULL;
        }

        // Inicializar todas as listas da tabela
        for(counter = 0; counter < n; counter ++) {
            if(!(table->table[counter] = list_create())) {
                // Deu erro, temos que libertar tudo
                ERROR("list_create");
                for(errorCounter = 0; errorCounter < counter; errorCounter ++) {
                    list_destroy(table->table[errorCounter]);
                }
                free(table);
                table = NULL;
            }
        }
    }
    return table;
    
}

/*
 * Eliminar/desalocar toda a memória.
 */
void table_destroy(struct table_t *table) {

    int counter = 0;
        
    if(table) {
        for(counter = 0; counter < table->hashSize; counter ++) {
            if(table->table[counter]) {
                list_destroy(table->table[counter]);
            }
        }
        free(table->table);
        free(table);
    }
    else {
        ERROR("NULL table");
    }

}

/*
 * Função para adicionar um elemento na tabela.
 * A função vai copiar a key (string) e os dados num espaço de memória
 * alocado por malloc().
 * Se a key ja existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok) ou -1 (out of memory)
 */
int table_put(struct table_t *table, char *key, struct data_t *data) {

    int retVal = 0, hashValue;
    struct entry_t *tempEntry;
    struct data_t *tempData;

    if (table && key && data) {
        hashValue = hash(key, table->hashSize);

        if (table->numElems > 0 && (tempEntry = list_get(table->table[hashValue], key))) {

            // Já havia um elemento com a chave key na lista e com um timestamp menor
            if (tempEntry->value && (tempData = data_dup(data))) {
                data_destroy(tempEntry->value);
                tempEntry->value = tempData;
                table->numUpdates++;
			} else {
                ERROR("data_dup ou ma inicialização previa");
                retVal = -1;
            }
        }
        else {
            // Criar a entrada e inserir na lista.
            if ((tempEntry = entry_create(strdup(key), data_dup(data)))) {
                if ((list_add(table->table[hashValue], tempEntry)) == 0) {
                    table->numElems++;
                    table->numUpdates++;
                }
                else {
                    ERROR("list_add");
                    retVal = -1;
                }
            }
            else {
                ERROR("entry_create");
                retVal = -1;
            }
        }
    }
    else {
        ERROR("NULL table, key or data");
        retVal = -1;
    }
    return retVal;

}

/*
 * Função para obter um elemento da tabela.
 * O argumento key indica a key da entrada da tabela. A função
 * aloca memória para armazenar uma *COPIA* dos dados da tabela e
 * retorna este endereço. O programa a usar essa função deve
 * desalocar (utilizando free()) esta memória.
 * Em caso de erro, devolve NULL.
 */
struct data_t *table_get(struct table_t *table, char *key) {

    struct data_t *tempData = NULL;
    struct entry_t *tempEntry = NULL;
    int hashValue;

    if (key) {
        hashValue = hash(key, table->hashSize);

        // Verifica se a entrada existe na tabela
        if((tempEntry = list_get(table->table[hashValue], key))) {

            // Aloca memória para a entra e copia-a
            if((tempData = data_create(tempEntry->value->datasize))) {

                // Copia a data e verifica se foi bem sucedido
                memcpy(tempData->data, tempEntry->value->data, tempEntry->value->datasize);
                if((memcmp(tempData->data, tempEntry->value->data, tempEntry->value->datasize) != 0)) {
                    ERROR("memcpy tempEntry->value->data");
                    if (tempData) {
                        data_destroy(tempData);
                    }
                    tempData = NULL;
                }
            }
            else {
                ERROR("data_create");
            }
        }
    }
    else {
        ERROR("NULL key");
    }
    return tempData;

}

/*
 * Função para remover um elemento da tabela. Vai desalocar
 * toda a memória alocada na respectiva operação tabel_put().
 * Devolve: 0 (ok), -1 (key not found)
 */
int table_del(struct table_t *table, char *key) {

    int result = -1;
	if(table) {
            result = list_remove(table->table[hash(key,table->hashSize)], key);
	}
    if(result == 0) {
        table->numElems--;
        table->numUpdates++;
    }
    return result;
    
}

/*
 * Devolve número de elementos da tabela.
 */
int table_size(struct table_t *table) {

    return (table ? table->numElems : -1);
    
}

/*
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **table_get_keys(struct table_t *table) {

    char **keys, **tempKeys;
    int elementsInList, listCounter,
        counter = 0, numMallocs = 0, elementsCopied = 0;

    // Verifica a validade do parâmetro, aloca memória para o vector de keys e verifica-o
    if(table && (keys = (char**)malloc(sizeof(char*) * (table->numElems + 1)))) {

        // Percorre todas as listas da table e copia as chaves
        for(counter = 0; counter < table->hashSize && (numMallocs < table->numElems); counter++) {
            if((elementsInList = table->table[counter]->elements) > 0) {

                // Obtém as keys de cada lista da tabela e verifica a operação
                if(!(tempKeys = list_get_keys(table->table[counter]))) {
                    ERROR("list_get_keys");
                    for(counter = 0; counter < numMallocs; counter++) {
                        free(keys[counter]);
                    }
                    counter = table->hashSize;
                    free(keys);
                    keys = NULL;
                } 
                else {
                    // Para cada lista, faz uma cópia de todas as keys
                    for(listCounter = 0; listCounter < elementsInList; listCounter++) {
                        if(tempKeys[listCounter]) {
                            if((keys[numMallocs] = strdup(tempKeys[listCounter]))) {
                                // Ambas variaveis tem o valor igual...
                                elementsCopied ++;
                                numMallocs ++;
                            }
                            else {
                                ERROR("strdup");
                                for(counter = 0; counter < numMallocs; counter++) {
                                    free(keys[counter]);
                                }
                                counter = table->hashSize;
                                free(keys);
                                keys = NULL;
                            }
                        }
                        else {
                            ERROR("NULL pointers were going to strdup!");
                        }
                    }
                    list_free_keys(tempKeys);
                }
            }
        }
        // Confirma a cópia de todas as keys e termina o vector com NULL
        if(keys) {
            keys[table->numElems] = NULL;
        }
    }
    else {
        ERROR("NULL table or malloc");
    }
    return keys;

}

/*
 * Desaloca a memória alocada por table_get_keys()
 */
void table_free_keys(char **keylist) {

    int counter = 0;
    if(keylist && keylist[counter]) {
        while(keylist[counter]) {
            free(keylist[counter]);
            counter ++;
        }
    }
    free(keylist);

}

/*
 * Retorna o número de actualizações realizadas na tabela.
 */
int table_get_num_updates(struct table_t *table) {
    
    return (table ? table->numUpdates : -1);

}
/*
 * Devolve um array de entry_t * com cópias de todas as entries da tabela,
 * e um último elemento a NULL.
 */
struct entry_t **table_get_entries(struct table_t *table) {

    struct entry_t **entries = NULL, **tempEntries = NULL;
    int counter = 0, currentEntry = 0, totalEntries = 0;

    if (table) {
        // Aloca memória para todas as entries e verifica-a
        if ((entries = (struct entry_t **) malloc(
                sizeof(struct entry_t *) * (table->numElems + 1)))) {

            // Percorre a tabela e obtem as entradas de cada lista
            for (counter = 0; counter < table->hashSize; counter++) {
                if ((tempEntries = list_get_entries(table->table[counter]))) {
                    for (currentEntry = 0; currentEntry <
                            table->table[counter]->elements; currentEntry++) {
                        entries[totalEntries] = tempEntries[currentEntry];
                        totalEntries++;
                    }
                    // Iremos desalocar as entradas posteriormente,
                    // apenas precisamos de desalocar o array.
                    free(tempEntries);
                }
                else {
                    ERROR("list_get_entries");
                    for (totalEntries -= 1; totalEntries >= 0; totalEntries--) {
                        free(entries[totalEntries]);
                    }
                    free(entries);
                    entries = NULL;
                }
            }
            // Confirma a cópia de todas as entries e termina o vector com NULL
            if (entries) {
                entries[totalEntries] = NULL;
            }
        }
    }
    return entries;

}

/*
 * Liberta a memória alocada por table_get_entries().
 */
void table_free_entries(struct entry_t **entries) {

    int counter = 0;
    if (entries) {
        while (entries[counter]) {
            entry_destroy(entries[counter]);
            counter++;
        }
        free(entries);
    }
    
}
 
/* 
 * 1,000,000 de strings aleatórias passadas para a tablea por esta função hash
 * com um mod = 8, tendo sido obtidos os seguintes resultados:
 *
 *  0 -> 125,624 elementos
 *  1 -> 125,492    "
 *  2 -> 124,553    "
 *  3 -> 124,694    "
 *  4 -> 125,299    "
 *  5 -> 124,908    "
 *  6 -> 124,403    "
 *  7 -> 125,027    "
 *
 * Não se obtive uma distribuição 100% uniforme (125,000) mas, sendo a margem de
 * erro máxima inferior a 0,5%, pode-se considerar esta de hash satisfatória
 *
 * 31 = http://www.cogs.susx.ac.uk/courses/dats/notes/html/node114.html
 *      http://computinglife.wordpress.com/2008/11/20/why-do-hash-functions-use-prime-numbers/
 */
int hash(char *string, int mod) {

    int length, counter = 0, result = 0, power;
    if(string) {
        length = (int)strlen(string);
        for(counter = 0; counter < length; counter++) {
            power = length - counter - 1;
            result += (int)string[counter] * pow(31, power);
        }
    }
    else {
        ERROR("NULL string");
    }

    // Retorna -1 em caso de erro.
    return (string ? abs(result % mod) : -1);

}

/*
 * Função para obter o timestamp do valor associado a essa chave.
 * Em caso de erro devolve -1. Em caso de chave nao encontrada devolve 0.
 */
long table_get_ts(struct table_t *table, char *key) {

    // Verifica os parâmetros
    if(table == NULL || key == NULL) {
        ERROR("NULL table or key");
        return -1;
    }

    // Procura a entrada na tabela
    struct data_t *tempData;
    if((tempData = table_get(table, key)) == NULL) {
        // Retorna NULL no caso de erro ou de não existir
        return 0;
    }

    // Em caso de sucesso
    long ts = tempData->timestamp;
	printf("Timestamp: %ld\n", ts);
    data_destroy(tempData);
    return ts;

}