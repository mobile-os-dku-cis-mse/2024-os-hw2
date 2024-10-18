#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int full;
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		pthread_mutex_lock(&so->lock);
		// a producer may only enter the critical section if the current buffer is empty.
		while (so->full)
			pthread_cond_wait(&so->cond, &so->lock);

		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
			so->full = 1;
			so->line = NULL;
			// signal the pending consumer.
			pthread_cond_signal(&so->cond);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		so->linenum = i;
		so->line = strdup(line);      /* share the line */
		i++;
		so->full = 1;
		// signal the pending consumer.
		pthread_cond_signal(&so->cond);
		pthread_mutex_unlock(&so->lock);
	}
	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;

	while (1) {
		pthread_mutex_lock(&so->lock);
		// a consumer may only enter the critical section if the current buffer is filled.
		while (!so->full)
			pthread_cond_wait(&so->cond, &so->lock);

		line = so->line;
		if (line == NULL) {
			// the producer has already finished its job; no need to signal the producer.
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		so->full = 0;
		pthread_cond_signal(&so->cond);
		pthread_mutex_unlock(&so->lock);
	}
	printf("Cons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod;
	pthread_t cons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;

	// since option 1 only requires you to work with a single producer/consumer, the optional arguments are removed.
	if (argc < 2) {
		printf("usage: ./prod_cons <readfile>\n");
		exit (0);
	}
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}

	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->cond, NULL);
	pthread_create(&prod, NULL, producer, share);
	pthread_create(&cons, NULL, consumer, share);
	printf("main continuing\n");

	rc = pthread_join(cons, (void **) &ret);
	printf("main: consumer joined with %d\n", *ret);
	rc = pthread_join(prod, (void **) &ret);
	printf("main: producer joined with %d\n", *ret);

	pthread_exit(NULL);
	exit(0);
}

