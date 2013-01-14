/*
 * File:   message.c
 *
 * Implementação de funções que trabalham sobre mensagens.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */

#include "utils.h"
#include "base64.h"
#include "list.h"

#include "remote_table.h"
#include "message.h"
#include "message-private.h"

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
int message_to_string(struct message_t *msg, char **msg_str) {
	int messageLength = -1; // Tamanho base "## ## "
	if(msg && msg_str) {
		// Tamanho minimo da string: OPCODE_SIZE + C_TYPE_SIZE + 2 (espaços) + tamanho dados a enviar em string
		switch (msg->c_type) {
			case CT_ENTRY:
				if((messageLength = entry_to_string(msg->opcode, msg->content.entry, msg_str)) == -1) {
					ERROR("entry_to_string");
				}
				break;
			case CT_KEY:
				if((messageLength = key_to_string(msg->opcode, msg->content.key, msg_str)) == -1) {
					ERROR("key_to_string");
				}
				break;
			case CT_KEYS:
				if((messageLength = keys_to_string(msg->opcode, msg->content.keys, msg_str)) == -1) {
					ERROR("keys_to_string");
				}
				break;
			case CT_VALUE:
				if((messageLength = value_to_string(msg->opcode, msg->content.value, msg_str)) == -1) {
					ERROR("value_to_string");
				}
				break;
			case CT_RESULT:
				if((messageLength = result_to_string(msg->opcode, msg->content.result, msg_str)) == -1) {
					ERROR("result_to_string");
				}
				break;
			case CT_TIMESTAMP:
				if((messageLength = timestamp_to_string(msg->opcode, msg->content.timestamp, msg_str)) == -1) {
					ERROR("timestamp_to_string");
				}
				break;
			default:
				ERROR("c_type errado");
				break;
		}
	} else {
		ERROR("NULL msg");
	}
	//printf("M: %s\n", *msg_str);
	return (int)strlen(*msg_str);
}

/*
 * Transforma um mensagem em string para uma struct message_t*
 */
struct message_t *string_to_message(char *msg_str) {
	char *tempStr;
	struct message_t *message = NULL;
	if(msg_str && (message = (struct message_t*)malloc(sizeof(struct message_t)))) {
		if(sscanf(msg_str, "%hd %hd", &message->opcode, &message->c_type) == 2) {
			if(((int)floor(log10((double)message->opcode)) + 1) == 2 && ((int)floor(log10((double)message->c_type)) + 1) == 2) {
				//printf("M: %s\n", msg_str);
				tempStr = msg_str;
				tempStr += 6;
				switch (message->c_type) {
					case CT_ENTRY:
						if(!(message->content.entry = string_to_entry(tempStr))) {
							ERROR("string_to_entry");
							free(message);
							message = NULL;
						}
						break;
					case CT_KEY:
						if(!(message->content.key = string_to_key(tempStr))) {
							ERROR("string_to_key");
							free(message);
							message = NULL;
						}
						break;
					case CT_KEYS:
						if(!(message->content.keys = string_to_keys(tempStr))) {
							ERROR("string_to_keys");
							free(message);
							message = NULL;
						}
						break;
					case CT_VALUE:
						if(!(message->content.value = string_to_value(tempStr))) {
							ERROR("string_to_value");
							free(message);
							message = NULL;
						}
						break;
					case CT_RESULT:
						if(sscanf(tempStr, "%d", &message->content.result) != 1) {
							ERROR("sscanf");
							free(message);
							message = NULL;
						}
						break;
					case CT_TIMESTAMP:
						if((message->content.timestamp = string_to_timestamp(tempStr)) == -1) {
							ERROR("string_to_timestamp");
							free(message);
							message = NULL;
						}
						break;
				}
			} else {
				ERROR("String invalida");
				free(message);
				message = NULL;
			}
		} else {
			ERROR("sscanf");
			free(message);
			message = NULL;
		}
	} else {
		ERROR("malloc: message ou msg_str NULL");
		message = NULL;
	}
	return message;
}

/* 
 * Liberta a memória alocada na função string_to_message
 */
void free_message(struct message_t *message) {
	int counter;
	if(message) {
		switch (message->c_type) {
			case CT_KEY:
				if(message->content.key) {
					free(message->content.key);
				}
				break;
			case CT_KEYS:
				counter = 0;
				if(message->content.keys) {
					while(message->content.keys[counter]) {
						free(message->content.keys[counter]);
						counter ++;
					}
				}
				free(message->content.keys);
				break;
			case CT_ENTRY:
				if(message->content.entry) {
					entry_destroy(message->content.entry);
				}
				break;
			case CT_VALUE:
				if(message->content.value) {
					data_destroy(message->content.value);
				}
				break;
			default:
				break;
		}
		free(message);
	}
}

/*
 * Funções de ajuda para o message_to_string
 */

/*
 * Converte um struct entry_t numa mensagem com o seguinte formato:
 *	"OPCODE C_TYPE key DATA-BASE64"
 *	Imprime a mensagem para o **msg_str passado.
 *	Retorna o tamanho da string impressa ou -1 em caso de erro.
 */
int entry_to_string(short opcode, struct entry_t *entry, char **msg_str) {
	int messageLength = 6;
	char *tempString, *ts;
	size_t encodedSize;
		
	// Qual o tamanho da entry (key + data (em base 64))?
	// String = "10 10 key data_base64"
	messageLength += strlen(entry->key) + 1; // 1 = extra space.
	encodedSize = base64_encode_alloc(entry->value->data, entry->value->datasize, &tempString);
	messageLength += strlen(tempString) + 1; // tb pode ser encoded_size +1
	encode_timestamp(entry->value->timestamp, &encodedSize, &ts);
	messageLength += strlen(ts) + 1;
	if(!ts) {
		ERROR("encode_timestamp");
		free(tempString);
		return -1;
	}
	// Finalmente, criamos a string.
	if(encodedSize > 0 && (*msg_str = (char*)malloc(messageLength + strlen(ts) + 1))) {
		sprintf(*msg_str, "%hd %hd %s %s %s", opcode, (short)CT_ENTRY, ts, entry->key, tempString);
		//printf("A enviar: %s\n", *msg_str);
		free(tempString);
		free(ts);
	} else {
		ERROR("malloc ou base64_encode_alloc");
		if(encodedSize > 0) {
			free(*msg_str);
		}
		messageLength = -1;
	}
	return messageLength;
}

/*
 * Converte um struct data_t numa mensagem com o seguinte formato:
 *	"OPCODE C_TYPE DATA-BASE64"
 *	Imprime a mensagem para o **msg_str passado.
 *  Retorna o tamanho da string impressa ou -1 em caso de erro.
 */
int value_to_string(short opcode, struct data_t *value, char **msg_str) {
	int messageLength = 6; /* 6 = 4 digitos (OPCODE + C_TYPE) + 2 espaços */
	char *tempString, *ts;
	size_t encodedSize;
	// Qual o tamanho dos dados (em base 64)?
	if(value && value->data) {
		encodedSize = base64_encode_alloc(value->data, value->datasize, &tempString);
		if(encodedSize) {
			messageLength += strlen(tempString) + 1;
		} else {
			ERROR("base_64_encode_alloc");
			messageLength = -1;
		}
	} else if (value) {
		// Temos um data NULL, passamos uma string "## ## 0(base64)"
		if((encodedSize = base64_encode_alloc("0", 1, &tempString))) {
			messageLength += strlen(tempString) + 1;
		} else {
			ERROR("base_64_encode_alloc");
			messageLength = -1;
		}
	} else {
		ERROR("NULL data");
		return -1;
	}
	
	encode_timestamp(value->timestamp, &encodedSize, &ts);
	if(encodedSize <= 0) {
		ERROR("encode_timestamp");
		free(tempString);
		return -1;
	}
	messageLength += strlen(ts) + 1;
	
	if(messageLength > 0 && (*msg_str = malloc(messageLength + strlen(ts) + 1))) {
		sprintf(*msg_str, "%hd %hd %s %s", opcode, (short)CT_VALUE, ts, tempString);
	} else {
		ERROR("malloc");
		messageLength = -1;
	}
	free(tempString);
	free(ts);
	
	return messageLength;
}

/*
 * Converte um array de keys numa mensagem com o seguinte formato:
 *	"OPCODE C_TYPE N_KEYS KEY1 .. KEYN"
 *	Imprime a mensagem para o **msg_str passado.
 *	Retorna o tamanho da string impressa ou -1 em caso de erro.
 */
int keys_to_string(short opcode, char **keys, char **msg_str) {
	int messageLength = 7, /* 7 = 4 digitos (OPCODE + C_TYPE) + 3 espaços */
		counter = 0,
		totalLength = 0,
		numDigits = 0;
	char *keysString = NULL, *tempString;

	if(keys) {
		while(keys[counter]) {
			if(!keysString && (keysString = (char*)malloc(strlen(keys[counter]) + 1))) {
				sscanf(keys[counter], "%s", keysString);
				messageLength += (int)strlen(keys[counter]) + 1;
			} else if((tempString = (char*)realloc(keysString, strlen(keysString) + strlen(keys[counter]) + 2))) {
				keysString = tempString;
				sprintf(keysString, "%s %s", keysString, keys[counter]);
				messageLength += (int)strlen(keys[counter]) + 2;
			} else {
				ERROR("malloc ou realloc");
				if(keysString) {
					free(keysString);
					messageLength = -1;
				}
			}
			counter ++;
		}
		// Isto não fica bonito mas como não consegui resolver o problema de estarmos a ficar com
		//	uninitialized bytes vamos só rever o messageLength para garantir que não os tentamos
		//	aceder.
		if(counter > 0) {
			totalLength = messageLength - 7;
			messageLength -= (totalLength - strlen(keysString));
			messageLength ++;
		}
		//printf("total: %d, strlen: %d, '%s'\n", totalLength, strlen(keysString), keysString);
		if(counter == 0) {
			// Não existem keys
			if((*msg_str = (char*)malloc(messageLength + 2))) {
				sprintf(*msg_str, "%hd %hd %d", opcode, (short)CT_KEYS, counter);
			} else {
				messageLength = -1;
			}
		} else {
			numDigits = (int)(floor(log10((double)counter))) + 1;
			if((*msg_str = (char*)malloc(messageLength + numDigits + 1))) {
				sprintf(*msg_str, "%hd %hd %d %s", opcode, (short)CT_KEYS, counter, keysString);
				free(keysString);
			} else {
				ERROR("malloc");
				if(keysString) {
					free(keysString);
					messageLength = -1;
				}
			}
		}
		// Acho que de vez em quando estamos a acusar um buffer maior que o real... Double check !
		if(!messageLength == strlen(*msg_str) + 1) {
			ERROR("messageLength!");
		}
	} else {
		ERROR("NULL keys");
		messageLength = -1;
	}
	return messageLength;
}

/*
 * Converte uma key numa mensagem com o seguinte formato:
 *  "OPCODE C_TYPE KEY"
 *	Imprime a mensagem para o **msg_str passado.
 *  Retorna o tamanho da string impressa ou -1 em caso de erro.
 */
int key_to_string(short opcode, char *key, char **msg_str) {
	// Qual o tamanho da key?
	int messageLength = 7 + (int)strlen(key); // '\0'
	if((*msg_str = (char*)malloc(messageLength))) {
		sprintf(*msg_str, "%hd %hd %s", opcode, (short)CT_KEY, key);
	} else {
		ERROR("malloc");
		messageLength = -1;
	}
	return messageLength;
}

/*
 * Converte um resultado numa mensagem com o seguinte formato:
 *  "OPCODE C_TYPE RESULT"
 *	Imprime a mensagem para o **msg_str passado.
 *	Retorna o tamanho da string impressa ou -1 em caso de erro.
 */
int result_to_string(short opcode, int result, char **msg_str) {
	int messageLength = 6, numDigits;
	// Qual o tamanho do resultado?
	if(abs(result)) {
		numDigits = (int)(floor(log10((double)abs(result)))) + 1;
	} else {
		numDigits = 1;
	}
	messageLength += numDigits + (result < 0 ? 2 : 1);
	if((*msg_str = (char*)malloc(messageLength))) {
		sprintf(*msg_str, "%hd %hd %d", opcode, (short)CT_RESULT, result);
	} else {
		ERROR("malloc");
		messageLength = -1;
	}
	return messageLength;
}

int timestamp_to_string(short opcode, long timestamp, char **msg_str) {
	int messageLength = 6;
	size_t encodedSize;
	char *ts;
	
	encode_timestamp(timestamp, &encodedSize, &ts);
	if(encodedSize == -1) {
		ERROR("timestamp_string");
		return -1;
	}
	
	if((*msg_str = malloc(messageLength + encodedSize + 2))) {
		sprintf(*msg_str, "%hd %hd %s", opcode, (short)CT_TIMESTAMP, ts);
		messageLength += (int)strlen(*msg_str);
	} else {
		ERROR("malloc");
		messageLength = -1;
	}
	free(ts);
	
	return (int)strlen(*msg_str);
}

void encode_timestamp(long timestamp, size_t *encoded_size, char **out_string) {
	int numDigits;
	char *ts;
	
	if(timestamp < 0) {
		ERROR("Invalid timestamp");
		*encoded_size = -1;
		*out_string = NULL;
		return;
	}
	
	numDigits = (timestamp == 0 ? 1 : (int)(floor(log10((double)timestamp))) + 1);
	if(!(ts = malloc(numDigits + 1))) {
		ERROR("malloc");
		*encoded_size = -1;
		*out_string = NULL;
		return;
	}
	sprintf(ts, "%ld", timestamp);
	
	*encoded_size = base64_encode_alloc(ts, strlen(ts), out_string);
	free(ts);
}

/*
 * Funções de ajuda para o string_to_message
 */

/*
 * Converte uma string num struct entry_t
 *	Retorna o entry_t se correu tudo bem ou NULL em caso de erro.
 */
struct entry_t *string_to_entry(char *msg_str) {
	int counter = 0, isDel = 0;
	char *key = NULL, *encodedString = NULL, *decodedString = NULL, *restOfTheString = NULL, *ts = NULL, *decodedTs;
	size_t decodedSize, tsSize;
	struct entry_t *entry = NULL;
	
	if((key = (char*)malloc(strlen(msg_str))) && (encodedString = (char*)malloc(strlen(msg_str))) &&
	   (ts = (char*)malloc(strlen(msg_str)))) {
		//printf("mensagem: %s\n", msg_str);
		if(sscanf(msg_str, "%s %s %s", ts, key, encodedString) == 2) {
			// é um del, encodedString = 0?
			isDel = 1;
			decodedSize = 0;
		}
		//printf("decomposta, ts: %s, key: %s, data: %s\n", ts, key, encodedString);
		if((isDel || base64_decode_alloc(encodedString, strlen(encodedString), &decodedString, &decodedSize)) &&
		   base64_decode_alloc(ts, strlen(ts), &decodedTs, &tsSize)) {
			// Criar o entry
			if(!(entry = entry_create(key, data_create2((int)decodedSize, decodedString)))) {
				ERROR("entry_create ou data_create2");
			} else {
				entry->value->timestamp = atol(decodedTs);
			}
			free(decodedTs);
		} else {
			ERROR("base64_decode_alloc");
		}
	} else {
		ERROR("malloc");
	}
	if(!entry) {
		if(decodedString) {
			free(decodedString);
		}
		if(restOfTheString) {
			free(restOfTheString);
		}
		if(entry) {
			entry_destroy(entry);
		}
	}
	if(encodedString) {
		free(encodedString);
	}
	if(ts) {
		free(ts);
	}
	return entry;
}

/*
 * Converte uma string numa key.
 *	Retorna NULL em caso de erro.
 */
char *string_to_key(char *msg_str) {
	int keyLength;
	char *key = NULL;
	// msg_str = "## ## %s", len(s) = len(msg_str) - 6 + 1 (\0)
	keyLength = (int)strlen(msg_str) + 1;
	if((key = (char*)malloc(keyLength))) {
		//printf("A ler a chave: %s\n", msg_str);
		sscanf(msg_str, "%s", key);
		/*	ERROR("sscanf");
			free(key);
			key = NULL;
		}*/
	} else {
		ERROR("malloc");
	}
	return key;
}

/*
 * Converte uma string num array de keys com o ultimo elemento a NULL para
 *	delimitar o fim do array.
 *	Retorna NULL em caso de erro.
 */
char **string_to_keys(char *msg_str) {
	int numKeys, numDigits, counter;
	char **keys = NULL, *key, *workString, *restOfTheString;
	
	if(sscanf(msg_str, "%d", &numKeys) == 1) {
		if(numKeys > 0) {
			numDigits = (int)floor(log10((double)numKeys)) + 1;
			msg_str += 1 + numDigits; // "n# "
			if((keys = (char**)malloc(sizeof(char*) * (numKeys + 1)))) {
				counter = 0;
				if((workString = strdup(msg_str))) {
					for(key = strtok_r(workString, " \0", &restOfTheString); key; key = strtok_r(NULL, " \0", &restOfTheString)) {
						if((keys[counter] = strdup(key))) {
							counter ++;
						} else {
							ERROR("strdup");
							while(counter) {
								free(keys[counter - 1]);
								counter --;
							}
							free(keys);
							keys = NULL;
						}
					}
				} else {
					ERROR("strdup");
					free(keys);
					keys = NULL;
				}
				free(workString);
				// A ultima entrada do array keys tem que ser NULL de modo a podermos trabalhar com ele no futuro.
				restOfTheString = NULL;
				keys[numKeys] = NULL;
			} else {
				ERROR("malloc");
			}
		} else {
			// 0 keys
			if((keys = (char**)malloc(sizeof(char*)))) {
				keys[0] = NULL;
			} else {
				ERROR("malloc");
				keys = NULL;
			}
		}
	} else {
		ERROR("sscanf");
	}
	return keys;
}

/*
 * Converte uma string num struct data_t
 *	Retorna o data_t se correu tudo bem ou NULL em caso de erro.
 */
// TODO: a string agora tem um TS e os DADOS
struct data_t *string_to_value(char *msg_str) {
	char *decodedString = NULL, *encoded, *rest, *workString;
	size_t decodedSize;
	struct data_t *value = NULL;
	
	if(!(workString = strdup(msg_str))) {
		ERROR("strdup");
		return NULL;
	}
	
	//printf("Data: %s\n", msg_str);
	
	encoded = strdup(strtok_r(workString, " ", &rest));
	if(encoded) {
		if(!(value = data_create(0))) {
			ERROR("data_create");
			free(workString);
			return NULL;
		}
		if((value->timestamp = string_to_timestamp(encoded)) == -1) {
			ERROR("string_to_timestamp");
			free(workString);
			return NULL;
		} else {
			printf("Timestamp: %ld\n", value->timestamp);
		}
		free(encoded);
	} else {
		ERROR("base64_decode_alloc");
		free(workString);
		return NULL;
	}
	
	encoded = strdup(strtok_r(NULL, "\0", &rest));
	//printf("Decoding: %s\n", encoded);
	base64_decode_alloc(encoded, strlen(encoded), &decodedString, &decodedSize);
	if(decodedString) {
		if(decodedSize > 1) {
			value->data = decodedString;
			value->datasize = (int)strlen(decodedString);
		}
		free(workString);
		free(encoded);
	} else {
		ERROR("base64_decode_alloc");
		data_destroy(value);
		value = NULL;
	}
	
	return value;
}

long string_to_timestamp(char *msg_str) {
	char *decodedTs = NULL;
	size_t decodedSize = 0;
	long ret = -1;
	if(*msg_str) {
		//printf("A descodificar: %s --> ", msg_str);
		base64_decode_alloc(msg_str, strlen(msg_str), &decodedTs, &decodedSize);
		if(decodedTs) {
			ret = atol(decodedTs);
			printf("%ld\n", ret);
			free(decodedTs);
		}
	} else {
		ERROR("string_to_timestamp: null string");
	}
	return ret;
}