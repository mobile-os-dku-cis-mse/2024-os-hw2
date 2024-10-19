#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

#define MAX_STRING_LENGTH 30 // 문자열의 크기
#define ASCII_SIZE 256
#define MAX_QUEUE_SIZE 100000 // 큐의 크기

pthread_mutex_t stat_lock; // 통계업데이트 할때 필요한 뮤텍스
int stat[MAX_STRING_LENGTH];
int stat2[ASCII_SIZE];

pthread_cond_t prod_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t cons_cv = PTHREAD_COND_INITIALIZER;

typedef struct sharedobject {
   FILE *rfile; 
   int linenum; 
   char *line[MAX_QUEUE_SIZE]; // 문자열
   pthread_mutex_t lock; // mutex lock
   int front; //큐의 앞(데이터가 큐에서 나가는곳)
   int rear; // 큐의 뒤(데이터가 큐에서 들어오는곳)
   int count; // 큐 요소의 개수
   int finish_flag;
} so_t;

// 큐가 빈것을 확인
int is_empty(so_t *q){
   return q->count == 0;
}

// 큐가 꽉참을 확인
int is_full(so_t *q){
   return q->count == MAX_QUEUE_SIZE;
}

// 큐에 데이터를 넣음
void enqueue(so_t *q, char *line){   
    if(is_full(q)){
      return;
   }
   else{
      q->line[q->rear] = strdup(line);      /* share the line */
      q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
      q->count++;
   }
}

// 큐에서 데이터를 뺌
char* dequeue(so_t *q){

    char *data;
   if (is_empty(q)){
      return NULL;
   }
   else{
      data = q->line[q->front];
      q->front = (q->front + 1) % MAX_QUEUE_SIZE;
      q->count--;      
   }
    return data;
}

void *producer(void *arg) {
   so_t *so = arg;
   int *ret = malloc(sizeof(int));
   FILE *rfile = so->rfile;
   int i = 0; // 생산자가 처리한 줄 개수
   char *line = NULL;
   size_t len = 0;
   ssize_t read = 0;

   while (1) {
    read = getdelim(&line, &len, '\n', rfile);
    if (read == -1) {  // 파일의 끝에 도달하면
        pthread_mutex_lock(&so->lock);
        so->finish_flag = 1;  // 플래그 설정
        pthread_cond_broadcast(&cons_cv);  // 모든 소비자에게 알림
        pthread_mutex_unlock(&so->lock); 
        break;
    }      
    pthread_mutex_lock(&so->lock);
    if (is_full(so)) { // 큐가 꽉차면 
         pthread_cond_wait(&prod_cv, &so->lock); // 생산을 대기
    }  
      enqueue(so, line);
      so->linenum = i++;
      pthread_cond_broadcast(&cons_cv);
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

      // 큐가 비어 있고, 생산자가 종료되지않았으면 대기
      while (is_empty(so) && so->finish_flag != 1) {
         pthread_cond_wait(&cons_cv, &so->lock);
      }

      // 생산자가 종료되었고, 큐가 비어 있으면 종료
      if (is_empty(so) && so->finish_flag == 1) {
         pthread_mutex_unlock(&so->lock);
         break;
      }

      // 큐에서 데이터를 가져와 처리
      line = dequeue(so);
      pthread_cond_broadcast(&prod_cv);
      pthread_mutex_unlock(&so->lock);
      len = strlen(line);        
      if (line[len-1] == '\n') len--;

      printf("Cons_%x: [%02d:%02d] %s",
         (unsigned int)pthread_self(), i, so->linenum, line);

      pthread_mutex_lock(&stat_lock);

      if (len >= MAX_STRING_LENGTH) len = MAX_STRING_LENGTH;
      stat[len-1]++;
      for (int j = 0; j < len; j++) {
         if (isalpha(line[j])) {
            stat2[(unsigned char)line[j]]++;
         }
      }

      pthread_mutex_unlock(&stat_lock);
      
      free(line);
      i++;
   }
   printf("Cons: %d lines\n", i);
   *ret = i;
   pthread_exit(ret);
}

int main (int argc, char *argv[])
{
   pthread_t prod[100]; // 생산자 버퍼
   pthread_t cons[100]; // 소비자 버퍼
   int Nprod, Ncons;
   int rc;   long t;
   int *ret;
   int i;
   FILE *rfile;
   int sum = 0;

   struct timeval start, end;
   double elapsed_time;

   gettimeofday(&start,NULL);

   if (argc == 1) {
      printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
      exit (0);
   }
   so_t *share = malloc(sizeof(so_t));
   memset(share, 0, sizeof(so_t)); // 공유 버퍼를 모든 값을 0으로 초기화 
   rfile = fopen((char *) argv[1], "r");
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

   share->rfile = rfile;
//    share->line = NULL;
   pthread_mutex_init(&share->lock, NULL);
   for (i = 0 ; i < Nprod ; i++)
      pthread_create(&prod[i], NULL, producer, share);
   for (i = 0 ; i < Ncons ; i++)
      pthread_create(&cons[i], NULL, consumer, share);
   printf("main continuing\n");

   for (i = 0 ; i < Ncons ; i++) {
      rc = pthread_join(cons[i], (void **) &ret);
      printf("main: consumer_%d joined with %d\n", i, *ret);
   }
   for (i = 0 ; i < Nprod ; i++) {
      rc = pthread_join(prod[i], (void **) &ret);
      printf("main: producer_%d joined with %d\n", i, *ret);
   }

   gettimeofday(&end,NULL);
   elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
   printf("elapsed time: %f seconds\n",elapsed_time);

   // sum
   for (i = 0 ; i < MAX_STRING_LENGTH ; i++) {
      sum += stat[i];
   }
   // print out distributions
   printf("*** print out distributions *** \n");
   printf("  #ch  freq \n");
   for (i = 0 ; i < MAX_STRING_LENGTH ; i++) {
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
         
   pthread_cond_destroy(&prod_cv);
   pthread_cond_destroy(&cons_cv);
   pthread_mutex_destroy(&share->lock); // 모든 스레드가 종료된 후 뮤텍스 해제
   pthread_mutex_destroy(&stat_lock);

   pthread_exit(NULL);
   exit(0);
}

