#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256

int done = 0;
FILE *write_fp;

// 공유 데이터 구조체
typedef struct sharedobject
{
    char *line;                // 버퍼에 저장된 라인
    int linenum;               // 현재 라인 번호
    pthread_mutex_t lock;      // 뮤텍스
    pthread_cond_t full_cond;  // 버퍼가 가득 찼을 때
    pthread_cond_t empty_cond; // 버퍼가 비었을 때
    int full;                  // 버퍼가 차 있는지 여부
    int prod_count;            // 남은 생산자의 수
    int rfile;                 // 파일 핸들러
} so_t;

// 스레드 인자 구조체
typedef struct thread_arg
{
    int id;
    so_t *shared;
} thread_arg_t;

int stat1[MAX_STRING_LENGTH] = {0}; // 단어 길이 통계
int stat2[ASCII_SIZE] = {0};        // 문자 빈도 통계
pthread_mutex_t stats_lock;         // 통계를 보호할 뮤텍스

// 통계 업데이트 함수
void update_stats(const char *line) {
    char *substr, *brka;
    char *cptr = strdup(line);         
    char *sep = " {}()[],;\" \n\t^.-"; 

    pthread_mutex_lock(&stats_lock); 

    for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
        size_t length = strlen(substr);
        if (length >= MAX_STRING_LENGTH)
            length = MAX_STRING_LENGTH;
        stat1[length - 1]++;

        // 문자 빈도 통계 업데이트
        for (size_t i = 0; i < length; i++) {
            if (isalnum((unsigned char)substr[i])) {
                stat2[(unsigned char)substr[i]]++;
            }
        }
    }

    pthread_mutex_unlock(&stats_lock); 
    free(cptr);                        
}

// 수집된 통계 출력 함수
void print_stats() {
    int sum = 0;
    for (int i = 0; i < MAX_STRING_LENGTH; i++) {
        sum += stat1[i];
    }

    printf("*** 길이 분포 *** \n");
    printf("  #ch  빈도 \n");
    for (int i = 0; i < MAX_STRING_LENGTH; i++) {
        int num_star = stat1[i] * 80 / sum;
        printf("[%3d]: %4d \t", i + 1, stat1[i]);
        for (int j = 0; j < num_star; j++)
            printf("*");
        printf("\n");
    }

    fprintf(write_fp, "*** 길이 분포 *** \n");
    fprintf(write_fp, "  #ch  빈도 \n");
    for (int i = 0; i < MAX_STRING_LENGTH; i++) {
        int num_star = stat1[i] * 80 / sum;
        fprintf(write_fp, "[%3d]: %4d \t", i + 1, stat1[i]);
        for (int j = 0; j < num_star; j++)
            fprintf(write_fp, "*");
        fprintf(write_fp, "\n");
    }

    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['A'] + stat2['a'], stat2['B'] + stat2['b'], stat2['C'] + stat2['c'], stat2['D'] + stat2['d'], stat2['E'] + stat2['e'],
           stat2['F'] + stat2['f'], stat2['G'] + stat2['g'], stat2['H'] + stat2['h'], stat2['I'] + stat2['i'], stat2['J'] + stat2['j'],
           stat2['K'] + stat2['k'], stat2['L'] + stat2['l'], stat2['M'] + stat2['m'], stat2['N'] + stat2['n'], stat2['O'] + stat2['o'],
           stat2['P'] + stat2['p'], stat2['Q'] + stat2['q'], stat2['R'] + stat2['r'], stat2['S'] + stat2['s'], stat2['T'] + stat2['t'],
           stat2['U'] + stat2['u'], stat2['V'] + stat2['v'], stat2['W'] + stat2['w'], stat2['X'] + stat2['x'], stat2['Y'] + stat2['y'],
           stat2['Z'] + stat2['z']);

    fprintf(write_fp, "       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    fprintf(write_fp, "%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
            stat2['A'] + stat2['a'], stat2['B'] + stat2['b'], stat2['C'] + stat2['c'], stat2['D'] + stat2['d'], stat2['E'] + stat2['e'],
            stat2['F'] + stat2['f'], stat2['G'] + stat2['g'], stat2['H'] + stat2['h'], stat2['I'] + stat2['i'], stat2['J'] + stat2['j'],
            stat2['K'] + stat2['k'], stat2['L'] + stat2['l'], stat2['M'] + stat2['m'], stat2['N'] + stat2['n'], stat2['O'] + stat2['o'],
            stat2['P'] + stat2['p'], stat2['Q'] + stat2['q'], stat2['R'] + stat2['r'], stat2['S'] + stat2['s'], stat2['T'] + stat2['t'],
            stat2['U'] + stat2['u'], stat2['V'] + stat2['v'], stat2['W'] + stat2['w'], stat2['X'] + stat2['x'], stat2['Y'] + stat2['y'],
            stat2['Z'] + stat2['z']);
}

// 프로듀서 스레드 함수
void *producer(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    so_t *so = targ->shared;
    char buffer[1024];

    while (1) {
        ssize_t byte_read = read(so->rfile, buffer, sizeof(buffer) - 1);
        if (byte_read == -1 || byte_read == 0) {
            pthread_mutex_lock(&so->lock);
            so->prod_count--;
            if (so->prod_count == 0) {
                done = 1;
                pthread_cond_broadcast(&so->full_cond);
            }
            pthread_mutex_unlock(&so->lock);
            break;
        }
        buffer[byte_read] = '\0';

        pthread_mutex_lock(&so->lock);
        while (so->full && !done)
            pthread_cond_wait(&so->empty_cond, &so->lock);

        so->line = strdup(buffer);
        so->linenum++;
        so->full = 1;
        pthread_cond_signal(&so->full_cond);
        pthread_mutex_unlock(&so->lock);
    }
    pthread_exit(NULL);
}

// 소비자 스레드 함수
void *consumer(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    so_t *so = targ->shared;

    while (1) {
        pthread_mutex_lock(&so->lock);

        if (done && !so->full) {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        while (!so->full && !done)
            pthread_cond_wait(&so->full_cond, &so->lock);

        if (so->line != NULL) {
            update_stats(so->line);
            free(so->line);
            so->line = NULL;
            so->full = 0;
            pthread_cond_signal(&so->empty_cond);
        }
        pthread_mutex_unlock(&so->lock);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: ./prod_cons <file> #Producer #Consumer\n");
        exit(0);
    }

    pthread_t prod_threads[100];
    pthread_t cons_threads[100];
    thread_arg_t prod_args[100];
    thread_arg_t cons_args[100];
    int Nprod = atoi(argv[2]), Ncons = atoi(argv[3]);
    struct timeval start, end;

    write_fp = fopen("result_mutex.txt", "w");

    so_t share;
    memset(&share, 0, sizeof(so_t));
    share.prod_count = Nprod;
    pthread_mutex_init(&share.lock, NULL);
    pthread_cond_init(&share.full_cond, NULL);
    pthread_cond_init(&share.empty_cond, NULL);
    share.rfile = open(argv[1], O_RDONLY);

    gettimeofday(&start, NULL);

    for (int i = 0; i < Nprod; i++) {
        prod_args[i].id = i;
        prod_args[i].shared = &share;
        pthread_create(&prod_threads[i], NULL, producer, &prod_args[i]);
    }

    for (int i = 0; i < Ncons; i++) {
        cons_args[i].id = i;
        cons_args[i].shared = &share;
        pthread_create(&cons_threads[i], NULL, consumer, &cons_args[i]);
    }

    for (int i = 0; i < Nprod; i++) pthread_join(prod_threads[i], NULL);
    for (int i = 0; i < Ncons; i++) pthread_join(cons_threads[i], NULL);

    gettimeofday(&end, NULL);

    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - start.tv_usec;
    printf("파일 처리 시간: %ld microseconds ( %lf sec )\n", micros, (double)micros / 1000000);

    print_stats();
    fclose(write_fp);
    pthread_mutex_destroy(&share.lock);
    pthread_cond_destroy(&share.full_cond);
    pthread_cond_destroy(&share.empty_cond);
    pthread_mutex_destroy(&stats_lock);

    close(share.rfile);
    return 0;
}
