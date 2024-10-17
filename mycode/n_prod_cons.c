#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 30
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256

int stat[MAX_STRING_LENGTH];
int stat2[ASCII_SIZE];
int count = 0;

typedef struct sharedobject {
    char *lines[BUF_SIZE];
    int full[BUF_SIZE];
    int done;
    int in;
    int out;
    pthread_mutex_t lock;
    pthread_cond_t cond_prod;
    pthread_cond_t cond_cons;
} so_t;

so_t share;
FILE *rfile;
int Nprod, Ncons;

void *producer(void *arg) {
    int *ret = malloc(sizeof(int));
    int i = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while (1) {
        pthread_mutex_lock(&share.lock); // 임계 영역 시작
        read = getline(&line, &len, rfile);
        if (read == -1) {
            share.done = 1;
            pthread_cond_broadcast(&share.cond_cons);
            pthread_mutex_unlock(&share.lock); // 임계 영역 끝
            break;
        }
        pthread_mutex_unlock(&share.lock); // 임계 영역 끝

        pthread_mutex_lock(&share.lock);
        while (share.full[share.in]) {
            pthread_cond_wait(&share.cond_prod, &share.lock);
        }
        
        share.lines[share.in] = strdup(line);
        share.full[share.in] = 1;
        share.in = (share.in + 1) % BUF_SIZE;
        i++;

        pthread_cond_broadcast(&share.cond_cons);
        pthread_mutex_unlock(&share.lock);
    }
    free(line);
    *ret = i;
    pthread_exit(ret);
}


void *consumer(void *arg) {
    int *ret = malloc(sizeof(int));
    int i = 0;
    char *line;

	//char_stat.c 변수 추가
	char *cptr = NULL;
	char *substr = NULL;
	char *brka = NULL;
	char *sep = "{}()[],;\" \n\t^";

    while (1) {
        pthread_mutex_lock(&share.lock);
        while (!share.full[share.out] && !share.done) {
            pthread_cond_wait(&share.cond_cons, &share.lock);
        }

        if (share.done && !share.full[share.out]) {
            pthread_mutex_unlock(&share.lock);
            break;
        }

        line = share.lines[share.out];
        share.lines[share.out] = NULL;
        share.full[share.out] = 0;
        share.out = (share.out + 1) % BUF_SIZE;
		count++;

        pthread_cond_broadcast(&share.cond_prod);
        pthread_mutex_unlock(&share.lock);

        // Process the line
        char *line_copy = strdup(line);
		cptr = line_copy;
		
#ifdef _IO_
		printf("[%3d] %s\n", line_num++, line);
#endif
		pthread_mutex_lock(&share.lock);
		for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
			size_t length = strlen(substr);
			// update stats

			// length of the sub-string
			if (length >= 30) length = 30;
#ifdef _IO_
			printf("length: %d\n", (int)length);
#endif
			stat[length-1]++;

			// number of the character in the sub-string
			for (int i = 0 ; i < length ; i++) {
				if (*cptr < 256 && *cptr > 1) {
					stat2[*cptr]++;
#ifdef _IO_
					printf("# of %c(%d): %d\n", *cptr, *cptr, stat2[*cptr]);
#endif
				}
				cptr++;
			}
			cptr++;
			if (*cptr == '\0') break;
		}
		pthread_mutex_unlock(&share.lock);

		free(line_copy); // strdup() 복사본 해제
        free(line);
        i++;
    }
    *ret = i;
    pthread_exit(ret);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    rfile = fopen(argv[1], "rb");
    if (rfile == NULL) {
        perror("rfile");
        exit(0);
    }

    Nprod = atoi(argv[2]);
    Ncons = atoi(argv[3]);

    if (Nprod > 100) Nprod = 100;
    if (Nprod == 0) Nprod = 1;
    if (Ncons > 100) Ncons = 100;
    if (Ncons == 0) Ncons = 1;

    pthread_t prod[Nprod], cons[Ncons];

    // Initialize shared object
    memset(&share, 0, sizeof(so_t));
    pthread_mutex_init(&share.lock, NULL);
    pthread_cond_init(&share.cond_prod, NULL);
    pthread_cond_init(&share.cond_cons, NULL);

    // Create threads
    for (int i = 0; i < Nprod; i++)
        pthread_create(&prod[i], NULL, producer, NULL);
    for (int i = 0; i < Ncons; i++)
        pthread_create(&cons[i], NULL, consumer, NULL);
    printf("main continuing\n");

    // Join threads
    for (int i = 0; i < Ncons; i++) {
        int *ret;
        pthread_join(cons[i], (void **)&ret);
        printf("Consumer %d processed %d lines\n", i, *ret);
        free(ret);
    }
    for (int i = 0; i < Nprod; i++) {
        int *ret;
        pthread_join(prod[i], (void **)&ret);
        printf("Producer %d produced %d lines\n", i, *ret);
        free(ret);
    }

    // Clean up
    pthread_mutex_destroy(&share.lock);
    pthread_cond_destroy(&share.cond_prod);
    pthread_cond_destroy(&share.cond_cons);
    printf("Thread destroy\n");

    fclose(rfile);

    // sum
    int sum = 0;
    for (int i = 0 ; i < 30 ; i++) {
        sum += stat[i];
    }
    // print out distributions
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");
    for (int i = 0 ; i < 30 ; i++) {
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
	printf("count: %d\n", count);
    return 0;
} //21262918
  //24461614
  //21262919