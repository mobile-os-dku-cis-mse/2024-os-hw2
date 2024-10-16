#include "shared_object.h"
#include <stdlib.h>
#include <string.h>

void *producer(void *arg)
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		pthread_mutex_lock(&so->lock);
		while (so->count == BUFFER_SIZE) {
			pthread_cond_wait(&so->not_full, &so->lock);
		}
		if (read == -1) {
			so->done = 1;
			pthread_cond_broadcast(&so->not_empty);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		so->buffer[so->in] = strdup(line);
		so->linenum[so->in] = i;
		so->in = (so->in + 1) % BUFFER_SIZE;
		so->count++;
		i++;
		pthread_cond_signal(&so->not_empty);
		pthread_mutex_unlock(&so->lock);
	}
	free(line);
	printf("Prod_%lx: %d lines\n", (unsigned long)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}
