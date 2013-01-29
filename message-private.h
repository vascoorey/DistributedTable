/*
 * File:   mensagem-private.h
 * 
 * Define varias funcoes auxiliares para trabalhar com mensagens.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include <sys/types.h>

#ifndef MESSAGE_PRIVATE_H
#define	MESSAGE_PRIVATE_H

struct data_t;
struct entry_t;

/*
 * Funções de ajuda para o message_to_string
 */
int entry_to_string(short opcode, struct entry_t *entry, char **msg_str);
int value_to_string(short opcode, struct data_t *value, char **msg_str);
int keys_to_string(short opcode, char **keys, char **msg_str);
int key_to_string(short opcode, char *key, char **msg_str);
int result_to_string(short opcode, int result, char **msg_str);
int timestamp_to_string(short opcode, long timestamp, char **msg_str);

/*
 * Funções de ajuda para o string_to_message
 */
struct entry_t *string_to_entry(char *msg_str);
char *string_to_key(char *msg_str);
char **string_to_keys(char *msg_str);
struct data_t *string_to_value(char *msg_str);
long string_to_timestamp(char *msg_str);
void encode_timestamp(long timestamp, size_t *encoded_size, char **out_string);

//void encode_timestamp(long timestamp, size_t *encoded_size, char **out_string);

#endif