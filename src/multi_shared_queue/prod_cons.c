#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shared_queue.h"
#define BILLION 1000000000L
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256
#define READ_BUFFER_SIZE 1024

// 단일 버퍼의 한계점은 버퍼를 한 번 채우면
// 자신이 가지고 있는 CPU 타임을 전부 버리고 문맥 교환을 수행해야 한다는 점임

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
	shared_queue_t* queue;
}producer_arg;

typedef struct res_arr {
	int stat [MAX_STRING_LENGTH];
	int stat2 [ASCII_SIZE];
}RESULT_ARR;

RESULT_ARR res;
void print_char_stat();

pthread_cond_t prod_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_cv = PTHREAD_COND_INITIALIZER;

void update_stat(int index, int value, int* local_stat) {
	local_stat[index] += value;
}

void update_stat2(int index, int value, int* local_stat2) {
	local_stat2[index] += value;
}

void get_char_stat_from_line(char* line, int* local_stat, int* local_stat2) {
	char* cptr = line;
	char* substr = NULL;
	char* brka = NULL;
	char *sep = "{}()[],;\" \n\t^"; // 구분자
	size_t length = 0;
	if(line == NULL) return;

	for(substr = strtok_r(cptr, sep, &brka);substr; substr = strtok_r(NULL, sep, &brka)) {
		length = strlen(substr);
		if(length >= 30) length = 30;

		update_stat(length-1, 1, local_stat);// mutex_lock 필요, stat[length-1]++;

		for(int i = 0; i < length; i++) {
			if(*cptr <256 && *cptr > 1)
				update_stat2(*cptr, 1, local_stat2);
			cptr++;
		}
		cptr++;
		if (*cptr == '\0') break;
	}
}

void *producer(void *arg) {
	producer_arg* prod_arg = (producer_arg*)arg;
	shared_queue_t *queue = prod_arg->queue;
	FILE *rfile = prod_arg->file;
	long start_pos = prod_arg->start_pos;
	long end_pos = prod_arg->end_pos;

	fseek(rfile, start_pos, SEEK_SET);

	char* line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	int i = 0;
	long current_pos = prod_arg->start_pos;

	while ((read = getdelim(&line, &len, '\n', rfile)) > 0) {
		current_pos = ftell(rfile);
		enqueue(queue, line);
		if(current_pos >= end_pos) {
			break;
		}



		line = NULL;
		len = 0;
		i++;
		//printf("Prod_%x: [%d] %s", (unsigned int)pthread_self(), i, buf->line);
	}

	pthread_mutex_lock(&queue->lock);
	queue->eof = 1;
	pthread_cond_signal(&queue->not_empty);
	pthread_mutex_unlock(&queue->lock);

	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	fclose(rfile);

	int *ret = (int *)malloc(sizeof(int));
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	shared_queue_t *queue = (shared_queue_t *)arg;
	int* local_stat = calloc(MAX_STRING_LENGTH, sizeof(int));
	int* local_stat2 = calloc(ASCII_SIZE, sizeof(int));
	int i = 0;

	while(1) {
		char *line = dequeue(queue);
		if(line == NULL) {
			break;
		}

		get_char_stat_from_line(line, local_stat, local_stat2);
		free(line);
		i++;
	}
	printf("Cons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	void** ptr = malloc(sizeof(int*));
	ptr[0] = local_stat;
	ptr[1] = local_stat2;
	pthread_exit((void*)ptr);
}

void merge_result(void*** ptr, int ncons) {
	for(int i = 0; i < ncons; i++) {
		for(int j = 0; j < MAX_STRING_LENGTH; j++) {
			res.stat[j] += ((int***)ptr)[i][0][j];
		}
		for(int j = 0; j < ASCII_SIZE; j++) {
			res.stat2[j] += ((int***)ptr)[i][1][j];
		}
	}
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

	// buffer & producer argument initialize
	shared_queue_t* queue_v = calloc(Nprod, sizeof(shared_queue_t));
	producer_arg* prod_arg = calloc(Nprod, sizeof(producer_arg));

	fseek(rfile, 0, SEEK_END);
	long file_size = ftell(rfile);
	long f_segment_size = file_size / Nprod;
	fseek(rfile, 0, SEEK_SET);

	for(int i = 0; i < Nprod; i++) {
		shared_queue_init(&queue_v[i]);

		prod_arg[i].file = fopen(argv[1], "r");

		// 시작 위치 설정
		prod_arg[i].start_pos = i * f_segment_size;
		if (i != 0) {  // 첫 번째 프로듀서가 아닌 경우에만 경계 조정
			fseek(prod_arg[i].file, prod_arg[i].start_pos - 1, SEEK_SET);
			int c;
			// 이전 위치에서 '\n'을 만날 때까지 포인터 이동
			while ((c = fgetc(prod_arg[i].file)) != '\n' && c != EOF) {
				prod_arg[i].start_pos = ftell(prod_arg[i].file);
			}
			// 다음 줄의 첫 번째 문자로 이동
			prod_arg[i].start_pos = ftell(prod_arg[i].file);
		}

		// 끝 위치 설정
		if (i == Nprod - 1) {
			// 마지막 프로듀서는 파일 끝까지 읽음
			prod_arg[i].end_pos = file_size;
		} else {
			prod_arg[i].end_pos = (i + 1) * f_segment_size;
			fseek(prod_arg[i].file, prod_arg[i].end_pos, SEEK_SET);
			int c;
			// 다음 '\n'을 만날 때까지 포인터 이동
			while ((c = fgetc(prod_arg[i].file)) != '\n' && c != EOF) {
				prod_arg[i].end_pos = ftell(prod_arg[i].file);
			}
		}

		prod_arg[i].queue = &queue_v[i];
	}

	struct timespec start, end;
	uint64_t diff;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, &prod_arg[i]);
	for (i = 0 ; i < Ncons ; i++)
		// 왜 bufv가?
		pthread_create(&cons[i], NULL, consumer, &queue_v[i]);
	printf("main continuing\n");

	int rc;
	void*** ptr = malloc(sizeof(void**) * Ncons);
	void** ptr_holder;

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ptr_holder);
		ptr[i] = ptr_holder;
		printf("main: consumer_%d joined\n", i);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
		free(ret);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	merge_result(ptr, Ncons);
	print_char_stat();

	diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	printf("Running Threads for %llu ns (%.9f sec)\n", (long long unsigned int) diff, (double) diff / BILLION);

	for(int i = 0; i < Nprod; i++) {
		shared_queue_destroy(&queue_v[i]);
	}
	free(queue_v);
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

		int num_star = sum ? res.stat[i]*80/sum : 0;
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