//
//  quorum_access.c
//  Fase3
//
//  Created by Vasco Orey on 12/7/11.
//  Copyright (c) 2011 Vasco Orey. All rights reserved.
//

#include "quorum_access.h"
#include "quorum_access-private.h"
#include "remote_table-private.h"
#include "entry.h"
//#include "message.h"

static int request_id = 0;
static struct quorum_access_t *shared_quorum = NULL;
/**/
static pthread_mutex_t queue_lock;
static pthread_mutex_t done_lock;
static pthread_mutex_t check_quit;
static pthread_mutex_t connections_lock;
/**/
static pthread_cond_t done_not_empty;
static pthread_cond_t queue_not_empty;
// Shared stuff
static struct task_t *queue_head = NULL;
static struct task_t *done = NULL;
static bool quit_and_cleanup = false;
static struct connection_t *connections = NULL;

/* Essa funcao deve criar as threads e as filas de comunicacao para que o
 * cliente invoque operações em um conjunto de tabelas em servidores
 * remotos. Recebe como parametro um array de rtable de tamanho n.
 * Retorna 0 (OK) ou -1 (erro). */
int init_quorum_access(struct rtable_t **rtable, int n) {
	int ret = -1, i;
	
	if(!shared_quorum) {
		if(!(shared_quorum = malloc(sizeof(struct quorum_access_t)))) {
			ERROR("malloc");
		} else if(!(shared_quorum->pool = malloc(sizeof(pthread_t) * n))) {
			ERROR("malloc");
			free(shared_quorum);
			shared_quorum = NULL;
		} else {
			if(!(connections = malloc(sizeof(struct connection_t) * n))) {
				ERROR("malloc");
				free(shared_quorum->pool);
				free(shared_quorum);
				shared_quorum = NULL;
			} else {
				for(i = 0; i < n; i ++) {
					connections[i].table = NULL;
					connections[i].self = 0;
				}
				shared_quorum->tables = rtable;
				shared_quorum->n_threads = n;
				init_thread_pool(shared_quorum->pool, n, worker_thread_function);
				ret = 0;
			}
		}
	} else {
		ERROR("init_quorum_access: already called.");
	}
	
	return ret;
}

/* Funcao que envia uma requisicao a um conjunto de servidores e devolve
 * um array com o numero esperado de respostas.
 * O parametro request e uma representacao do pedido enquanto
 * expected_replies <= n representa a quantidade de respostas a ser 
 * esperadas antes da funcao retornar.
 * Note que os campos id e sender da request serão preenchidos dentro da
 * funcao. O array retornado é um array com n posicoes (0 … n-1) sendo a
 * posicao i correspondente a um apontador para a resposta do servidor 
 * i, ou NULL caso a resposta desse servidor não tenha sido recebida.
 * Este array deve ter pelo menos expected_replies posicoes != NULL.
 */
struct quorum_op_t **quorum_access(struct quorum_op_t *request, int expected_replies) {
	int i, replies = 0;
	struct task_t *task = NULL, *completed_task, *temp;
	struct quorum_op_t **ret = malloc(sizeof(struct quorum_op_t*) * shared_quorum->n_threads);
	
	if(!ret) {
		ERROR("malloc");
		return NULL;
	}
	
	request_id ++;
	request->id = request_id;
	
	clear_completed_tasks();
	for(i = 0; i < shared_quorum->n_threads; i++) {
		// Publicar a tarefa n vezes
		task = task_create(request);
		//task->task->sender = i;
		//printf("adding a new task -> %d\n", request_id);
		queue_add_task(task);
		// Init as n possiveis respostas a NULL
		ret[i] = NULL;
	}
	//printf("Expected replies: %d\n", expected_replies);
	while(replies < expected_replies) {
		//printf("Checking for a completed task...\n");
		completed_task = get_completed_task();
		if(completed_task) {
			//printf("Got one!\n");
			// O que fazer se a task_t que encontramos é invalida? (id != request_id)?
			if(completed_task->task->id == request_id) {
				ret[completed_task->task->sender] = completed_task->task;
				//printf("Tarefa %d: %d\n", completed_tasks->task->sender, completed_tasks->task->content.result);
				replies ++;
				//printf("Got reply no: %d\n", replies);
			} else {
				// Por enquanto deitamos fora uma task invalida...
				//printf("Tarefa invalida, %d != %d !\n", request_id, completed_task->task->id);
				free(completed_task->task);
				replies ++;
			}
			free(completed_task);
		}
	}
	clear_tasks();
	return ret;
}

/* Liberta a memoria, destroi as rtables usadas e destroi as threads.
 */
int destroy_quorum_access() {
	int i;
	pthread_mutex_lock(&check_quit);
	// Assim todas as threads sabem que é para terminar...
	quit_and_cleanup = true;
	pthread_mutex_unlock(&check_quit);
	
	for(i = 0; i < shared_quorum->n_threads; i++) {
		//pthread_cancel(shared_quorum->pool[i]);
		//pthread_kill(shared_quorum->pool[i], SIGTERM);
		//printf("Mandando cond_signal...\n");
		pthread_cond_signal(&done_not_empty);
		pthread_cond_signal(&queue_not_empty);
		//pthread_join(shared_quorum->pool[i], NULL);
	}
	// Clean up...
	for(i = 0; i < shared_quorum->n_threads; i++) {
		// Free rtable? connections[i].table
		pthread_join(shared_quorum->pool[i], NULL);
	}
	task_destroy(queue_head);
	task_destroy(done);
	queue_destroy();
	free(shared_quorum->pool);
	free(shared_quorum);
	free(connections);
	shared_quorum = NULL;
	return 1;
}

void task_destroy(struct task_t *tasks) {
	struct task_t *temp;
	while(tasks) {
		temp = tasks->next;
		free(tasks->task);
		free(tasks);
		tasks = temp;
	}
}

// Funções auxiliares
// qa_table_t
struct qa_table_t *qa_table(struct rtable_t *table, int id) {
	struct qa_table_t *ret = malloc(sizeof(struct qa_table_t));
	if(!ret) {
		ERROR("malloc");
		ret = NULL;
	} else {
		ret->table = table;
		ret->id = id;
	}
	return ret;
}

void init_thread_pool(pthread_t *pool, int n_threads, void *(*tfunc)(void*)) {
	int i;
	struct qa_table_t *table;
	queue_init();
	for(i = 0; i < n_threads; i ++) {
		table = qa_table(shared_quorum->tables[i], i);
		pthread_create(&shared_quorum->pool[i], NULL, tfunc, table);
	}
}

void *qa_pipe_handler(sig_t sig) {
	int i = 0;
	struct rtable_t *table;
	bool quit;
	
	signal(SIGPIPE, (__sighandler_t)qa_pipe_handler);
	
	pthread_mutex_lock(&connections_lock);
	while(connections[i].self != pthread_self()) {
		i++;
	}
	table = connections[i].table;
	pthread_mutex_unlock(&connections_lock);
	// Verificar se entretanto foi para sair...
	pthread_mutex_lock(&check_quit);
	quit = quit_and_cleanup;
	pthread_mutex_unlock(&check_quit);
	// Re-estabelecer ligação?
	while(rtable_connect(table) == -1 && !quit) {
		sleep(5);
		// Check quit condition
		pthread_mutex_lock(&check_quit);
		quit = quit_and_cleanup;
		pthread_mutex_unlock(&check_quit);
	}
}

void qa_exit_handler(void *table) {
	if(table) {
		struct qa_table_t *table = (struct qa_table_t *)table;
		if(table) {
			free(table);
		}
	}
}

void *worker_thread_function(void *arg) {
	char *key = NULL;
	struct entry_t *entry;
	struct qa_table_t *table = (struct qa_table_t *)arg;
	bool quit = false;
	int i = 0, old_type;
	
	//pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);
	//pthread_cleanup_push(&qa_exit_handler, table);
	pthread_mutex_lock(&connections_lock);
	while(connections[i].self != 0) {
		i ++;
	}
	connections[i].self = pthread_self();
	// Associar esta rtable ao nosso pid de modo a facilitar voltarmos a ligar
	connections[i].table = table->table;
	table->id = i;
	pthread_mutex_unlock(&connections_lock);
	
	signal(SIGPIPE, (__sighandler_t)qa_pipe_handler);
	
	while(rtable_connect(table->table) == -1 && !quit) {
		// Check quit condition
		pthread_mutex_lock(&check_quit);
		quit = quit_and_cleanup;
		pthread_mutex_unlock(&check_quit);
	}
	
	while(!quit) {
		// Vai buscar a nossa task
		struct task_t *task = queue_get_task();
		// Do something with the task...
		// OPCODE fica + 1?
		//printf("%d - task\n", task);
		if(task) {
			//printf("Got a task!\n");
			task->task->sender = table->id;
			switch(task->task->opcode) {
				case OP_RT_DEL:
					key = strdup(task->task->content.key);
					//free(task->task->content.key);
					if(!(key)) {
						ERROR("strdup");
						task->task->opcode = OP_RT_ERROR;
						task->task->content.result = -1;
					}
					task->task->content.result = rtable_del(table->table, key);
					free(key);
					key = NULL;
					break;
				case OP_RT_GET:
					key = strdup(task->task->content.key);
					//free(task->task->content.key);
					if(!(key)) {
						ERROR("strdup");
						task->task->opcode = OP_RT_ERROR;
						task->task->content.result = -1;
					}
					if((task->task->content.value = rtable_get(table->table, key))) {
						//printf("Apanhei ts: %ld, size: %d\n", task->task->content.value->timestamp, task->task->content.value->datasize);
					} else {
						//printf("Entrada n encontrada...\n");
					}
					free(key);
					key = NULL;
					break;
				case OP_RT_GETKEYS:
					task->task->content.keys = rtable_get_keys(table->table);
					break;
				case OP_RT_PUT:
					entry = entry_dup(task->task->content.entry);
					task->task->content.result = rtable_put(table->table, entry);
					//entry_destroy(entry);
					break;
				case OP_RT_SIZE:
					task->task->content.result = rtable_size(table->table);
					//printf("Tamanho obtido: %d\n", task->task->content.result);
					break;
				case OP_RT_GETTS:
					key = strdup(task->task->content.key);
					if(!(key)) {
						ERROR("strdup");
						task->task->opcode = OP_RT_ERROR;
						task->task->content.result = -1;
					}
					//free(task->task->content.key);
					task->task->content.timestamp = rtable_get_ts(table->table, key);
					printf("Timestamp %s: %ld\n", key, task->task->content.timestamp);
					free(key);
					key = NULL;
					break;
				default:
					ERROR("bad opcode");
					break;
			}
			add_completed_task(task);
		} else {
			quit = true;
		}
	}
	
	if(table) {
		free(table);
	}

	// Disconnect fica para o quorum_table
	//free(table->table->ip);
	//free(table->table->porto);
	//free(table->table);
	//free(table);
	
	//pthread_cleanup_pop(1);
	//pthread_setcanceltype(old_type, 0);
	
	pthread_exit(0);
}

// Queue
void queue_init() {
	if(pthread_mutex_init(&queue_lock, NULL) != 0 || pthread_cond_init(&queue_not_empty, NULL) != 0
	   || pthread_mutex_init(&done_lock, NULL) != 0 || pthread_cond_init(&done_not_empty, NULL) != 0
	   || pthread_mutex_init(&check_quit, NULL) != 0) {
		ERROR("queue init");
		exit(-1);
	}
}

void queue_destroy() {
	pthread_mutex_destroy(&queue_lock);
	pthread_mutex_destroy(&done_lock);
	pthread_mutex_destroy(&check_quit);
	pthread_cond_destroy(&done_not_empty);
	pthread_cond_destroy(&queue_not_empty);
}

void queue_add_task(struct task_t *task) {
	pthread_mutex_lock(&queue_lock);
	if(!queue_head) {
		queue_head = task;
	} else {
		struct task_t *ptr = queue_head;
		while(ptr->next) {
			ptr = ptr->next;
		}
		ptr->next = task;
	}
	task->next = NULL;
	pthread_cond_signal(&queue_not_empty);
	pthread_mutex_unlock(&queue_lock);
}

struct task_t *queue_get_task() {
	bool quit = false;
	struct task_t *task = NULL;
	
	pthread_mutex_lock(&queue_lock);
	while(!queue_head && !quit) {
		//printf("%d waiting on task...\n", pthread_self());
		pthread_cond_wait(&queue_not_empty, &queue_lock);
		pthread_mutex_lock(&check_quit);
		quit = quit_and_cleanup;
		pthread_mutex_unlock(&check_quit);
	}
	if(!quit) {
		//printf("Got a task: %d\n", queue_head);
		task = queue_head;
		queue_head = queue_head->next;
	}
	pthread_mutex_unlock(&queue_lock);
	return task;
}

void add_completed_task(struct task_t *task) {
	//printf("adding a new completed task! - %d\n", pthread_self());
	pthread_mutex_lock(&done_lock);
	if(!done) {
		done = task;
	} else {
		struct task_t *ptr = done;
		while(ptr->next) {
			ptr = ptr->next;
		}
		ptr->next = task;
	}
	task->next = NULL;
	pthread_cond_signal(&done_not_empty);
	pthread_mutex_unlock(&done_lock);
}

struct task_t *get_completed_task() {
	bool quit = false;
	struct task_t *done_task = NULL;
	pthread_mutex_lock(&done_lock);
	while(!done && !quit) {
		pthread_cond_wait(&done_not_empty, &done_lock);
		pthread_mutex_lock(&check_quit);
		quit = quit_and_cleanup;
		pthread_mutex_unlock(&check_quit);
	}
	if(!quit) {
		done_task = done;
		done = done->next;
		pthread_mutex_unlock(&done_lock);
	}
	return done_task;
}

void clear_completed_tasks() {
	struct task_t *task;
	pthread_mutex_lock(&done_lock);
	while(done) {
		//printf("A limpar as task_t's...\n");
		task = done->next;
		free(done->task);
		free(done);
		done = task;
	}
	done = NULL;
	pthread_mutex_unlock(&done_lock);
}

void clear_tasks() {
	struct task_t *task;
	pthread_mutex_lock(&queue_lock);
	while (queue_head) {
		task = queue_head->next;
		free(queue_head->task);
		free(queue_head);
		queue_head = task;
	}
	queue_head = NULL;
	pthread_mutex_unlock(&queue_lock);
}

// Task
struct task_t *task_create(struct quorum_op_t *op) {
	struct task_t *task = NULL;
	if(!(task = malloc(sizeof(struct task_t)))) {
		ERROR("malloc");
		task = NULL;
	} else {
		if(!(task->task = (struct quorum_op_t*)malloc(sizeof(struct quorum_op_t)))) {
			ERROR("malloc");
			free(task);
			task = NULL;
		} else {
			task->task->opcode = op->opcode;
			task->task->content = op->content;
			task->task->id = op->id;
			//printf("created a task with id -> %d\n", op->id);
		}
	}
	return task;
}