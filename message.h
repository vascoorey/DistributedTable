#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "data.h"
#include "entry.h"
#include "message-private.h"

/* Definição do número de digitos do opcode e do c_type */
#define OPCODE_SIZE 2
#define C_TYPE_SIZE 2

/* Define códigos para os possíveis conteúdos da mensagem */
#define CT_ENTRY  10
#define CT_KEY    20
#define CT_KEYS   30
#define CT_VALUE  40
#define CT_RESULT 50
#define CT_TIMESTAMP 60

/* 
 * Estrutura que representa uma mensagem genérica a ser transmitida.
 * Esta mensagem pode ter vários tipos de conteúdos, representados por uma
 * union. 
 */
struct message_t {
	short opcode; /* código da operação na mensagem, usado no projeto 2 */
	short c_type; /* indica qual o tipo do content da mensagem */
	union content_u {
		struct entry_t *entry;
		char *key;
		char **keys;
		struct data_t *value;
		int result;
		long timestamp;
	} content; /* conteúdo da mensagem */
};

/*
 * Transforma uma message_t -> char*, retornando o tamanho do buffer
 * alocado para a mensagem em formato string.
 *
 * c_type	content		mensagem
 * CT_ENTRY	entry		"OC 10 KEY DATA-BASE64"
 * CT_KEY	key		"OC 20 KEY"
 * CT_KEYS	keys		"OC 30 N KEY1 KEY2 ... KEYN"
 * CT_VALUE	value		"OC 40 DATA-BASE64"
 * CT_RESULT	result		"OC 50 RESULT"
 *
 * DATA-BASE64 corresponde a um bloco de dados binários no formato
 * BASE64. Para isso recomenda-se o uso da biblioteca disponível
 * em http://josefsson.org/base-encoding/
 */
int message_to_string(struct message_t *msg, char **msg_str);

/*
 * Transforma um mensagem em string para uma struct message_t*
 */
struct message_t *string_to_message(char *msg_str);

/* 
 * Liberta a memória alocada na função string_to_message
 */
void free_message(struct message_t *message);

#endif

