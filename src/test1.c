#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256

int stat[MAX_STRING_LENGTH];
int stat2[ASCII_SIZE]; // 알파벳 통계

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
    int full;
} so_t;

void *producer(void *arg) {
    so_t *so = arg;
    FILE *rfile = so->rfile;
    int *ret = malloc(sizeof(int));
    int i = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while (1) {
        pthread_mutex_lock(&so->lock);

        // 기존 라인이 소비되지 않았을 경우 대기
        while (so->line != NULL) {
            pthread_mutex_unlock(&so->lock);
            usleep(100);
            pthread_mutex_lock(&so->lock);
        }

        read = getdelim(&line, &len, '\n', rfile);
        if (read == -1) {
            so->line = NULL;
            so->full = 1;
            pthread_mutex_unlock(&so->lock);
            break;
        }

        // 새 라인을 저장
        so->linenum++;
        so->line = strdup(line);
        so->full = 1;
        
        pthread_mutex_unlock(&so->lock);
        i++;
    }
    free(line);

    printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
    *ret = i;
    pthread_exit(ret);
}

void *consumer(void *arg) {
    so_t *so = arg;
    char *line;
    int *ret = malloc(sizeof(int));
    int i = 0;

    while (1) {
        pthread_mutex_lock(&so->lock);

        if (so->line == NULL && so->full == 1) {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        line = so->line;
        so->line = NULL;
		so->full = 0;
		pthread_mutex_unlock(&so->lock);

        if (line == NULL) continue;
        else i++;

        char *line_copy = strdup(line);
		free(line);

		//char_stat.c 변수
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
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen((char *)argv[1], "r");
    if (rfile == NULL) {
        perror("rfile");
        exit(0);
    }

    Nprod = (argc > 2) ? atoi(argv[2]) : 1;
    Ncons = (argc > 3) ? atoi(argv[3]) : 1;
    Nprod = (Nprod > 100) ? 100 : Nprod;
    Ncons = (Ncons > 100) ? 100 : Ncons;

    share->rfile = rfile;
    share->linenum = 0;
    share->line = NULL;
    share->full = 0;
    pthread_mutex_init(&share->lock, NULL);

    // pthread_create로 생성된 후 독립적으로 실행
    for (i = 0; i < Nprod; i++)
        pthread_create(&prod[i], NULL, producer, share);
    for (i = 0; i < Ncons; i++)
        pthread_create(&cons[i], NULL, consumer, share);

    /*
    생성 후...
    pthread_join은 각 스레드가 종료될 때까지 메인 스레드를 블록하는 역할
    스레드가 완료되지 않으면 메인 스레드는 그 이후의 코드(예: 다음 스레드 조인이나 이후 작업)를 실행 불가
    */
    for (i = 0; i < Ncons; i++) {
        rc = pthread_join(cons[i], (void **) &ret);
        /*
        메인 스레드는 cons[0] 소비자 스레드가 종료될 때까지 대기합니다.
        cons[0]가 작업을 마치고 종료되면, 그 스레드의 반환 값이 ret에 저장됩니다.
        */
        printf("main: consumer_%d joined\n", i);
    }
    for (i = 0; i < Nprod; i++) {
        rc = pthread_join(prod[i], (void **) &ret);
        /*
        메인 스레드는 prod[0] 생산자 스레드가 종료될 때까지 대기합니다.
        prod[0]가 작업을 마치고 종료되면, 그 스레드의 반환 값이 ret에 저장됩니다.
        */
        printf("main: producer_%d joined\n", i);
    }

    // 최종 알파벳 통계 출력
    printf("\n\nFinal Alphabet Count:\n");
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
    fclose(rfile);
    free(share);

    pthread_exit(NULL);
	exit(0);
}
