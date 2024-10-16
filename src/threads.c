#include "producer.h"
#include "consumer.h"
#include "shared_object.h"
#include <stdlib.h>

void create_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons, so_t *share)
{
	int i;

	for (i = 0; i < Nprod; i++)
		pthread_create(&prod[i], NULL, producer, share);

	for (i = 0; i < Ncons; i++)
		pthread_create(&cons[i], NULL, consumer, share);
}

void join_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons)
{
	int *ret = NULL;
    int i = 0;

	for (i = 0; i < Ncons; i++) {
		pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
		free(ret);
	}

	for (i = 0; i < Nprod; i++) {
		pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
		free(ret);
	}
}

void cleanup(so_t *share)
{
	pthread_mutex_destroy(&share->lock);
	pthread_cond_destroy(&share->not_full);
	pthread_cond_destroy(&share->not_empty);
	fclose(share->rfile);
	free(share);
}
