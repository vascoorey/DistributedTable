/*
 * File:   data.c
 *
 * Implementação da struct data_t
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "utils.h"
#include "data.h"

/*
 * Um construtor que cria o data_t e aloca a quantidade de memória requisitada
 * no parâmetro size para data.
 */
struct data_t *data_create(int size) {

    struct data_t *new_data = NULL;
    
    if(!(new_data = malloc(sizeof(struct data_t)))) {
        ERROR("Malloc new_data");
    }
    else if (size > 0) {
        if(!(new_data->data = (char*)malloc(size))) {
            ERROR("Malloc new_data->data");
            //Como não conseguimos allocar o tamanho para os dados libertamos
            //tudo o que allocamos
            free(new_data);
            new_data = NULL; // Just in case...
        }
        else {
            new_data->datasize = size;
            //Inicializar a zona de memória a 0's para não termos reads invalidos ;)
            memset(new_data->data,0,size);
            new_data->timestamp = 0;
        }
    }
    //No caso da data ser NULL
    else if (!size) {
		if(!(new_data->data = malloc(1))) {
			ERROR("malloc");
			return NULL;
		}
        memcpy(new_data->data, "0", 1);
		new_data->datasize = 1;
		new_data->timestamp = 0;
    }
    else {
        ERROR("data_create");
        free(new_data);
        new_data = NULL;
    }
    
    //Se tudo OK então retorna um apontador válido, se não retorna NULL.
    //Retorna NULL se deu erro. Iremos usar o *data, logo fazemos free dele no
    //data_destroy
    return new_data;

}

/*
 * Um construtor que cria o data_t com o data e o datasize passados como
 * parâmetros (i.e., o data da struct vai apontar para o parâmetro data).
 */
struct data_t *data_create2(int datasize, void *data) {

    struct data_t *newData = NULL;

    //No caso da data ser NULL
    if(!datasize) {
        return data_create(0);
    }
    else if((newData = (struct data_t*)malloc(sizeof(struct data_t)))) {
        newData->datasize = datasize;
        newData->data = data;
        newData->timestamp = 0;
    }
    else {
        ERROR("malloc newData");
    }
    return newData;

}

/*
 * Cria uma *cópia* do data_t passado (incluindo o data->data).
 */
struct data_t *data_dup(struct data_t *data) {
    
    struct data_t *dup = NULL;

    if(data) {
        if(!(dup = data_create(data->datasize))) {
            ERROR("data_create");
        }
        //Verifica se o data está a null
        else if(data->data) {
            memcpy(dup->data, data->data, data->datasize);
            if((memcmp(dup->data, data->data, data->datasize)) != 0) {
                ERROR("memcpy");
                data_destroy(dup);
            }
            dup->timestamp = data->timestamp;
        }
        else {
            //Este data->data = NULL então fazemos o mesmo para o dup.
            dup->data = NULL;
            dup->timestamp = data->timestamp;
        }
    }
    else {
        ERROR("NULL data");
    }
    return dup;

}

/*
 * Liberta a memória de um data_t (incluindo data->data).
 */
void data_destroy(struct data_t *data) {
	if(data) {
		if(data->data)
			free(data->data);
		free(data);
		data = NULL;
    }
}
