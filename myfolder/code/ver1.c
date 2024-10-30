///////r8 ver.1(기본 버전) [제출용]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 

typedef struct sharing_obj{
    int product;
    int consume;
    FILE* rfile;

    int dashboard1[30]; //문자열 길이
    int dashboard2[123]; // ASCII : A = 65로 시작,  z = 122로 마지막 
    char* lines[100]; // 버퍼 개수(읽어온 라인 저장 포인터 배열)

    sem_t full;
    sem_t empty;
    pthread_mutex_t lock;
} so;

void* producer(void* sharing_obj);
void* consumer(void* sharing_obj);
void statistics (so* sp);

//메인 함수 흐름
int main(int argc, char* argv[]){
    FILE* rfile;
    int proN, conN;
    int i;
    int* ret;
    int rc;
    pthread_t pro[100];
    pthread_t con[100];
    struct timespec start, end;
    double elapsed_time;


    if(argc == 1){ // 명령줄 인수가 1개인 경우 -> exit
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
    }

    rfile = fopen((char*)argv[1],"rb");
    if(rfile == NULL){//읽어올 텍스트 파일이 NULL인 경우 -> exit
        printf("file error");
        exit(0);
    }

    if(argv[2] != NULL){//프로듀서 스레드 개수 결정
        proN = atoi(argv[2]);
        if(proN>100){
            proN = 100;
        }else if(proN == 0){
            proN = 1;
        }
    }else{
        proN = 1;
    }

    if(argv[3] != NULL){//컨슈머 스레드 개수 결정
        conN = atoi(argv[3]);
        if(conN>100){
            conN = 100;
        }else if(conN == 0){
            conN = 1;
        }
    }else{
        conN = 1;
    }

    so *sp = malloc(sizeof(so));//공유객체 포인터 생성 및 동적 할당
    memset(sp, 0, sizeof(so)); // 해당 공간 메모리 초기화
    sp->rfile = rfile;
    
    sem_init(&sp->full,0,0);
    sem_init(&sp->empty,0,100);
    pthread_mutex_init(&sp->lock,NULL);

    // 실행 시간 측정 시작
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("main continuing\n");
    for(i = 0;i<proN;i++){ //프로듀서 스레드 생성
        pthread_create(&pro[i],NULL,producer,sp);
    }

    for(i=0;i<conN;i++){ // 컨슈머 스레드 생성
        pthread_create(&con[i],NULL,consumer,sp);
    }

    for(i=0;i<conN;i++){ //컨슈머 스레드 종료 대기
        rc = pthread_join(con[i],(void**)&ret);
        printf("main: consumer_%d joined with %d\n", i, *ret);
    }

    for(i = 0;i<proN;i++){ //프로듀서 스레드 종료 대기
        rc = pthread_join(pro[i],(void**)&ret);
        printf("main: producer_%d joined with %d\n", i, *ret);
    }


    statistics(sp); // 통계 함수 호출

        // 실행 시간 측정 종료
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 경과 시간 계산
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Elapsed time: %.2f seconds\n", elapsed_time);

    pthread_exit(NULL); //메인 스레드 종료
    exit(0); // 모든 스레드 종료를 보장하면서 프로그램 종료
}

void* producer(void* sharing_obj){
    so* sp = sharing_obj;
    ssize_t contents = 0; //EOF를 감지 시 -1 저장
    char* line = NULL;
    size_t len = 0;
    int count = 0;
    int* ret = malloc(sizeof(int));
    
    while(1){
        sem_wait(&sp->empty);
        pthread_mutex_lock(&sp->lock);

        contents = getdelim(&line, &len, '\n',sp->rfile);
        if(contents == -1){
            sp->lines[sp->product] = NULL;
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->full);
            break;
        }

        sp->lines[sp->product] = strdup(line);
        sp->product = (sp->product + 1)%100;
        count ++;

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->full);
    }
    free(line);
    *ret = count;
    pthread_exit(ret);
}

void* consumer(void* sharing_obj){
    so* sp = sharing_obj;
    char* line = NULL;
    int count = 0;
    int* ret = malloc(sizeof(int));

    while(1){
        sem_wait(&sp->full);
        pthread_mutex_lock(&sp->lock);

        line = sp->lines[sp->consume];
        if(line == NULL){
            pthread_mutex_unlock(&sp->lock);
            sem_post(&sp->empty);
            sem_post(&sp->full);
            break;
        }

        sp->consume = (sp->consume + 1)%100;
        count++;

        // 문자열 처리를 위한 복사본 생성
        char* cptr = strdup(line);
        char* brka = NULL;
        char* substr = NULL;
        const char* sep = "{}()[],;\" \n\t^";

        // 토큰화하여 각 단어 처리
        for(substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
            size_t length = strlen(substr);
            if(length >= 30) length = 30;
            
            // 문자열 길이 카운팅 (1-based index로 변경)
            sp->dashboard1[length-1]++;
            
            // 각 문자 처리
            for(int i = 0; substr[i] != '\0'; i++) {
                char c = substr[i];
                if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    sp->dashboard2[c]++;
                }
            }
        }

        free(cptr);  // 복사본 메모리 해제
        free(line);  // 처리 완료된 라인 메모리 해제

        pthread_mutex_unlock(&sp->lock);
        sem_post(&sp->empty);
    }
    *ret = count;
    pthread_exit(ret);
}

void statistics (so* sp){
    // sum
    int sum = 0;
    for (int i = 0; i < 30; i++) sum += sp->dashboard1[i];
    
    // print out the distributions
    printf("\n");
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");
    for (int i = 0; i < 30; i++) {
        int num_star = sp->dashboard1[i] * 80 / (sum == 0 ? 1 : sum);
        printf("[%3d]: %10d \t", i + 1, sp->dashboard1[i]);
        for (int j = 0; j < num_star; j++) printf("*");
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