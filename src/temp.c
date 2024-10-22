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
	// producer condition, consumer condition.
	pthread_cond_t p_cond;
	pthread_cond_t c_cond;
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
		// because there may be multiple producers, we wait only if the buffer is 'actually' full. if the entire file has been read, the buffer will be marked as 'full', and every other producers will wait indefinitely for the lock, hence the nullity check of so->line is necessary.
		while (so->full && so->line)
			pthread_cond_wait(&so->p_cond, &so->lock);

		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
			so->full = 1;
			so->line = NULL;
			pthread_cond_broadcast(&so->c_cond);
			pthread_mutex_unlock(&so->lock);
			break;
		}

		// here, we simply increment the line number instead of assigning 'i' to it. the arbitrary context switching between multiple threads makes it impossible to determine the line number by the local counter 'i'.
		so->linenum++;
		so->line = strdup(line);      /* share the line */
		so->full = 1;
		i++;

		pthread_cond_broadcast(&so->c_cond);
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
		while (!so->full)
			pthread_cond_wait(&so->c_cond, &so->lock);

		line = so->line;
		if (line == NULL) {
			pthread_cond_broadcast(&so->p_cond);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		so->full = 0;
		pthread_cond_broadcast(&so->p_cond);
		pthread_mutex_unlock(&so->lock);
	}
	printf("Cons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	// here, linenum is set to -1, because no line has been read yet.
	share->linenum = -1;
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->p_cond, NULL);
	pthread_cond_init(&share->c_cond, NULL);
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	pthread_exit(NULL);
	exit(0);
}
