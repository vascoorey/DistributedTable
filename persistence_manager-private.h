/* 
 * File:   persistence_manager-private.h
 *
 * Define a estrutura de um gestor de persistência e algumas funções a
 * serem usadas neste módulo.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
#ifndef _PERSISTENCE_MANAGER_PRIVATE_H
#define _PERSISTENCE_MANAGER_PRIVATE_H

#include <stdio.h>
#include "message.h"
#include "table-private.h"

#define PERMISSIONS 0666

/*
 * Define a estrutura de um gestor de persistência.
 *
 * char *ckp_name => nome do ficheiro checkpoint
 * int ckp_fd => descritor do ficheiro checkpoint
 * 
 * char *stt_name => nome do ficheiro temporário
 * int stt_fd => descritor do ficheiro temporário
 *
 * char *log_name => nome do ficheiro log
 * int log_fd => descritor do ficheiro log
 *
 * int create_flags => flag do tipo de escrita
 *
 * int max_log_size => tamanho máximo do logfile
 * int current_log_size => tamanho corrente do logfile
 */
struct pmanager_t {
	char *ckp_name;
	int ckp_fd;

	char *stt_name;
	int stt_fd;

	char *log_name;
	int log_fd;

	int create_flags;

	int max_log_size;
	int current_log_size;
};

/*
 * Determina tamanho do ficheiro referenciado por fd.
 */
int file_size(int fd);

/*
 * Preenche a tabela com a informação contida no ficheiro.
 */
int table_fill(int fd, struct table_t *table);

/*
 * Percorre o ficheiro log linha a linha.
 */
int execute_log(int fd, struct table_t *table);

/*
 * Interpreta cada linha do ficheiro log
 */
int run_line(char *line, struct table_t *table);

int recover_from_ckp_log(struct pmanager_t *pmanager, struct table_t *table);

#endif