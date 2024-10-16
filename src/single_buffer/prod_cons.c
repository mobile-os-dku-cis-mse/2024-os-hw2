#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define BILLION 1000000000L
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256
typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	int eof;
	int full;
} so_t;

typedef struct res_arr {
	int stat [MAX_STRING_LENGTH];
	pthread_mutex_t stat_mutex[MAX_STRING_LENGTH];
	int stat2 [ASCII_SIZE];
	pthread_mutex_t stat2_mutex[ASCII_SIZE];
}RESULT_ARR;

RESULT_ARR res;
void print_char_stat();

pthread_cond_t prod_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_cv = PTHREAD_COND_INITIALIZER;

void update_stat(int index, int value) {
	pthread_mutex_lock(&res.stat_mutex[index]);
	res.stat[index] += value;
	pthread_mutex_unlock(&res.stat_mutex[index]);
}

void update_stat2(int index, int value) {
	pthread_mutex_lock(&res.stat2_mutex[index]);
	res.stat2[index] += value;
	pthread_mutex_unlock(&res.stat2_mutex[index]);
}

void res_arr_init() {
	for(int i = 0; i < MAX_STRING_LENGTH; i++) {
		pthread_mutex_init(&res.stat_mutex[i], NULL);
	}
	for(int i = 0; i < ASCII_SIZE; i++) {
		pthread_mutex_init(&res.stat2_mutex[i], NULL);
	}
}

void get_char_stat_from_line(char* line) {
	char* cptr = line;
	char* substr = NULL;
	char* brka = NULL;
	char *sep = "{}()[],;\" \n\t^"; // 구분자
	size_t length = 0;
	if(line == NULL) return;

	for(substr = strtok_r(cptr, sep, &brka);substr; substr = strtok_r(NULL, sep, &brka)) {
		length = strlen(substr);

		if(length >= 30) length = 30;

		update_stat(length-1, 1);// mutex_lock 필요, stat[length-1]++;

		for(int i = 0; i < length; i++) {
			if(*cptr <256 && *cptr > 1)
				update_stat2(*cptr, 1);
			cptr++;
		}
		cptr++;
		if (*cptr == '\0') break;
	}
}

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

		// lock waiting
		while(so->full == 1) {
			pthread_cond_wait(&prod_cv, &so->lock);
		}

		// read the line
		read = getdelim(&line, &len, '\n', rfile);

		// check the terminate condition
		if (read == -1) {
			so->eof = 1;
			//so->full = 1;
			//so->line = NULL;
			pthread_cond_broadcast(&cond_cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}

		// shared object job
		so->linenum = i;
		so->line = strdup(line);      /* share the line */
		i++;
		so->full = 1;
		pthread_cond_signal(&cond_cv);
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
		//check the condition for mutex lock and Termination condition
		pthread_mutex_lock(&so->lock);
		while(so->full == 0 && so->eof == 0) {
			pthread_cond_wait(&cond_cv, &so->lock);
		}
		line = so->line;
		if (so->full == 0 && so->eof == 1) {
			pthread_cond_broadcast(&prod_cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}

		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		//free(so->line);
		i++;
		so->full = 0;

		//thread unlock
		pthread_cond_signal(&prod_cv);
		pthread_mutex_unlock(&so->lock);

		get_char_stat_from_line(line);
		free(line);
	}
	printf("Cons: %d lines\n", i);
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



	struct timespec start, end;
	uint64_t diff;
	clock_gettime(CLOCK_MONOTONIC, &start);

	pthread_mutex_init(&share->lock, NULL);
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

	clock_gettime(CLOCK_MONOTONIC, &end);

	print_char_stat();

	diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	printf("Running Threads for %llu ns (%.9f sec)\n", (long long unsigned int) diff, (double) diff / BILLION);

	pthread_exit(NULL);

	exit(0);
}


void print_char_stat() {

	// sum
	int sum = 0;
	for (int i = 0 ; i < 30 ; i++) {
		sum += res.stat[i];
	}

	// print out distributions
	printf("*** print out distributions *** \n");
	printf("  #ch  freq \n");
	for (int i = 0 ; i < 30 ; i++) {
		int j = 0;
		int num_star = res.stat[i]*80/sum;
		printf("[%3d]: %4d \t", i+1, res.stat[i]);
		for (j = 0 ; j < num_star ; j++)
			printf("*");
		printf("\n");
	}
	printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
	printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
			res.stat2['A']+res.stat2['a'], res.stat2['B']+res.stat2['b'],  res.stat2['C']+res.stat2['c'],  res.stat2['D']+res.stat2['d'],  res.stat2['E']+res.stat2['e'],
			res.stat2['F']+res.stat2['f'], res.stat2['G']+res.stat2['g'],  res.stat2['H']+res.stat2['h'],  res.stat2['I']+res.stat2['i'],  res.stat2['J']+res.stat2['j'],
			res.stat2['K']+res.stat2['k'], res.stat2['L']+res.stat2['l'],  res.stat2['M']+res.stat2['m'],  res.stat2['N']+res.stat2['n'],  res.stat2['O']+res.stat2['o'],
			res.stat2['P']+res.stat2['p'], res.stat2['Q']+res.stat2['q'],  res.stat2['R']+res.stat2['r'],  res.stat2['S']+res.stat2['s'],  res.stat2['T']+res.stat2['t'],
			res.stat2['U']+res.stat2['u'], res.stat2['V']+res.stat2['v'],  res.stat2['W']+res.stat2['w'],  res.stat2['X']+res.stat2['x'],  res.stat2['Y']+res.stat2['y'],
			res.stat2['Z']+res.stat2['z']);
}