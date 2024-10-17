#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// #define _IO_
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE      256


int stat [MAX_STRING_LENGTH];
int stat2 [ASCII_SIZE];
long int total;

int cnt[26] = { 0, };

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cond; // 조건 변수: 특정 조건이 만족될 때 thread 대기/시작 사용
    pthread_attr_t attr;
	int full;
} so_t;

void *producer(void *arg) { // 변수(type 상관 X) 넣어도 되고 안넣어도 OK
	so_t *so = arg; // arg == NULL 일 때의 상황 넣어야 하나?
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		read = getdelim(&line, &len, '\n', rfile); // 자원 읽기
		if (read == -1) {
			pthread_mutex_lock(&so->lock);  // 뮤텍스 잠금

			// 데이터가 소비(full == 1)될 때까지 기다림
			while (so->full == 1) {
					pthread_cond_wait(&so->cond, &so->lock);
			}

			so->full = 1;
			so->line = NULL;

				// Consumer에게 데이터가 생성되었음을 알림
			pthread_cond_signal(&so->cond);

			pthread_mutex_unlock(&so->lock);  // 뮤텍스 잠금 해제
			break;
		}

		pthread_mutex_lock(&so->lock);  // 뮤텍스 잠금

		// 데이터가 소비될 때까지 기다림
		// printf("so->full = %d\t", so->full);
		while (so->full == 1) {
			// printf("(prod while) so->full = %d\t", so->full);
			pthread_cond_wait(&so->cond, &so->lock);
		}

		so->linenum = i;
		so->line = strdup(line);        /* share the line -> line 복제하여 저장 */
		total++;
		i++;
		so->full = 1;

		// Consumer에게 데이터가 생성되었음을 알림
		pthread_cond_signal(&so->cond);

		pthread_mutex_unlock(&so->lock);  // 뮤텍스 잠금 해제
	}
	free(line);
#ifdef _IO_
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
#endif
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
		char *cptr = NULL;
		char *substr = NULL;
		char *brka = NULL;
		char *sep = "{}()[],;\" \n\t^";

		pthread_mutex_lock(&so->lock);  // 뮤텍스 잠금

		// 데이터가 생산될 때까지 기다림
		while (so->full == 0) {
			pthread_cond_wait(&so->cond, &so->lock);
		}

		line = so->line;
		if (line == NULL) {
			break;
		}
		len = strlen(line);
		// cptr = line;

#ifdef _IO_
		printf("Cons_%x: [%02d:%02d] %s",
				(unsigned int)pthread_self(), i, so->linenum, line);
#endif

		for (substr = strtok_r(line, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
			len = strlen(substr);
			// update stats

			// length of the sub-string
			if (len >= 30) len = 30;
#ifdef _IO2_
			printf("len: %d\n", (int)len);
#endif
			stat[len-1]++;

			// number of the character in the sub-string
			for (int i = 0 ; i < len ; i++) {
				if (*line < 256 && *line > 1) {
					stat2[*line]++;
#ifdef _IO2_
					printf("# of %c(%d): %d\n", *line, *line, stat2[*line]);
#endif
				}
				line++;
			}
			line++;
			if (*line == '\0') break;
		}

		free(so->line);
		i++;
		so->full = 0;

		// Producer에게 데이터를 소비했음을 알림
		pthread_cond_signal(&so->cond);

		pthread_mutex_unlock(&so->lock);  // 뮤텍스 잠금 해제
	}
#ifdef _IO2_
	printf("Cons: %d lines\n", i);
#endif
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;       // thread 개수 결정
	int rc;   long t;
	int *ret;
	int i;
	int sum = 0;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}

	// structure init
	so_t *share = malloc(sizeof(so_t)); // 공유 자원에 대한 구조체 미리 할당
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "rb"); // 받은 파일 open
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

	// 상태 초기화
	memset(stat, 0, sizeof(stat));
	memset(stat2, 0, sizeof(stat));

	share->rfile = rfile; // 공유할 file
	share->line = NULL;

	// pthread 초기화
	pthread_attr_init(&share->attr);
	pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond, NULL);

	// producer, consumer 함수에 대한 thread 새로 생성
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	// thread의 resource 해제
	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}

	// sum
	sum = 0;
	for (i = 0 ; i < 30 ; i++) {
		sum += stat[i];
	}
	// print out distributions
	printf("*** print out distributions *** \n");
	printf("  #ch  freq \n");
	for (i = 0 ; i < 30 ; i++) {
		int j = 0;
		int num_star = stat[i]*80/sum;
		printf("[%3d]: %4d \t", i+1, stat[i]);
		for (j = 0 ; j < num_star ; j++)
				printf("*");
		printf("\n");
	}
	printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
	printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
				stat2['A']+stat2['a'], stat2['B']+stat2['b'],  stat2['C']+stat2['c'],  stat2['D']+stat2['d'],  stat2['E']+stat2['e'],
				stat2['F']+stat2['f'], stat2['G']+stat2['g'],  stat2['H']+stat2['h'],  stat2['I']+stat2['i'],  stat2['J']+stat2['j'],
				stat2['K']+stat2['k'], stat2['L']+stat2['l'],  stat2['M']+stat2['m'],  stat2['N']+stat2['n'],  stat2['O']+stat2['o'],
				stat2['P']+stat2['p'], stat2['Q']+stat2['q'],  stat2['R']+stat2['r'],  stat2['S']+stat2['s'],  stat2['T']+stat2['t'],
				stat2['U']+stat2['u'], stat2['V']+stat2['v'],  stat2['W']+stat2['w'],  stat2['X']+stat2['x'],  stat2['Y']+stat2['y'],
				stat2['Z']+stat2['z']);
	printf("total line: %ld\n", total);


    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond);

	pthread_exit(NULL);
	exit(0);

}