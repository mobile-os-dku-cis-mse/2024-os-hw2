#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sbuf.h"
#include "stat.h"

typedef struct sharedobject {
	FILE *rfile;
	int rfile_end;
	int p_cnt;
	sbuf_queue empty_buf;
	sbuf_queue full_buf;
	pthread_cond_t p_cond;
	pthread_cond_t c_cond;
	pthread_mutex_t lock;
	int res[26];
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	FILE *rfile = so->rfile;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		sbuf *buf;

		pthread_mutex_lock(&so->lock);
		while (!so->rfile_end && sbuf_queue_empty(&so->empty_buf))
			pthread_cond_wait(&so->p_cond, &so->lock);
		if (so->rfile_end)
		{
			so->p_cnt--;
			pthread_cond_broadcast(&so->c_cond);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		buf = sbuf_queue_pop(&so->empty_buf);
		pthread_mutex_unlock(&so->lock);

		// end-of-file flag.
		int flag = 0;

		// read file until buffer is full or eof is reached.
		while (1)
		{
			read = getdelim(&line, &len, '\n', rfile);

			if (read == -1)
			{
				flag = 1;
				break;
			}

			sbuf_append(buf, line);

			if (sbuf_full(buf))
				break;
		}

		pthread_mutex_lock(&so->lock);
		if (flag)
			so->rfile_end = flag;
		sbuf_queue_push(&so->full_buf, buf);
		pthread_cond_broadcast(&so->c_cond);
		pthread_mutex_unlock(&so->lock);
	}

	free(line);
	pthread_exit(NULL);
}

void *consumer(void *arg) {
	int local_res[26] = {0};
	so_t *so = arg;

	while (1) {
		sbuf *buf;

		pthread_mutex_lock(&so->lock);
		while (sbuf_queue_empty(&so->full_buf) && so->p_cnt)
			pthread_cond_wait(&so->c_cond, &so->lock);
		if (sbuf_queue_empty(&so->full_buf) && !so->p_cnt)
		{
			pthread_mutex_unlock(&so->lock);
			break;
		}
		buf = sbuf_queue_pop(&so->full_buf);
		pthread_mutex_unlock(&so->lock);

		for (int i = 0; i < buf->sz; i++)
			alph_cnt(buf->mem[i], local_res);
		sbuf_clear(buf);

		pthread_mutex_lock(&so->lock);
		sbuf_queue_push(&so->empty_buf, buf);
		pthread_cond_broadcast(&so->p_cond);
		pthread_mutex_unlock(&so->lock);
	}

	// accumulate the locally calculated statistics.
	pthread_mutex_lock(&so->lock);
	for (int i = 0; i < 26; i++)
		so->res[i] += local_res[i];
	pthread_mutex_unlock(&so->lock);

	pthread_exit(NULL);
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
	share->p_cnt = Nprod;
	pthread_cond_init(&share->p_cond, NULL);
	pthread_cond_init(&share->c_cond, NULL);
	pthread_mutex_init(&share->lock, NULL);
	for (i = 0 ; i < Nprod ; i++)
		sbuf_queue_push(&share->empty_buf, sbuf_new(1024));

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++)
		rc = pthread_join(cons[i], (void **) &ret);

	for (i = 0 ; i < Nprod ; i++)
		rc = pthread_join(prod[i], (void **) &ret);

	puts("char | occurrence");
	for (int i = 0; i < 26; i++)
		printf("%c(%c) | %d\n", 'A'+i, 'a'+i, share->res[i]);

	pthread_exit(NULL);
	exit(0);
}

