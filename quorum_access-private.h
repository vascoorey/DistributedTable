//
//  quorum_access-private.h
//  Fase3
//
//  Created by Vasco Orey on 12/7/11.
//  Copyright (c) 2011 Vasco Orey. All rights reserved.
//

#ifndef _QUORUM_ACCESS_PRIVATE_H
#define _QUORUM_ACCESS_PRIVATE_H

#include "utils.h"
#include "quorum_access.h"
#include "remote_table.h"

struct quorum_access_t {
	pthread_t *pool;
	struct rtable_t **tables;
	int n_threads;
};

struct task_t {
	struct quorum_op_t *task;
	struct task_t *next;
	int id;
};

struct qa_table_t {
	struct rtable_t *table;
	int id;
};

struct connection_t {
	pthread_t self;
	struct rtable_t *table;
};

// qa_table_t
struct qa_table_t *qa_table(struct rtable_t *table, int id);
// Funções threads
void init_thread_pool(pthread_t *pool, int n_threads, void *(*tfunc)(void*));
void *worker_thread_function(void *arg);
void *handle_dc(sig_t sig);
// Queue
void queue_init();
void queue_destroy();
void queue_add_task(struct task_t *task);
struct task_t *queue_get_task();
void add_completed_task(struct task_t *task);
struct task_t *get_completed_task();
void clear_completed_tasks();
void clear_tasks();
// Task
struct task_t *task_create(struct quorum_op_t *op);
void task_destroy(struct task_t *tasks);
#endif
