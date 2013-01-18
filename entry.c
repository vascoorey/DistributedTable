/*
 * File:   entry.c
 *
 * Implementação de struct entry_t
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "utils.h"
#include "entry.h"
#include "data.h"

/* 
 * Cria uma entry com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {

    struct entry_t *entry = NULL;
    if((entry = (struct entry_t*) malloc(sizeof(struct entry_t)))) {
		entry->key = key;
        entry->value = data;
    }
    else {
        ERROR("malloc entry");
    }
    return entry;

}

/*
 * Aloca uma nova memória e copia a entry para ela.
 */
struct entry_t *entry_dup(struct entry_t *entry) {

    struct entry_t *newEntry = NULL;
    
    if (entry) {
        // Aloca memória para uma nova entrada
        if ((newEntry = (struct entry_t*) malloc(sizeof (struct entry_t)))) {
            // Copia a chave
            if ((newEntry->key = strdup(entry->key))) {
                // Aloca memória para a nova data e copia-a
                if (!(newEntry->value = data_dup(entry->value))) {
                    ERROR("data_dup");
                    free(newEntry->key);
                    free(newEntry);
                    newEntry = NULL;
                }
            }
            else {
                ERROR("strdup key");
                free(newEntry);
                newEntry = NULL;
            }
        }
        else {
            ERROR("malloc newEntry");
        }
    }
    else {
        ERROR("NULL entry");
    }
    return newEntry;

}

/*
 * Liberta toda memória alocada pela entry.
 */
void entry_destroy(struct entry_t *entry) {

    if (entry) {
        if (entry->value) {
            data_destroy(entry->value);
        }
        if (entry->key) {
            free(entry->key);
        }
        free(entry);
    }
    
}