#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256
#define BUFFER_SIZE 256

int stat[MAX_STRING_LENGTH];
int stat2[ASCII_SIZE];
void *producer(void *arg);
void *consumer(void *arg);
void calculate_stat(char *arg);
void print_stat();

typedef struct sharedobject{
	FILE *rfile;
	int lineNum;
	pthread_mutex_t lock;
	pthread_cond_t cv;

	char *data_buffer[BUFFER_SIZE];
	int full[BUFFER_SIZE];
	int prodCount;
	int consCount;
	int done;
} so_t;

void *producer(void *arg){
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	int in = 0;

	while(1){
		pthread_mutex_lock(&so->lock);
		in = so->prodCount;
		(so->prodCount)++;
		so->prodCount %= BUFFER_SIZE;

		while(so->full[in] == 1)
			pthread_cond_wait(&so->cv, &so->lock);

		read = getdelim(&line, &len, '\n', rfile);

		if(read == -1){
			so->full[in] = 1;
			so->data_buffer[in] = NULL;
		} else{
			so->full[in] = 1;
			so->data_buffer[in] = strdup(line);
			so->lineNum = i++;
		}

		pthread_cond_signal(&so->cv);
		pthread_mutex_unlock(&so->lock);
		if(read == -1) break;
	}

	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg){
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	char *line = NULL;
	int out = 0;

	while(1){
		pthread_mutex_lock(&so->lock);
		out = so->consCount;
		(so->consCount)++;
		so->consCount %= BUFFER_SIZE;

		while(so->full[out] == 0){
			if(so->done == 1) break;
			pthread_cond_wait(&so->cv, &so->lock);
		}

		if(so->done == 1){
			pthread_cond_signal(&so->cv);
			pthread_mutex_unlock(&so->lock);
			break;
		}

		line = so->data_buffer[out];

		if(line != NULL){
			//printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), i, so->lineNum, line);
    		calculate_stat(line);
			free(so->data_buffer[out]);
			i++;
			so->full[out] = 0;
		} else so->done = 1;

		pthread_cond_signal(&so->cv);
		pthread_mutex_unlock(&so->lock);
	}
	//printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}

void calculate_stat(char *arg){
    char *cptr = NULL;
    char *word = NULL;
    char *brka = NULL;
    char *sep = "{}()[],;\" \n\t^";
    size_t length, length2 = 0;
    cptr = arg;

    for(word = strtok_r(cptr, sep, &brka); word;
    	word = strtok_r(NULL, sep, &brka)){
        if(*cptr == '\0') break;
        length = strlen(word);
        length2 = length;
        if(length >= 30) length = 30;

        stat[length - 1]++;
        cptr = word;
        for(int i = 0; i < length2 + 1; i++){
            if(*cptr < 256 && *cptr > 1){
                stat2[*cptr]++;
            }
            cptr++;
        }
    }
}

void print_stat(){
    int sum = 0;
	for(int i = 0; i < 30; i++) sum += stat[i];

    printf("#ch   freq\n");
	for(int i = 0; i < 30; i++){
    	int num_star = stat[i]*80/sum;
     	printf("[%2d]: %4d \t", i+1, stat[i]);

    	for(int j = 0; j < num_star; j++)
    		printf("*");
    	printf("\n");
   }
    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['A'] + stat2['a'], stat2['B'] + stat2['b'], stat2['C'] + stat2['c'], stat2['D'] + stat2['d'], stat2['E'] + stat2['e'],
           stat2['F'] + stat2['f'], stat2['G'] + stat2['g'], stat2['H'] + stat2['h'], stat2['I'] + stat2['i'], stat2['J'] + stat2['j'],
           stat2['K'] + stat2['k'], stat2['L'] + stat2['l'], stat2['M'] + stat2['m'], stat2['N'] + stat2['n'], stat2['O'] + stat2['o'],
           stat2['P'] + stat2['p'], stat2['Q'] + stat2['q'], stat2['R'] + stat2['r'], stat2['S'] + stat2['s'], stat2['T'] + stat2['t'],
           stat2['U'] + stat2['u'], stat2['V'] + stat2['v'], stat2['W'] + stat2['w'], stat2['X'] + stat2['x'], stat2['Y'] + stat2['y'],
           stat2['Z'] + stat2['z']);
}

int main(int argc, char* argv[]){
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int* ret;
	int i;
	FILE* rfile;

	if(argc == 1){
		printf("usage: ./<executable file> <readfile> <number of producers> <number of consumers>\n");
		exit(0);
	}
	so_t* share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));

	rfile = fopen((char*)argv[1], "r");
	if(rfile == NULL){
		perror("rfile");
		exit(0);
	}
	if(argv[2] != NULL){
		Nprod = atoi(argv[2]);
		if(Nprod > 100) Nprod = 100;
		if(Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if(argv[3] != NULL){
		Ncons = atoi(argv[3]);
		if(Ncons > 100) Ncons = 100;
		if(Ncons == 0) Ncons = 1;
	} else Ncons = 1;

	share->rfile = rfile;
	memset(stat, 0, sizeof(stat));
	memset(stat2, 0, sizeof(stat2));

	pthread_cond_init(&share->cv, NULL);
	pthread_mutex_init(&share->lock, NULL);

	for(i = 0; i < Nprod; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for(i = 0; i < Ncons; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for(i = 0; i < Ncons; i++){
		rc = pthread_join(cons[i], (void**)&ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for(i = 0; i < Nprod; i++){
		rc = pthread_join(prod[i], (void**)&ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}

	print_stat();
	pthread_exit(NULL);
	exit(0);
}

