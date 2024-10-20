#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define BUFFSIZE 4096
#define ASCII_SIZE 256

typedef struct sharedobject {
    int nextin;
    int nextout;
    FILE* rfile;
    char* line[BUFFSIZE];

    pthread_mutex_t lock;
    pthread_cond_t cond_producer; // 생산자 조건 변수
    pthread_cond_t cond_consumer; // 소비자 조건 변수
    int full; // 버퍼에 채워진 항목 수
    int done; // 생산자 종료 플래그
    int stat[ASCII_SIZE]; // 알파벳 통계
} so_t;

void *producer(void *arg) {
    so_t *so = arg;
    size_t len = 0;
    ssize_t read = 0;
    char *line = NULL;
    int *ret = malloc(sizeof(int));
    int count = 0;

    while (1) {
        pthread_mutex_lock(&so->lock);

        // 버퍼가 가득 차면 대기
        while (so->full >= BUFFSIZE) {
            pthread_cond_wait(&so->cond_producer, &so->lock);
        }

        read = getdelim(&line, &len, '\n', so->rfile);
        if (read == -1) {
            so->done = 1; // 생산자가 완료되었음을 표시
            pthread_cond_broadcast(&so->cond_consumer); // 소비자에게 신호를 보냄
            printf("Producer done.\n");
            pthread_mutex_unlock(&so->lock);
            free(line); // 완료 시 line 메모리 해제
            break;
        }

        // 읽은 줄에 대한 메모리 할당
        char *new_line = strdup(line);
        if (new_line == NULL) {
            perror("Failed to allocate memory");
            free(line); // strdup 실패 시 line 해제
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->line[so->nextin] = new_line; // 줄을 버퍼에 저장
        so->nextin = (so->nextin + 1) % BUFFSIZE;        
        so->full++;
        count++;

        printf("Prod_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->full, new_line);
        
        pthread_cond_broadcast(&so->cond_consumer); // 소비자에게 신호를 보냄
        pthread_mutex_unlock(&so->lock);
    }

    *ret = count;
    pthread_exit(ret);
}

void *consumer(void *arg) {
    so_t *so = arg;
    int count = 0;
    char *line;
    int *ret = malloc(sizeof(int));

    while (1) {
        pthread_mutex_lock(&so->lock);

        // 데이터가 없으면 대기
        while (so->full <= 0 && !so->done) {
            pthread_cond_wait(&so->cond_consumer, &so->lock);
        }

        if (so->full <= 0 && so->done) { // 생산자가 완료되었고 버퍼가 비어 있으면
            pthread_mutex_unlock(&so->lock);
            break;
        }

        line = so->line[so->nextout];
        if (line == NULL) {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        // 소비한 줄의 문자 수 카운트 /* multi_cons에서 추가된 코드 */
        for (char *ptr = line; *ptr != '\0'; ptr++) {
            if (*ptr < ASCII_SIZE) {
                so->stat[(unsigned char)*ptr]++;
            }
        }

        // 소비한 줄 출력 및 메모리 해제
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->nextout, line);
        free(line);
        so->line[so->nextout] = NULL; // 포인터를 비워둠
        so->nextout = (so->nextout + 1) % BUFFSIZE;
        so->full--;
        count++;

        pthread_cond_broadcast(&so->cond_producer); // 생산자에게 신호를 보냄
        pthread_mutex_unlock(&so->lock);
    }

    *ret = count;
    pthread_exit(ret);
}

/* multi_cons에서 추가된 코드 */
void print_statistics(so_t *so) {
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");

    int sum = 0;
    for (int i = 0; i < ASCII_SIZE; i++) {
        sum += so->stat[i];
    }

    for (int i = 0; i < 30; i++) {
        int num_star = so->stat[i] * 80 / sum; // 막대 차트 생성을 위한 예시
        printf("[%3d]: %4d \t", i + 1, so->stat[i]);
        for (int j = 0; j < num_star; j++) {
            printf("*");
        }
        printf("\n");
    }
    
    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
            so->stat['A'] + so->stat['a'], so->stat['B'] + so->stat['b'],  so->stat['C'] + so->stat['c'],  so->stat['D'] + so->stat['d'],  so->stat['E'] + so->stat['e'],
            so->stat['F'] + so->stat['f'], so->stat['G'] + so->stat['g'],  so->stat['H'] + so->stat['h'],  so->stat['I'] + so->stat['i'],  so->stat['J'] + so->stat['j'],
            so->stat['K'] + so->stat['k'], so->stat['L'] + so->stat['l'],  so->stat['M'] + so->stat['m'],  so->stat['N'] + so->stat['n'],  so->stat['O'] + so->stat['o'],
            so->stat['P'] + so->stat['p'], so->stat['Q'] + so->stat['q'],  so->stat['R'] + so->stat['r'],  so->stat['S'] + so->stat['s'],  so->stat['T'] + so->stat['t'],
            so->stat['U'] + so->stat['u'], so->stat['V'] + so->stat['v'],  so->stat['W'] + so->stat['w'],  so->stat['X'] + so->stat['x'],  so->stat['Y'] + so->stat['y'],
            so->stat['Z'] + so->stat['z']);
}

int main (int argc, char *argv[]) {
    int *ret;
    int i;
    FILE *rfile;
    int Nprod, Ncons;
    pthread_t prod[100];
    pthread_t cons[100];

    if (argc < 4) {
        printf("usage: ./word_count_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    so_t *share = malloc(sizeof(so_t));
    memset(share, 0, sizeof(so_t));
    rfile = fopen(argv[1], "rb");

    if (rfile == NULL) {
        perror("rbfile");
        exit(0);
    }

    Nprod = atoi(argv[2]);
    Ncons = atoi(argv[3]);

    share->rfile = rfile;
    for (i = 0; i < BUFFSIZE; i++) share->line[i] = NULL;

    // 통계 배열 초기화 /* multi_cons에서 추가된 코드 */
    memset(share->stat, 0, sizeof(share->stat));

    // 동기화 프리미티브 초기화
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond_producer, NULL);
    pthread_cond_init(&share->cond_consumer, NULL);
    share->full = 0; // 버퍼의 초기 상태 설정
    share->done = 0; // 초기 종료 상태 설정
    share->nextin = 0; // 다음 입력 위치 초기화
    share->nextout = 0; // 다음 출력 위치 초기화

    // 생산자 스레드 생성
    for (i = 0; i < Nprod; i++) {
        pthread_create(&prod[i], NULL, producer, share);
    }

    // 소비자 스레드 생성
    for (i = 0; i < Ncons; i++) {
        pthread_create(&cons[i], NULL, consumer, share);
    }

    // 스레드 조인 및 결과 출력
    printf("\n\n");
    for (i = 0; i < Nprod; i++) {
        pthread_join(prod[i], (void **)&ret);
        printf("Producer %d created %d lines.\n", i, *ret);
        free(ret); // Producer result 메모리 해제
    }

    for (i = 0; i < Ncons; i++) {
        pthread_join(cons[i], (void **)&ret);
        printf("Consumer %d consumed %d lines.\n", i, *ret);
        free(ret); // Consumer result 메모리 해제
    }

    fclose(rfile);

    // 통계 출력
    print_statistics(share);

    // 자원 해제
    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond_producer);
    pthread_cond_destroy(&share->cond_consumer);
    free(share);

    return 0;
}
