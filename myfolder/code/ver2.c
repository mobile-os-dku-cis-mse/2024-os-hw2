//작업단위를 늘린 버전 ver2 [제출용]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BATCH_SIZE 1000 //읽어들일 줄 수 설정
#define BUFFER_SIZE 100

typedef struct line_batch { // 기존의 한 줄 단위가 아닌 여러 줄을 읽기위해 새로운 구조체 도입
    char** lines;
    int count;  // 실제 배치에 포함된 라인 수
    int is_last;  // 마지막 배치 여부
} line_batch;

typedef struct sharing_obj {
    int product;
    int consume;
    FILE* rfile;
    int eof_reached;  // EOF 도달 여부//한번에 여러줄을 읽어야 하기 때문

    int dashboard1[30];  // 문자열 길이
    int dashboard2[123];  // ASCII : A = 65로 시작, z = 122로 마지막
    line_batch* batches[BUFFER_SIZE];  // 배치 버퍼

    sem_t full;
    sem_t empty;
    pthread_mutex_t lock;
    pthread_mutex_t file_lock;  // 파일 읽기용 뮤텍스
    pthread_mutex_t dashboards_lock;  // 대시보드 업데이트용 뮤텍스
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

    if(argc == 1) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }

    rfile = fopen((char*)argv[1], "rb");
    if(rfile == NULL) {
        printf("file error");
        exit(0);
    }

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
    sp->eof_reached = 0;

    sem_init(&sp->full, 0, 0);
    sem_init(&sp->empty, 0, BUFFER_SIZE);
    pthread_mutex_init(&sp->lock, NULL);
    pthread_mutex_init(&sp->file_lock, NULL);
    pthread_mutex_init(&sp->dashboards_lock, NULL);

    clock_gettime(CLOCK_MONOTONIC, &start);

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
        free(ret);
    }

    for(i = 0; i < proN; i++) {
        rc = pthread_join(pro[i], (void**)&ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
        free(ret);
    }

    statistics(sp);

    // 리소스 정리
    pthread_mutex_destroy(&sp->lock);
    pthread_mutex_destroy(&sp->file_lock);
    pthread_mutex_destroy(&sp->dashboards_lock);
    sem_destroy(&sp->full);
    sem_destroy(&sp->empty);
    fclose(sp->rfile);
    free(sp);

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Elapsed time: %.2f seconds\n", elapsed_time);

    return 0;
}

void* producer(void* sharing_obj) {
    so* sp = sharing_obj;
    ssize_t contents;
    char* line = NULL;
    size_t len = 0;
    int* ret = malloc(sizeof(int));
    *ret = 0;

    while(1) {
        // EOF에 도달했는지 확인
        pthread_mutex_lock(&sp->file_lock);
        if(sp->eof_reached) {
            pthread_mutex_unlock(&sp->file_lock);
            break;
        }
        pthread_mutex_unlock(&sp->file_lock);

        // 새 배치 생성
        line_batch* current_batch = malloc(sizeof(line_batch));
        current_batch->lines = malloc(sizeof(char*) * BATCH_SIZE);
        current_batch->count = 0;
        current_batch->is_last = 0;

        // 배치 크기만큼 라인 읽기
        pthread_mutex_lock(&sp->file_lock);
        for(int i = 0; i < BATCH_SIZE; i++) {
            contents = getdelim(&line, &len, '\n', sp->rfile);
            if(contents == -1) {
                current_batch->is_last = 1;
                sp->eof_reached = 1;
                break;
            }
            current_batch->lines[i] = strdup(line);
            current_batch->count++;
            (*ret)++;
        }
        pthread_mutex_unlock(&sp->file_lock);

        // 배치가 비어있고 마지막이면 종료
        if(current_batch->count == 0) {
            free(current_batch->lines);
            free(current_batch);
            break;
        }

        // 배치를 버퍼에 추가
        sem_wait(&sp->empty);
        pthread_mutex_lock(&sp->lock);
        
        sp->batches[sp->product] = current_batch;
        sp->product = (sp->product + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->full);

        if(line) {
            free(line);
            line = NULL;
        }
    }

    // 다른 producer들에게 알리기 위해 빈 배치 추가
    if(sp->eof_reached) {
        sem_wait(&sp->empty);
        pthread_mutex_lock(&sp->lock);
        
        line_batch* empty_batch = malloc(sizeof(line_batch));
        empty_batch->lines = malloc(sizeof(char*));
        empty_batch->count = 0;
        empty_batch->is_last = 1;
        
        sp->batches[sp->product] = empty_batch;
        sp->product = (sp->product + 1) % BUFFER_SIZE;
        
        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->full);
    }

    return ret;
}

void* consumer(void* sharing_obj) {
    so* sp = sharing_obj;
    int* ret = malloc(sizeof(int));
    *ret = 0;
    int last_batch_seen = 0;

    while(!last_batch_seen) {
        sem_wait(&sp->full);
        pthread_mutex_lock(&sp->lock);

        line_batch* current_batch = sp->batches[sp->consume];
        sp->consume = (sp->consume + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->empty);

        // 빈 마지막 배치 확인
        if(current_batch->count == 0 && current_batch->is_last) {
            free(current_batch->lines);
            free(current_batch);
            last_batch_seen = 1;
            continue;
        }

        // 각 배치의 라인들을 처리
        for(int i = 0; i < current_batch->count; i++) {
            char* line = current_batch->lines[i];
            
            // 문자열 처리를 위한 복사본 생성
            char* cptr = strdup(line);
            char* brka = NULL;
            char* substr = NULL;
            const char* sep = "{}()[],;\" \n\t^";

            // 토큰화하여 각 단어 처리
            for(substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
                size_t length = strlen(substr);
                if(length >= 30) length = 30;
                
                // 대시보드 업데이트를 위한 락 획득
                pthread_mutex_lock(&sp->dashboards_lock);
                
                // 문자열 길이 카운팅
                sp->dashboard1[length-1]++;
                
                // 각 문자 처리
                for(int j = 0; substr[j] != '\0'; j++) {
                    char c = substr[j];
                    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                        sp->dashboard2[c]++;
                    }
                }
                
                pthread_mutex_unlock(&sp->dashboards_lock);
            }

            free(cptr);
            free(line);
            (*ret)++;
        }

        // 마지막 배치 체크
        if(current_batch->is_last) {
            last_batch_seen = 1;
        }

        // 배치 메모리 해제
        free(current_batch->lines);
        free(current_batch);
    }

    return ret;
}

void statistics(so* sp) {
    // sum
    int sum = 0;
    for(int i = 0; i < 30; i++) sum += sp->dashboard1[i];
    
    // print out the distributions
    printf("\n");
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");
    for(int i = 0; i < 30; i++) {
        int num_star = sp->dashboard1[i] * 80 / (sum == 0 ? 1 : sum);
        printf("[%3d]: %10d \t", i + 1, sp->dashboard1[i]);
        for(int j = 0; j < num_star; j++) printf("*");
        printf("\n");
    }
    printf("\n");

    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
            sp->dashboard2['A']+sp->dashboard2['a'], sp->dashboard2['B']+sp->dashboard2['b'],  sp->dashboard2['C']+sp->dashboard2['c'],  sp->dashboard2['D']+sp->dashboard2['d'],  sp->dashboard2['E']+sp->dashboard2['e'],
            sp->dashboard2['F']+sp->dashboard2['f'], sp->dashboard2['G']+sp->dashboard2['g'],  sp->dashboard2['H']+sp->dashboard2['h'],  sp->dashboard2['I']+sp->dashboard2['i'],  sp->dashboard2['J']+sp->dashboard2['j'],
            sp->dashboard2['K']+sp->dashboard2['k'], sp->dashboard2['L']+sp->dashboard2['l'],  sp->dashboard2['M']+sp->dashboard2['m'],  sp->dashboard2['N']+sp->dashboard2['n'],  sp->dashboard2['O']+sp->dashboard2['o'],
            sp->dashboard2['P']+sp->dashboard2['p'], sp->dashboard2['Q']+sp->dashboard2['q'],  sp->dashboard2['R']+sp->dashboard2['r'],  sp->dashboard2['S']+sp->dashboard2['s'],  sp->dashboard2['T']+sp->dashboard2['t'],
            sp->dashboard2['U']+sp->dashboard2['u'], sp->dashboard2['V']+sp->dashboard2['v'],  sp->dashboard2['W']+sp->dashboard2['w'],  sp->dashboard2['X']+sp->dashboard2['x'],  sp->dashboard2['Y']+sp->dashboard2['y'],
            sp->dashboard2['Z']+sp->dashboard2['z']);
}