/* 
 * File:   persistent_table.c
 *
 * Implementação de funções que trabalham sobre um gestor de 
 * persistência.
 *
 * Author: sd001 > Bruno Neves, n.º 31614
 *               > Vasco Orey,  n.º 32550
 */
 
#include "persistence_manager.h"
#include "persistence_manager-private.h"
#include "base64.h"
#include "data.h"
#include "entry.h"
#include "utils.h"
#include "table.h"
#include "remote_table.h"

/* 
 * Cria um gestor de persistência que armazena logs em filename+".log" e o
 * estado do sistema em filename+".ckp". O parâmetro logsize define o tamanho
 * máximo em bytes que o ficheiro de log pode ter enquanto mode define o modo
 * de escrita dos logs (MODE_ASYNC, MODE_FSYNC e MODE_DSYNC).
 * Retorna o pmanager criado ou NULL em caso de erro.
 */
struct pmanager_t *pmanager_create(char *filename, int logsize, int mode) {
	struct pmanager_t *ret = NULL;
	char *name;
	
	if(filename && logsize > 0 && (ret = (struct pmanager_t *)malloc(sizeof(struct pmanager_t)))) {
		if(mode >= 0 && mode <= 2) {
			ret->max_log_size = logsize;
			ret->stt_fd = -1;
			ret->ckp_fd = -1;
			ret->log_fd = -1;
			// Criar os nomes que vamos usar para os 3 ficheiros.
			if((name = (char*)malloc(strlen(filename) + 5))) {
				sprintf(name, "%s.stt", filename);
				if(!(ret->stt_name = strdup(name))) {
					ERROR("strdup");
					free(ret);
					return NULL;
				}
				sprintf(name, "%s.ckp", filename);
				if(!(ret->ckp_name = strdup(name))) {
					ERROR("strdup");
					free(ret->stt_name);
					free(ret);
					return NULL;
				}
				sprintf(name, "%s.log", filename);
				if(!(ret->log_name = strdup(name))) {
					ERROR("strdup");
					free(ret->stt_name);
					free(ret->ckp_name);
					free(ret);
					return NULL;
				}
				free(name);
				
				switch(mode) {
					case MODE_ASYNC:
						ret->create_flags = O_WRONLY | O_CREAT | O_EXCL;
						break;
					case MODE_DSYNC:
						ret->create_flags = O_WRONLY | O_CREAT | O_EXCL | O_DSYNC;
						break;
					case MODE_FSYNC:
						ret->create_flags = O_WRONLY | O_CREAT | O_EXCL | O_SYNC;
						break;
					default:
						ERROR("Isto nao devia ser impresso !");
						break;
				}
				
				if((ret->log_fd = open(ret->log_name, O_RDONLY)) != -1) {
					ret->current_log_size = file_size(ret->log_fd);
				} else {
					ret->current_log_size = 0;
				}
			} else {
				ERROR("Malloc");
				free(ret);
				ret = NULL;
			}
		} else {
			ERROR("Mode invalido.");
			free(ret);
			ret = NULL;
		}
	}
	return ret;
	
}

/* 
 * Destroi este gestor de persistência. Retorna 0 se tudo estiver OK ou -1
 * em caso de erro.
 * Note que esta função não limpa os ficheiros de log e ckp do sistema.
 */
int pmanager_destroy(struct pmanager_t *pmanager) {
	int fileOk = -2;
	if(pmanager) {
		if(pmanager->ckp_name) {
			free(pmanager->ckp_name);
		}
		if(pmanager->ckp_fd > 0) {
			close(pmanager->ckp_fd);
		}
		if(pmanager->stt_name) {
			free(pmanager->stt_name);
		}
		if(pmanager->stt_fd > 0) {
			close(pmanager->stt_fd);
		}
		if(pmanager->log_name) {
			free(pmanager->log_name);
		}
		if(pmanager->log_fd > 0) {
			if(write(pmanager->log_fd, &fileOk, sizeof(int)) != sizeof(int)) {
				ERROR("write");
			}
			close(pmanager->log_fd);
		}
		free(pmanager);
		return 0;
	}
	return -1;
	
}

/* 
 * Apaga os ficheiros de log e ckp geridos por este gestor de persistência
 * e o destroi. Retorna 0 se tudo estiver OK ou -1 em caso de erro.
 */
int pmanager_destroy_clear(struct pmanager_t *pmanager) {
	if(pmanager) {
		if(pmanager->ckp_name) {
			if(pmanager->ckp_fd > 0) {
				close(pmanager->ckp_fd);
			}
			remove(pmanager->ckp_name);
			free(pmanager->ckp_name);
		}
		if(pmanager->log_name) {
			if(pmanager->log_fd > 0) {
				close(pmanager->log_fd);
			}
			remove(pmanager->log_name);
			free(pmanager->log_name);
		}
		return pmanager_destroy(pmanager);
	}
	return -1;
	
}

/*
 * Determina tamanho do ficheiro referenciado por fd.
 */
int file_size(int fd) {
	
	struct stat buf;
	if(fd > 0) {
		fstat(fd, &buf);
	}
	return fd > 0 ? (int)buf.st_size : -1;
	
}

/* 
 * Retorna 1 caso existam dados nos ficheiros de log e/ou checkpoint e 0
 * caso contrário.
 */
int pmanager_have_data(struct pmanager_t *pmanager) {
	if(pmanager->current_log_size > 0) {
		return 1;
	} else if((pmanager->ckp_fd = open(pmanager->ckp_name, O_RDONLY)) != -1 && file_size(pmanager->ckp_fd) > 0) {
		close(pmanager->ckp_fd);
		pmanager->ckp_fd = -1;
		return 1;
	} else {
		return 0;
	}
}

/* 
 * Escreve uma string msg no fim do ficheiro de log associado a pmanager.
 * Retorna o número de bytes escritos no log ou -1 em caso de problemas na
 * escrita (e.g., erro no write()) ou caso o tamanho do ficheiro de log após
 * o armazenamento da mensagem seja maior que logsize (neste caso msg não é
 * escrita no log).
 */
int pmanager_log(struct pmanager_t *pmanager, char *msg) {
	int size, zero = 0;
	if(pmanager && msg) {
		// Vamos verificar se é preciso de criar um novo .log...
		if(pmanager->log_fd == -1 && (pmanager->log_fd = open(pmanager->log_name, pmanager->create_flags, PERMISSIONS)) != -1) {
			pmanager->current_log_size = 0;
			lseek(pmanager->log_fd, pmanager->max_log_size - 1, SEEK_SET);
			if(write(pmanager->log_fd, &zero, 1) != 1) {
				ERROR("write");
				close(pmanager->log_fd);
				free(pmanager->stt_name);
				free(pmanager->ckp_name);
				free(pmanager->log_name);
				free(pmanager);
				pmanager = NULL;
			}
			lseek(pmanager->log_fd, 0, SEEK_SET);
		}
		size = (int)strlen(msg);
		if((pmanager->current_log_size + sizeof(size) + strlen(msg) + 1) < pmanager->max_log_size) {
			printf("A escrever: %s\n", msg);
			if(write(pmanager->log_fd, &size, sizeof(size)) == sizeof(size) && 
			   write(pmanager->log_fd, msg, size + sizeof(int)) == strlen(msg) + sizeof(int)) {
				pmanager->current_log_size += sizeof(int) + strlen(msg) + 1;
				return 0;
			} else {
				ERROR("write - esqueceu-se de fazer table-fill?");
			}
		} else {
			ERROR("max log size has been reached");
		}
	} else {
		ERROR("pmanager or msg NULL");
	}
	return -1;
}

/* 
 * Cria um ficheiro filename+".stt" com o estado de table. Retorna
 * o tamanho do ficheiro criado ou -1 em caso de erro.
 * 
 * Estrutura do ficheiro:
 * Cada entrada na tabela equivale a 2 linhas,
 *	1: O tamanho, em bytes, da linha seguinte
 *  2: Uma linha descrevendo uma entrada na tabela, com a seguinte estrutura:
 *	---> KEY DATA-(BASE64)-SIZE DATA-BASE64
 */
int pmanager_store_table(struct pmanager_t *pmanager, struct table_t *table) {
	struct entry_t **entries;
	char *data, *string_to_print, *ts, *encodedTs;
	int counter = 0, ret = 0, encoded_size, stringSize, fileOk = -2;
	size_t encodedSize;
	
	if(pmanager && table) {
		if((entries = table_get_entries(table))) {
			if((pmanager->stt_fd = open(pmanager->stt_name, pmanager->create_flags, PERMISSIONS)) == -1) {
				// Vamos verificar qual o tamanho do .stt, se for 0 podemos usa-lo.
				if((pmanager->stt_fd = open(pmanager->stt_name, O_RDONLY)) != -1 &&
				   file_size(pmanager->stt_fd) == 0) {
					close(pmanager->stt_fd);
					remove(pmanager->stt_name);
					if((pmanager->stt_fd = open(pmanager->stt_name, pmanager->create_flags, PERMISSIONS)) == -1) {
						ERROR("open");
						table_free_entries(entries);
						return -1;
					}
				} else {
					table_free_entries(entries);
					ERROR("Ficheiro .stt ja existe! Fez table-fill e rotate-log?");
					return -1;
				}
			}
			while(entries[counter]) {
				// Dados data são comprimidos
				encode_timestamp(entries[counter]->value->timestamp, &encodedSize, &encodedTs);
				if(!encodedTs) {
					ERROR("encode_timestamp");
					ret = -1;
				} else if((encoded_size = (int)base64_encode_alloc(entries[counter]->value->data, entries[counter]->value->datasize, &data)) > 0) {
					stringSize = (int)strlen(entries[counter]->key) + (int)strlen(data) + (int)strlen(encodedTs) + 3;
					if((string_to_print = (char*)malloc(stringSize))) {
						sprintf(string_to_print, "%s %s %s", encodedTs, entries[counter]->key, data);
						stringSize = (int)strlen(string_to_print);
						if(write(pmanager->stt_fd, &stringSize, sizeof(stringSize)) == sizeof(stringSize) &&
						   write(pmanager->stt_fd, string_to_print, strlen(string_to_print)) == strlen(string_to_print)) {
							// Correu tudo OK
							counter ++;
							free(data);
						} else {
							ERROR("Erro com write!");
							free(data);
							ret = -1;
							break;
						}
						free(string_to_print);
						string_to_print = NULL;
					} else {
						ERROR("Erro com malloc!");
						free(data);
						ret = -1;
						break;
					}
					free(encodedTs);
				} else {
					ERROR("Erro com base64_encode_alloc");
					ret = -1;
					break;
				}
			}
			if(!ret) {
				if(write(pmanager->stt_fd, &fileOk, sizeof(int)) != sizeof(int)) {
					ERROR("write");
					ret = -1;
				}
			}
			table_free_entries(entries);
		}
	}
	
	return ret;
}

/* 
 * Limpa o ficheiro ".log" e copia o ficheiro ".stt" para ".ckp". Retorna 0
 * se tudo correr bem ou -1 em caso de erro.
 */
int pmanager_rotate_log(struct pmanager_t *pmanager) {
	
	if(pmanager) {
		// Apaga o ficheiro .log
		if(pmanager->log_fd > 0) {
			close(pmanager->log_fd);
			remove(pmanager->log_name);
			pmanager->log_fd = -1;
			pmanager->current_log_size = 0;
		}
		// Apaga o ficheiro .ckp
		remove(pmanager->ckp_name);
		// Rename .stt para .ckp
		rename(pmanager->stt_name, pmanager->ckp_name);
	}
	
	return 0;
}

/*
 * Preenche a tabela com a informação contida no ficheiro.
 * -1 : Erro fatal, -2: ficheiro sem dados.
 */
int table_fill(int fd, struct table_t *table) {
	int size, ok = 1, ret = 0, encodedSize, fileOk = 0;
	size_t decodedSize;
	char *string, *keyString, *encodedData, *decodedData, *rest, *encodedTs = NULL, *decodedTs = NULL;
	struct data_t *data;
	
	// Se o stt foi apenas criado?
	if(file_size(fd) == 0) {
		return -2;
	}
	
	if(table) {
		while(ok) {
			if(read(fd, &size, sizeof(int)) == sizeof(int)) {
				if(size == -2) {
					fileOk = 1;
				} else if((string = (char*)malloc(size + 1))) {
					if(read(fd, string, size) == size) {
						string[size] = '\0';
						if((encodedTs = strdup(strtok_r(string, " \0", &rest))) && (keyString = strdup(strtok_r(NULL, " ", &rest))) && 
						   (encodedData = strdup(strtok_r(NULL, "\0", &rest)))) {
							if(encodedData) {
								encodedSize = (int)strlen(encodedData);
							} else {
								encodedSize = 0;
								decodedData = NULL;
							}
							if(encodedSize == 0 || base64_decode_alloc(encodedData, encodedSize, &decodedData, &decodedSize)) {
								if((data = data_create2((int)decodedSize, decodedData))) {
									if(base64_decode_alloc(encodedTs, strlen(encodedTs), &decodedTs, &decodedSize)) {
										data->timestamp = atol(decodedTs);
										if(table_put(table, keyString, data) == -1) {
											ERROR("table_put");
											ok = 0;
											ret = -1;
										}
										free(decodedTs);
									} else {
										ERROR("base64_decode_alloc");
										ret = -1;
									}
									data_destroy(data);
								} else {
									ERROR("data_create");
									ret = -1;
									ok = 0;
								}
								free(encodedData);
							} else {
								ERROR("base64_decode_alloc");
								free(encodedData);
								ret = -1;
								ok = 0;
							}
							free(keyString);
							free(string);
							free(encodedTs);
						} else {
							ERROR("strdup");
							if(keyString) {
								free(keyString);
							}
							free(string);
							ret = -1;
							ok = 0;
						}
					} else {
						ERROR("bad file");
						free(string);
						ret = -1;
						ok = 0;
					}
				} else {
					ERROR("malloc");
					ret = -1;
					ok = 0;
				}
			} else {
				// Já não temos nada para ler
				ok = 0;
				if(!fileOk) {
					ret = -2;
				} else {
					printf("encontrei o -2\n");
				}
			}
		}
	} else {
		ERROR("NULL table");
		ret = -1;
	}
	return ret;
	
}

/* 
 * Mete o estado contido nos ficheiros .log, .stt ou .ckp na tabela passada
 * como argumento.
 */
int pmanager_fill_state(struct pmanager_t *pmanager, struct table_t *table) {
	int ret = -1, val;
	// Se existir um .stt usamos esse, senão usamos o .ckp e o log.
	if(pmanager && table) {
		if((pmanager->stt_fd = open(pmanager->stt_name, O_RDONLY)) != -1) {
			// Existe um ficheiro .stt, iremos recuperar os dados a partir de esse e passa-lo para .ckp, apaga os .log e .ckp se existirem, pois como houve falha não temos garantia que estão OK.
			printf("Existe um .stt, vamos recuperar o estado nele contido.\n");
			if((val = table_fill(pmanager->stt_fd, table)) == -2) {
				printf(".stt nao tinha dados... Vamos usar o .log e .skp e ver se conseguimos recuperar algo...\n");
				struct table_t *newTable = table_create(table->hashSize);
				//table_destroy(table);
				table = newTable;
				remove(pmanager->stt_name);
				ret = recover_from_ckp_log(pmanager, table);
			} else if(val == -1) {
				ERROR("Erro a recuperar de .stt.. A tabela pode estar incorrecta!\n");
				ret = -1;
			} else {
				ret = 0;
			}
			remove(pmanager->log_name);
			remove(pmanager->ckp_name);
		} else {
			ret = recover_from_ckp_log(pmanager, table);
		}
	}
	return ret;
}

int recover_from_ckp_log(struct pmanager_t *pmanager, struct table_t *table) {
	int ret = -1, val;
	struct table_t *newTable;
	if((pmanager->ckp_fd = open(pmanager->ckp_name, O_RDONLY)) != -1) {
		printf("Existe um .ckp, vamos recuperar o estado nele contido.\n");
		if((val = table_fill(pmanager->ckp_fd, table)) == -1) {
			ERROR("table-fill error. A tabela pode não estar correcta!");
		} else if(val == -2) {
			newTable = table_create(table->hashSize);
			//table_destroy(table);
			table = newTable;
			ret = -1;
		} else {
			ret = 0;
		}
	}
	if((pmanager->log_fd > 0 && pmanager->current_log_size > 0) || (pmanager->log_fd = open(pmanager->log_name, O_RDONLY)) != -1) {
		printf("Vamos recuperar do .log\n");
		if(execute_log(pmanager->log_fd, table) < 0) {
			ERROR("execute-log error. A tabela pode não estar correcta!");
			newTable = table_create(table->hashSize);
			//table_destroy(table);
			table = newTable;
		} else {
			ret = 0;
		}
	}
	return ret;
}

/*
 * Percorre o ficheiro log linha a linha.
 */
int execute_log(int fd, struct table_t *table) {
	int size, ok = 1, ret = 0, fileOk = 0;
	char *logString;
	if(table) {
		while(ok) {
			if(read(fd, &size, sizeof(int)) == sizeof(int)) {
				if(size == -2) {
					fileOk = 1;
				} else if((logString = (char*)malloc(size + 1))) {
					if(read(fd, logString, size) == size) {
						logString[size] = '\0';
						if(run_line(logString, table) == -1) {
							ERROR("bad message");
						}
						free(logString);
					} else {
						ERROR("bad log");
						ret = -1;
						ok = 0;
					}
				} else {
					ERROR("malloc");
					ret = -1;
					ok = 0;
				}
			} else {
				// Já não temos nada para ler
				ok = 0;
				if(!fileOk) {
					ret = -1;
				} else {
					printf("Encontrei o -2\n");
				}
			}
		}
	} else {
		ERROR("NULL table");
		ret = -1;
	}
	return ret;
	
}

/*
 * Interpreta cada linha do ficheiro log
 */
int run_line(char *line, struct table_t *table) {
	int retVal = 0;
	size_t decodedSize;
	char *key, *encodedData = NULL, *op, *dup, *decodedData = NULL, *encodedTs = NULL, *decodedTs = NULL;
	struct data_t *data;
	
	if(strlen(line) > 0) {
		if(line && (dup = strdup(line))) {
			op = strdup(strtok(dup, " \0"));
			if(op && strcmp(op, "del") == 0) {
				if((key = strtok(NULL, " \0"))) {
					retVal = table_del(table, key);
				} else {
					ERROR("log corrompido!");
					retVal = -1;
				}
				free(op);
			} else if(op && strcmp(op, "put") == 0) {
				printf("A recuperar: %s\n", line);
				if((encodedTs = strdup(strtok(NULL, " \0"))) && 
				   (key = strdup(strtok(NULL, " \0"))) && 
				   (encodedData = strdup(strtok(NULL, " \0")))) {
					if((base64_decode_alloc(encodedData, strlen(encodedData), &decodedData, &decodedSize))) {
						if((data = data_create2((int)decodedSize, decodedData))) {
							if((base64_decode_alloc(encodedTs, strlen(encodedTs), &decodedTs, &decodedSize))) {
								printf("Timestamp recuperado: %ld\n", atol(decodedTs));
								data->timestamp = atol(decodedTs);
								retVal = table_put(table, key, data);
								free(decodedTs);
							} else {
								ERROR("base64_decode_alloc: timestamp");
								retVal = -1;
							}
							data_destroy(data);
						} else {
							ERROR("data-create2");
							retVal = -1;
						}
					} else {
						ERROR("base64_decode_alloc");
						retVal = -1;
					}
					free(key);
					free(encodedData);
				} else {
					ERROR("log corrompido!");
					retVal = -1;
				}
				free(op);
			} else {
				ERROR("log corrompido ou malloc.");
				retVal = -1;
			}
			free(dup);
		} else {
			ERROR("NULL table or line");
			retVal = -1;
		}
	}
	return retVal;
	
}