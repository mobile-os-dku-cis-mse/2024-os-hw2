#pragma once
#include <pthread.h>
#include "shared_object.h"

void join_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons);
void create_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons,
    so_t *share);
void cleanup(so_t *share);
