#ifndef _DATA_H
#define _DATA_H

struct data_t {
	int datasize;	/* Tamanho do bloco de dados do data */
	void *data;	/* Conteúdo arbitrário */
	long timestamp; /* Contador + o id do processo cliente */
};

/* 
 * Um construtor que cria o data_t e aloca a quantidade de memória requisitada
 * no parâmetro size para data.
 */
struct data_t *data_create(int size);

/* 
 * Um construtor que cria o data_t com o data e o datasize passados como
 * parâmetros (i.e., o data da struct vai apontar para o parâmetro data).
 */
struct data_t *data_create2(int datasize, void *data);

/*
 * Cria uma *cópia* do data_t passado (incluindo o data->data).
 */
struct data_t *data_dup(struct data_t *data);

/* 
 * Liberta a memória de um data_t (incluindo data->data).
 */
void data_destroy(struct data_t *data);

#endif