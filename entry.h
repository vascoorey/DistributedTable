#ifndef _ENTRY_H
#define _ENTRY_H

#include "data.h"

/* Define o par chave-valor para a tabela. */
struct entry_t {
    char *key;              /* String (char* terminado por '\0') */
    struct data_t *value;   /* Bloco de dados */
};

/*
 * Cria uma entry com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data);

/* 
 * Aloca uma nova memória e copia a entry para ela.
 */
struct entry_t *entry_dup(struct entry_t *entry);

/*
 * Liberta toda memória alocada pela entry.
 */
void entry_destroy(struct entry_t *entry);

#endif

