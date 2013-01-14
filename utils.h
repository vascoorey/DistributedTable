#ifndef _UTILS_H
#define _UTILS_H

// Std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// Time
#include <time.h>
#include <sys/time.h>
// Math
#include <math.h>
// General Sys
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
// INET/INET6
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <signal.h>
// Threads
#include <pthread.h>

#include <assert.h>

// Para usar: ERROR("Malloc"); ira fazer um fprintf da linha e func que chamaram, com a string "Malloc"
// Trocar comentarios para entregar o projecto sem que as nossas mensagens de erro para debug sejam impressas.
#define ERROR(a) fprintf( stderr, "%s : %d : "#a"\n", __FUNCTION__, __LINE__)
//#define ERROR(a)

#define PRINT_LATENCIES 1

#endif