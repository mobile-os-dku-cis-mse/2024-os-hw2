//// Ver.3 기본형 + 동기화 최적화[제출용]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_BUFFERS 100

typedef struct sharing_obj {
    int product;          // 다음에 저장할 버퍼 번호
    int consume;          // 소비할 버퍼 번호
    FILE* rfile;

    int dashboard1[NUM_BUFFERS][30];    // 문자열 길이 (스레드별) 최대 스레드 100개이므로 100개 공간 마련
    int dashboard2[NUM_BUFFERS][123];   // ASCII 문자 빈도 (스레드별) 최대 스레드 100개이므로 100개 공간 마련
    char* lines[100];                   // 버퍼 배열

    sem_t full;
    sem_t empty;
    pthread_mutex_t lock;
} so;

void* producer(void* sharing_obj);
void* consumer(void* sharing_obj);
void statistics(so* sp, int conN);

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

    // 시간 측정 시작
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

    // Producer, Consumer 개수 설정
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
    sem_init(&sp->empty, 0, 100);
    pthread_mutex_init(&sp->lock, NULL);

    printf("main continuing\n");
    for(i = 0; i < proN; i++) {
        pthread_create(&pro[i], NULL, producer, sp);
    }

    // consumer 스레드에 ID 전달을 위한 구조체
    struct {
        so* sp;
        int id;
    } *thread_args[NUM_BUFFERS];

    for(i = 0; i < conN; i++) {
        thread_args[i] = malloc(sizeof(*thread_args[i]));
        thread_args[i]->sp = sp;
        thread_args[i]->id = i;
        pthread_create(&con[i], NULL, consumer, thread_args[i]);
    }

    for(i = 0; i < conN; i++) {
        rc = pthread_join(con[i], (void**)&ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
        free(thread_args[i]);
    }

    for(i = 0; i < proN; i++) {
        rc = pthread_join(pro[i], (void**)&ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }

    statistics(sp, conN);

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
    
    while(1) {
        sem_wait(&sp->empty);
        pthread_mutex_lock(&sp->lock);

        // 파일에서 한 줄 읽기
        contents = getdelim(&line, &len, '\n', sp->rfile);
        if(contents == -1) {
            // EOF 도달 시 처리
            sp->lines[sp->product] = NULL;
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->full);
            break;
        }

        sp->lines[sp->product] = strdup(line);
        sp->product = (sp->product + 1) % NUM_BUFFERS;
        count++;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->full);
    }
    
    free(line);
    *ret = count;
    pthread_exit(ret);
}

void* consumer(void* arg) {
    struct {
        so* sp;
        int id;
    } *thread_arg = arg;
    
    so* sp = thread_arg->sp;
    int thread_id = thread_arg->id;
    
    char* line = NULL;
    int count = 0;
    int* ret = malloc(sizeof(int));

    while(1) {
        sem_wait(&sp->full);
        pthread_mutex_lock(&sp->lock);

        // 버퍼에서 데이터 가져오기
        line = sp->lines[sp->consume];
        if(line == NULL) {
            // EOF 표시된 버퍼 발견
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->empty);
            sem_post(&sp->full);
            break;
        }

        sp->consume = (sp->consume + 1) % NUM_BUFFERS;
        count++;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->empty);
        
        // 문자열 처리를 위한 복사본 생성
        char* cptr = strdup(line);
        char* brka = NULL;
        char* substr = NULL;
        const char* sep = "{}()[],;\" \n\t^";

        // 토큰화하여 각 단어 처리 - thread_id에 해당하는 배열에 저장
        for(substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
            size_t length = strlen(substr);
            if(length >= 30) length = 30;
            
            sp->dashboard1[thread_id][length-1]++;
            
            for(int i = 0; substr[i] != '\0'; i++) {
                char c = substr[i];
                if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    sp->dashboard2[thread_id][c]++;
                }
            }
        }

        free(cptr);
        free(line);  // 처리 완료된 라인 메모리 해제

    }
    
    *ret = count;
    pthread_exit(ret);
}

void statistics(so* sp, int conN) {
    // 모든 스레드의 결과를 합산할 배열
    int total_dashboard1[30] = {0};
    int total_dashboard2[123] = {0};
    
    // 각 스레드의 결과 합산
    for(int t = 0; t < conN; t++) {
        for(int i = 0; i < 30; i++) {
            total_dashboard1[i] += sp->dashboard1[t][i];
        }
        for(int i = 65; i <= 122; i++) {  // 알파벳 문자 범위만 처리
            total_dashboard2[i] += sp->dashboard2[t][i];
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
            total_dashboard2['A']+total_dashboard2['a'], 
            total_dashboard2['B']+total_dashboard2['b'],  
            total_dashboard2['C']+total_dashboard2['c'],  
            total_dashboard2['D']+total_dashboard2['d'],  
            total_dashboard2['E']+total_dashboard2['e'],
            total_dashboard2['F']+total_dashboard2['f'], 
            total_dashboard2['G']+total_dashboard2['g'],  
            total_dashboard2['H']+total_dashboard2['h'],  
            total_dashboard2['I']+total_dashboard2['i'],  
            total_dashboard2['J']+total_dashboard2['j'],
            total_dashboard2['K']+total_dashboard2['k'], 
            total_dashboard2['L']+total_dashboard2['l'],  
            total_dashboard2['M']+total_dashboard2['m'],  
            total_dashboard2['N']+total_dashboard2['n'],  
            total_dashboard2['O']+total_dashboard2['o'],
            total_dashboard2['P']+total_dashboard2['p'], 
            total_dashboard2['Q']+total_dashboard2['q'],  
            total_dashboard2['R']+total_dashboard2['r'],  
            total_dashboard2['S']+total_dashboard2['s'],  
            total_dashboard2['T']+total_dashboard2['t'],
            total_dashboard2['U']+total_dashboard2['u'], 
            total_dashboard2['V']+total_dashboard2['v'],  
            total_dashboard2['W']+total_dashboard2['w'],  
            total_dashboard2['X']+total_dashboard2['x'],  
            total_dashboard2['Y']+total_dashboard2['y'],
            total_dashboard2['Z']+total_dashboard2['z']);
}