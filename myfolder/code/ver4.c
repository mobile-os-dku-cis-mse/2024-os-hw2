//// 작업 단위를 1줄에서 1000줄 이상으로 늘려 동기화 빈도 최소화한 버전(최종 버전) [제출용]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 700  // 버퍼당 1000줄 저장 --> 만줄에서 2.55초 최고 기록, 십만줄에서 3.17초// 최대 천만줄까지
#define NUM_BUFFERS 100   // 총 버퍼 개수

typedef struct sharing_obj {
    int product;          // 다음에 저장할 버퍼 번호
    int consume;          // 소비할 버퍼 번호
    int lines_in_buffer[NUM_BUFFERS];  // 각 버퍼에 저장된 라인 수
    FILE* rfile;

    int dashboard1[NUM_BUFFERS][30];    // 문자열 길이 (스레드당 각각)
    int dashboard2[NUM_BUFFERS][123];   // ASCII 문자 빈도 (스레드당 각각)
    char* lines[NUM_BUFFERS][BUFFER_SIZE]; // 1000줄 단위로 저장하는 2차원 버퍼

    sem_t full;
    sem_t empty;
    pthread_mutex_t lock;
} so;

void* producer(void* sharing_obj);
void* consumer(void* sharing_obj);
void statistics(so* sp);

int main(int argc, char* argv[]) {
    FILE* rfile;
    int proN, conN;
    int i;
    int* ret;
    int rc;
    pthread_t pro[100];
    pthread_t con[100];
    struct timespec start, end;
    double elapsed_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start);

    if(argc == 1) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    rfile = fopen((char*)argv[1], "rb");
    if(rfile == NULL) {
        printf("file error\n");
        exit(0);
    }

    // Producer, Consumer 개수 설정 (기존과 동일)
    if(argv[2] != NULL) {
        proN = atoi(argv[2]);
        if(proN > 100) proN = 100;
        else if(proN == 0) proN = 1;
    } else {
        proN = 1;
    }

    if(argv[3] != NULL) {
        conN = atoi(argv[3]);
        if(conN > 100) conN = 100;
        else if(conN == 0) conN = 1;
    } else {
        conN = 1;
    }

    so *sp = malloc(sizeof(so));
    memset(sp, 0, sizeof(so));
    sp->rfile = rfile;
    
    sem_init(&sp->full, 0, 0);
    sem_init(&sp->empty, 0, NUM_BUFFERS);
    pthread_mutex_init(&sp->lock, NULL);


    printf("main continuing\n");
    for(i = 0; i < proN; i++) {
        pthread_create(&pro[i], NULL, producer, sp);
    }

    for(i = 0; i < conN; i++) {
        pthread_create(&con[i], NULL, consumer, sp);
    }

    for(i = 0; i < conN; i++) {
        rc = pthread_join(con[i], (void**)&ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
    }

    for(i = 0; i < proN; i++) {
        rc = pthread_join(pro[i], (void**)&ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }

    statistics(sp);

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Elapsed time: %.2f seconds\n", elapsed_time);

    pthread_exit(NULL);
    exit(0);
}

void* producer(void* sharing_obj) {
    so* sp = sharing_obj;
    ssize_t contents = 0;
    char* line = NULL;
    size_t len = 0;
    int count = 0;
    int* ret = malloc(sizeof(int));
    int lines_read = 0;
    int current_buffer = 0;
    
    while(1) {
        sem_wait(&sp->empty);
        pthread_mutex_lock(&sp->lock);
        
        current_buffer = sp->product;
        lines_read = 0;

        
        while(lines_read < BUFFER_SIZE) {
            contents = getdelim(&line, &len, '\n', sp->rfile);
            if(contents == -1) {
                break;
            }
            sp->lines[current_buffer][lines_read] = strdup(line);
            lines_read++;
            count++;
        }

        if(lines_read == 0) {
            // EOF에 도달한 경우
            sp->lines_in_buffer[current_buffer] = -1;  // EOF 표시
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->full);
            break;
        }

        sp->lines_in_buffer[current_buffer] = lines_read;
        sp->product = (sp->product + 1) % NUM_BUFFERS;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->full);

        if(contents == -1) break;  // EOF 도달 시 종료
    }

    free(line);
    *ret = count;
    pthread_exit(ret);
}

void* consumer(void* sharing_obj) {
    so* sp = sharing_obj;
    int count = 0;
    int* ret = malloc(sizeof(int));
    
    while(1) {
        sem_wait(&sp->full);
        pthread_mutex_lock(&sp->lock);

        int current_buffer = sp->consume;
        int lines_in_current_buffer = sp->lines_in_buffer[current_buffer];

        if(lines_in_current_buffer == -1) {
            // EOF 표시된 버퍼 발견
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->empty);
            sem_post(&sp->full);
            break;
        }

        sp->consume = (sp->consume + 1) % NUM_BUFFERS;
        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->empty);

        // 버퍼의 모든 라인 처리
        for(int line_idx = 0; line_idx < lines_in_current_buffer; line_idx++) {
            char* line = sp->lines[current_buffer][line_idx];
            if(!line) continue;

            char* cptr = strdup(line);
            char* brka = NULL;
            char* substr = NULL;
            const char* sep = "{}()[],;\" \n\t^";

            for(substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
                size_t length = strlen(substr);
                if(length >= 30) length = 30;
                
                sp->dashboard1[current_buffer][length-1]++;
                
                for(int i = 0; substr[i] != '\0'; i++) {
                    char c = substr[i];
                    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                        sp->dashboard2[current_buffer][c]++;
                    }
                }
            }
            free(cptr);
            free(line);  // 처리 완료된 라인 메모리 해제
            count++;
        }
    }

    *ret = count;
    pthread_exit(ret);
}

void statistics(so* sp) {
    int total_dashboard1[30] = {0};
    int total_dashboard2[123] = {0};

    // 버퍼별 통계 합산
    for(int i = 0; i < NUM_BUFFERS; i++) {
        for(int j = 0; j < 30; j++) {
            total_dashboard1[j] += sp->dashboard1[i][j];
        }
        for(int j = 65; j <= 122; j++) {
            total_dashboard2[j] += sp->dashboard2[i][j];
        }
    }

    // 문자열 길이 분포 출력
    int sum = 0;
    for(int i = 0; i < 30; i++) {
        sum += total_dashboard1[i];
    }

    printf("\n*** print out distributions ***\n");
    printf("  #ch  freq\n");
    for(int i = 0; i < 30; i++) {
        int num_star = total_dashboard1[i] * 80 / (sum == 0 ? 1 : sum);
        printf("[%3d]: %10d \t", i + 1, total_dashboard1[i]);
        for(int j = 0; j < num_star; j++) {
            printf("*");
        }
        printf("\n");
    }
    printf("\n");

    // 알파벳 빈도 출력
    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
        total_dashboard2['A'] + total_dashboard2['a'],
        total_dashboard2['B'] + total_dashboard2['b'],
        total_dashboard2['C'] + total_dashboard2['c'],
        total_dashboard2['D'] + total_dashboard2['d'],
        total_dashboard2['E'] + total_dashboard2['e'],
        total_dashboard2['F'] + total_dashboard2['f'],
        total_dashboard2['G'] + total_dashboard2['g'],
        total_dashboard2['H'] + total_dashboard2['h'],
        total_dashboard2['I'] + total_dashboard2['i'],
        total_dashboard2['J'] + total_dashboard2['j'],
        total_dashboard2['K'] + total_dashboard2['k'],
        total_dashboard2['L'] + total_dashboard2['l'],
        total_dashboard2['M'] + total_dashboard2['m'],
        total_dashboard2['N'] + total_dashboard2['n'],
        total_dashboard2['O'] + total_dashboard2['o'],
        total_dashboard2['P'] + total_dashboard2['p'],
        total_dashboard2['Q'] + total_dashboard2['q'],
        total_dashboard2['R'] + total_dashboard2['r'],
        total_dashboard2['S'] + total_dashboard2['s'],
        total_dashboard2['T'] + total_dashboard2['t'],
        total_dashboard2['U'] + total_dashboard2['u'],
        total_dashboard2['V'] + total_dashboard2['v'],
        total_dashboard2['W'] + total_dashboard2['w'],
        total_dashboard2['X'] + total_dashboard2['x'],
        total_dashboard2['Y'] + total_dashboard2['y'],
        total_dashboard2['Z'] + total_dashboard2['z']
    );
}