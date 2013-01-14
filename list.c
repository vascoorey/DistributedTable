/*
 * File:   lista.c
 *
 * Define a classe que cria listas duplamente ligadas
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#include "list.h"
#include "list-private.h"

/*
 * Cria uma nova lista. Em caso de erro, retorna NULL.
 */
struct list_t *list_create() {

    struct list_t *newList = NULL;
    if((newList = (struct list_t*)malloc(sizeof(struct list_t)))) {
        newList->elements = 0;
        newList->head = NULL;
        newList->tail = NULL;
    }
    else {
        ERROR("Malloc newList");
    }
    return newList;

}

/*
 * Eliminar uma lista. Desaloca *toda* a memória utilizada pela lista.
 * Retorna 0 (OK) ou -1 (erro).
 */
void list_destroy(struct list_t *list) {

    struct node_t *tempNode;
    
    if(list) {
        //enquanto existir um nó seguinte...
        while(list->head) {
            tempNode = list->head;        //aponta para a cabeça da lista
            list->head = list->head->next;//move a cabeça da lista para a frente
            node_destroy(tempNode);       //destroi o nó
        }
        free(list);
    }
    else {
        ERROR("Lista NULL");
    }

}

/*
 * Adicionar um elemento (no final da lista). Retorna 0 (OK) ou -1 (erro).
 */
int list_add(struct list_t *list, struct entry_t *entry) {

    int retVal = -1;
    struct node_t *newNode;

    if(list) {
        newNode = node_create(entry);
        if(newNode) {
            //caso a lista esteja vazia
            if(list->elements == 0) {
                list->head = newNode;
                list->tail = newNode;
            }
            else {
                list->tail->next = newNode;
                newNode->prev = list->tail;
                list->tail = newNode;
            }
            list->tail->next = NULL;
            list->elements ++;
            retVal = 0;
        }
        else {
            ERROR("node_create");
        }
    }
    else {
        ERROR("NULL list");
    }
    return retVal;

}

/*
 * Elimina da lista um elemento com a chave key (string). Retorna 0 (OK) ou
 * -1 (erro/não encontrado)
 */
int list_remove(struct list_t *list, char *key) {

    struct node_t *tempNode;
    int found = 0;

    if(list) {
        tempNode = list->head;

        //corre a lista até ao fim ou até ter encontrado a entrada
        while(!found && tempNode) {
            if(strcmp(tempNode->entry->key, key) == 0) {
                found = 1;
            }
            else {
                tempNode = tempNode->next;
            }
        }
        
        //mudar referências da lista...
        if(found) {
            //quando o nó a apagar é a cabeça da lista
            if(tempNode == list->head) {
                list->head = tempNode->next;
            }
            //quando o nó a apagar é a cauda da lista
            else if(tempNode == list->tail) {
                list->tail = tempNode->prev;
                tempNode->prev->next = NULL;
            }
            else {
                tempNode->prev->next = tempNode->next;
                if(tempNode->next) {
                    tempNode->next->prev = tempNode->prev;
                }
            }
            node_destroy(tempNode);
            list->elements --;
        }
    }
    return (found ? 0 : -1);
    
}

/*
 * Obtem um elemento da lista com a chave key (não uma cópia). Retorna a referência
 * do elemento na lista (ou seja, uma alteração implica alterar o elemento
 * na lista; a função list_remove ou list_destroy vai desalocar a memória
 * ocupada pelo elemento, ou NULL em caso de erro).
 */
struct entry_t *list_get(struct list_t *list, char *key) {

    struct node_t *tempNode;
    struct entry_t *tempEntry = NULL;
    int found = 0;

    if(list && key) {
        tempNode = list->head;
 
        //percorre a lista
        while(!found && tempNode && tempNode->entry && tempNode->entry->key) {
            if(tempNode->entry->key && strcmp(tempNode->entry->key, key) == 0) {
                found = 1;
                tempEntry = tempNode->entry;
            }
            else {
                tempNode = tempNode->next;
            }
        }
    }
    else {
        ERROR("Lista ou key NULL");
    }
    return tempEntry;

}

/* 
 * Retorna o tamanho (número de elementos) da lista (-1: erro).
 */
int list_size(struct list_t *list) {

    return (list && list->elements >= 0 ? list->elements : -1);

}

/* 
 * Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **list_get_keys(struct list_t *list) {

    char **keys = NULL;
    struct node_t *tempNode;
    int numMallocs = 0, counter = 0;

    if(list) {
        tempNode = list->head;

        // Aloca memória para guardar todas as chaves da tabela
        if((keys = (char**)malloc((list->elements + 1)*sizeof(char*)))) {

            // Percorre a tabela
            while(tempNode) {

                // Aloca memória para cada chave e duplica a mesma
                if((keys[numMallocs] = strdup(tempNode->entry->key))) {
                    numMallocs ++;
                    tempNode = tempNode->next;
                }
                else {
                    // Deu erro, temos que libertar tudo o que ja foi allocado !
                    ERROR("Malloc");
                    tempNode = NULL;
                    for(counter = 0; counter < numMallocs; counter ++) {
                        free(keys[counter]);
                    }
                    free(keys);
                    keys = NULL;
                    numMallocs = 0;
                }
            }

            // Confirma a cópia de todas as chaves e termina o vector com NULL
            if(keys && numMallocs == list->elements) {
                keys[numMallocs] = NULL;
            }
        }
        else {
            ERROR("Malloc keys");
            keys = NULL;
        }
    }
    else {
        ERROR("NULL Lista");
    }
    return keys;

}

/* 
 * Desaloca a memória alocada por list_get_keys
 */
void list_free_keys(char **keys) {

    int counter = 0;
    while(keys[counter]) {
        free(keys[counter]);
        counter++;
    }
    free(keys);

}

/*
 * Retorna todas as entries dessa lista.
 */
struct entry_t **list_get_entries(struct list_t *list) {

    struct entry_t **entries = NULL;
    struct node_t *node = NULL;
    int counter = 0;

    if (list) {
        // Aloca memória para guardar todas as chaves da tabela
        if ((entries = (struct entry_t **) malloc(
                sizeof (struct entry_t *) * (list->elements + 1)))) {
            
            node = list->head;
            // Percorre a tabela
            while (node) {

                // Aloca memória para cada entrada e copia-a
                if (!(entries[counter] = entry_dup(node->entry))) {
                    for (counter -= 1; counter >= 0; counter--) {
                        free(entries[counter]);
                    }
                    free(entries);
                    entries = NULL;
                    break;
                }
                else {
                    node = node->next;
                    counter++;
                }
            }
            // Confirma a cópia de todas as entradas e termina a NULL
            if (entries) {
                entries[counter] = NULL;
            }
        }
    }
    return entries;

}

/*
 * Métodos auxiliares.
 */

/*
 * Cria e devolve um novo nó e mantem a referência da entry passada.
 * Devolve NULL em caso de erro.
 */
struct node_t *node_create(struct entry_t *entry) {

    struct node_t *node = NULL;
    if(entry && (node = (struct node_t*) malloc(sizeof(struct node_t)))) {
        node->entry = entry;
        node->prev = NULL;
        node->next = NULL;
    }
    else {
        ERROR("Malloc node or NULL entry");
    }
    return node;

}

/*
 * Destroí o nó passado e desaloca toda a sua memória.
 */
void node_destroy(struct node_t *node) {

    if(node) {
        if(node->entry) {
            entry_destroy(node->entry);
        }
        free(node);
    }

}