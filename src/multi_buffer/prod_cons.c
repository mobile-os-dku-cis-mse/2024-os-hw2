#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define BILLION 1000000000L
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256

// 공유 버퍼의 개수는 생산자에 맞추기
typedef struct {
	char* line;
	size_t size;
	pthread_mutex_t lock;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
	int full;
	int eof;
} buffer;

typedef struct {
	FILE* file;
	long start_pos;
	long end_pos;
	buffer* buffer;
}producer_arg;

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
		//res.stat[i] = 0;
	}
	for(int i = 0; i < ASCII_SIZE; i++) {
		pthread_mutex_init(&res.stat2_mutex[i], NULL);
		//res.stat2[i] = 0;
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
	producer_arg* prod_arg = (producer_arg*)arg;
	buffer *buf = prod_arg->buffer;
	FILE *rfile = prod_arg->file;
	long start_pos = prod_arg->start_pos;
	long end_pos = prod_arg->end_pos;

	fseek(rfile, start_pos, SEEK_SET);

	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	int i = 0;
	long current_pos = start_pos;

	while ((read = getdelim(&line, &len, '\n', rfile)) != -1) {
		current_pos = ftell(rfile);

		if(current_pos > end_pos) {
			break;
		}

		// critical section
		pthread_mutex_lock(&buf->lock);

		while(buf->full == 1) {
			pthread_cond_wait(&buf->cond_empty, &buf->lock);
		}

		buf->line = strdup(line);
		buf->full = 1;

		pthread_cond_signal(&buf->cond_full);
		pthread_mutex_unlock(&buf->lock);

		free(line);
		line = NULL;
		len = 0;
		i++;
		printf("Prod_%x: [%d] %s", (unsigned int)pthread_self(), i, buf->line);
	}

	pthread_mutex_lock(&buf->lock);
	buf->eof = 1;
	pthread_cond_signal(&buf->cond_full);
	pthread_mutex_unlock(&buf->lock);

	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	fclose(rfile);

	int *ret = (int *)malloc(sizeof(int));
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	buffer* buf = (buffer*) arg;
	int i = 0;
	char *line = NULL;

	while(1) {
		pthread_mutex_lock(&buf->lock);

		while(!buf->full && !buf->eof) {
			pthread_cond_wait(&buf->cond_full, &buf->lock);
		}

		if (buf->full) {
			line = buf->line;
			buf->line = NULL;
			buf->full = 0;
			printf("Cons_[%d]: %s", i, line);
			pthread_cond_signal(&buf->cond_empty);
			pthread_mutex_unlock(&buf->lock);

			get_char_stat_from_line(line);
			free(line);
			i++;
		} else if(buf->eof) {
			pthread_mutex_unlock(&buf->lock);
			break;
		} else {
			pthread_mutex_unlock(&buf->lock);
		}
	}
	printf("Cons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	int* ret = (int *)malloc(sizeof(int));
	*ret = i;
	pthread_exit(ret);
}

int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];

	int Nprod, Ncons;
	int *ret;
	int i;
	FILE *rfile;

	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}

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

	// result stat array initialize
	res_arr_init();

	// buffer & producer argument initialize
	buffer* bufv = calloc(Nprod, sizeof(buffer));
	producer_arg* prod_arg = calloc(Nprod, sizeof(producer_arg));

	fseek(rfile, 0, SEEK_END);
	long file_size = ftell(rfile);
	long f_segment_size = file_size / Nprod;
	fseek(rfile, 0, SEEK_SET);

	for(int i = 0; i < Nprod; i++) {
		bufv[i].line = NULL;
		bufv[i].size = 0;
		pthread_mutex_init(&bufv[i].lock, NULL);
		pthread_cond_init(&bufv[i].cond_full, NULL);
		pthread_cond_init(&bufv[i].cond_empty, NULL);
		bufv[i].full = 0;
		bufv[i].eof = 0;

		// 파일 세그먼트의 마지막 문장이면 어떻게 처리할지 고민하기
		prod_arg[i].file = fopen(argv[1], "r");
		//printf("File pointer: %x", (unsigned int)prod_arg[i].file);
		prod_arg[i].start_pos = i * f_segment_size;
		if (i == Nprod - 1) {
			prod_arg[i].end_pos = file_size;
		} else {
			prod_arg[i].end_pos = (i + 1) * f_segment_size;
		}

		if(prod_arg[i].start_pos != 0) {
			fseek(prod_arg[i].file, prod_arg[i].start_pos - 1, SEEK_SET);
			int c = fgetc(prod_arg[i].file);
			while(c != '\n' && c != EOF) {
				c = fgetc(prod_arg[i].file);
				prod_arg[i].start_pos = ftell(prod_arg[i].file);
			}
		} else {
			fseek(prod_arg[i].file, prod_arg[i].start_pos, SEEK_SET);
		}

		if(prod_arg[i].end_pos != file_size) {
			fseek(prod_arg[i].file, prod_arg[i].end_pos, SEEK_SET);
			int c = fgetc(prod_arg[i].file);
			while(c != '\n' && c != EOF) {
				c = fgetc(prod_arg[i].file);
				prod_arg[i].end_pos = ftell(prod_arg[i].file);
			}
		}

		prod_arg[i].buffer = &bufv[i];
	}





	struct timespec start, end;
	uint64_t diff;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, &prod_arg[i]);
	for (i = 0 ; i < Ncons ; i++)
		// 왜 bufv가?
		pthread_create(&cons[i], NULL, consumer, &bufv[i]);
	printf("main continuing\n");
	int rc;
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

	for(int i = 0; i < Nprod; i++) {
		pthread_mutex_destroy(&bufv[i].lock);
		pthread_cond_destroy(&bufv[i].cond_full);
		pthread_cond_destroy(&bufv[i].cond_empty);
		if(bufv[i].line != NULL) {
			// 이중 Free 주의
		}
		// 이중 Close
		//fclose(prod_arg[i].file);
	}
	free(bufv);
	free(prod_arg);

	pthread_exit(NULL);
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