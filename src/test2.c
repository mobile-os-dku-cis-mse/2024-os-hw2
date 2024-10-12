#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256

int stat [MAX_STRING_LENGTH];
int stat2 [ASCII_SIZE];

/*
line 변수
현재 소비해야할 줄이 있는지 여부
소비자가 NULL로 설정

full 변수
줄의 소비 가능 여부
소비자가 NULL로 설정 후 바로 다른 소비자가 동작하여 비정상적인 종료 방지
*/

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cond_prod;
	pthread_cond_t cond_cons;
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

		while (so->line != NULL) pthread_cond_wait(&so->cond_prod, &so->lock);

		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
    		so->full = 1;
    		pthread_cond_broadcast(&so->cond_cons);
    		pthread_mutex_unlock(&so->lock);
    		break;
		}
		so->linenum = i;
		so->line = strdup(line);
		so->full = 1;
        i++;

		pthread_cond_broadcast(&so->cond_cons);
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

		while (!so->full) pthread_cond_wait(&so->cond_cons, &so->lock);

		line = so->line;
		if (line == NULL && so->full == 1) {
			pthread_cond_broadcast(&so->cond_prod);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);

		so->line = NULL;
		so->full = 0;
		pthread_cond_broadcast(&so->cond_prod);
		pthread_mutex_unlock(&so->lock);

		char *line_copy = strdup(line);
		free(line);
		i++;

		char *cptr = line_copy;
    	char *substr = NULL;
    	char *brka = NULL;
    	char *sep = "{}()[],;\" \n\t^";

		for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
			size_t length = strlen(substr);
			if (length >= 30) length = 30;

			pthread_mutex_lock(&so->lock);
			stat[length-1]++;
			pthread_mutex_unlock(&so->lock);

			// number of the character in the sub-string
			for (int i = 0 ; i < length ; i++) {
				if (*cptr < 256 && *cptr > 1) {
					pthread_mutex_lock(&so->lock);
                    stat2[*cptr]++;
					pthread_mutex_unlock(&so->lock);
                }
				cptr++;
			}
			cptr++;
			if (*cptr == '\0') break;
		}
		free(line_copy); // strdup() 복사본 해제
	}
	printf("Cons_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[]) {
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;
	int *ret;
	int i;
	FILE *rfile;

	if (argc < 2) {
		printf("usage: ./prod_cons <readfile> [#Producer] [#Consumer]\n");
		exit (0);
	}

	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}

	Nprod = (argc > 2) ? atoi(argv[2]) : 1;
    Ncons = (argc > 3) ? atoi(argv[3]) : 1;

    Nprod = (Nprod > 100) ? 100 : Nprod;
    Ncons = (Ncons > 100) ? 100 : Ncons;

	share->rfile = rfile;
	share->line = NULL;

	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->cond_prod, NULL);
	pthread_cond_init(&share->cond_cons, NULL);

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		// printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		// printf("main: producer_%d joined with %d\n", i, *ret);
	}

    printf("\nFinal Alphabet Count:\n");
    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['A'] + stat2['a'], stat2['B'] + stat2['b'], stat2['C'] + stat2['c'],
		   stat2['D'] + stat2['d'], stat2['E'] + stat2['e'], stat2['F'] + stat2['f'],
		   stat2['G'] + stat2['g'], stat2['H'] + stat2['h'], stat2['I'] + stat2['i'],
		   stat2['J'] + stat2['j'], stat2['K'] + stat2['k'], stat2['L'] + stat2['l'],
		   stat2['M'] + stat2['m']);
    
    printf("       N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['N'] + stat2['n'], stat2['O'] + stat2['o'], stat2['P'] + stat2['p'],
		   stat2['Q'] + stat2['q'], stat2['R'] + stat2['r'], stat2['S'] + stat2['s'],
		   stat2['T'] + stat2['t'], stat2['U'] + stat2['u'], stat2['V'] + stat2['v'],
		   stat2['W'] + stat2['w'], stat2['X'] + stat2['x'], stat2['Y'] + stat2['y'],
		   stat2['Z'] + stat2['z']);
    
	pthread_mutex_destroy(&share->lock);
	pthread_cond_destroy(&share->cond_prod);
	pthread_cond_destroy(&share->cond_cons);
    fclose(rfile);
    free(share);

	pthread_exit(NULL);
	exit(0);
}
