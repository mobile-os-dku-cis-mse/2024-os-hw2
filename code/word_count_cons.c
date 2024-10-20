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
    pthread_cond_t cond_producer; // ������ ���� ����
    pthread_cond_t cond_consumer; // �Һ��� ���� ����
    int full; // ���ۿ� ä���� �׸� ��
    int done; // ������ ���� �÷���
    int stat[ASCII_SIZE]; // ���ĺ� ���
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

        // ���۰� ���� ���� ���
        while (so->full >= BUFFSIZE) {
            pthread_cond_wait(&so->cond_producer, &so->lock);
        }

        read = getdelim(&line, &len, '\n', so->rfile);
        if (read == -1) {
            so->done = 1; // �����ڰ� �Ϸ�Ǿ����� ǥ��
            pthread_cond_broadcast(&so->cond_consumer); // �Һ��ڿ��� ��ȣ�� ����
            printf("Producer done.\n");
            pthread_mutex_unlock(&so->lock);
            free(line); // �Ϸ� �� line �޸� ����
            break;
        }

        // ���� �ٿ� ���� �޸� �Ҵ�
        char *new_line = strdup(line);
        if (new_line == NULL) {
            perror("Failed to allocate memory");
            free(line); // strdup ���� �� line ����
            pthread_mutex_unlock(&so->lock);
            break;
        }

        so->line[so->nextin] = new_line; // ���� ���ۿ� ����
        so->nextin = (so->nextin + 1) % BUFFSIZE;        
        so->full++;
        count++;

        printf("Prod_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->full, new_line);
        
        pthread_cond_broadcast(&so->cond_consumer); // �Һ��ڿ��� ��ȣ�� ����
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

        // �����Ͱ� ������ ���
        while (so->full <= 0 && !so->done) {
            pthread_cond_wait(&so->cond_consumer, &so->lock);
        }

        if (so->full <= 0 && so->done) { // �����ڰ� �Ϸ�Ǿ��� ���۰� ��� ������
            pthread_mutex_unlock(&so->lock);
            break;
        }

        line = so->line[so->nextout];
        if (line == NULL) {
            pthread_mutex_unlock(&so->lock);
            break;
        }

        // �Һ��� ���� ���� �� ī��Ʈ /* multi_cons���� �߰��� �ڵ� */
        for (char *ptr = line; *ptr != '\0'; ptr++) {
            if (*ptr < ASCII_SIZE) {
                so->stat[(unsigned char)*ptr]++;
            }
        }

        // �Һ��� �� ��� �� �޸� ����
        printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), count, so->nextout, line);
        free(line);
        so->line[so->nextout] = NULL; // �����͸� �����
        so->nextout = (so->nextout + 1) % BUFFSIZE;
        so->full--;
        count++;

        pthread_cond_broadcast(&so->cond_producer); // �����ڿ��� ��ȣ�� ����
        pthread_mutex_unlock(&so->lock);
    }

    *ret = count;
    pthread_exit(ret);
}

/* multi_cons���� �߰��� �ڵ� */
void print_statistics(so_t *so) {
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");

    int sum = 0;
    for (int i = 0; i < ASCII_SIZE; i++) {
        sum += so->stat[i];
    }

    for (int i = 0; i < 30; i++) {
        int num_star = so->stat[i] * 80 / sum; // ���� ��Ʈ ������ ���� ����
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

    // ��� �迭 �ʱ�ȭ /* multi_cons���� �߰��� �ڵ� */
    memset(share->stat, 0, sizeof(share->stat));

    // ����ȭ ������Ƽ�� �ʱ�ȭ
    pthread_mutex_init(&share->lock, NULL);
    pthread_cond_init(&share->cond_producer, NULL);
    pthread_cond_init(&share->cond_consumer, NULL);
    share->full = 0; // ������ �ʱ� ���� ����
    share->done = 0; // �ʱ� ���� ���� ����
    share->nextin = 0; // ���� �Է� ��ġ �ʱ�ȭ
    share->nextout = 0; // ���� ��� ��ġ �ʱ�ȭ

    // ������ ������ ����
    for (i = 0; i < Nprod; i++) {
        pthread_create(&prod[i], NULL, producer, share);
    }

    // �Һ��� ������ ����
    for (i = 0; i < Ncons; i++) {
        pthread_create(&cons[i], NULL, consumer, share);
    }

    // ������ ���� �� ��� ���
    printf("\n\n");
    for (i = 0; i < Nprod; i++) {
        pthread_join(prod[i], (void **)&ret);
        printf("Producer %d created %d lines.\n", i, *ret);
        free(ret); // Producer result �޸� ����
    }

    for (i = 0; i < Ncons; i++) {
        pthread_join(cons[i], (void **)&ret);
        printf("Consumer %d consumed %d lines.\n", i, *ret);
        free(ret); // Consumer result �޸� ����
    }

    fclose(rfile);

    // ��� ���
    print_statistics(share);

    // �ڿ� ����
    pthread_mutex_destroy(&share->lock);
    pthread_cond_destroy(&share->cond_producer);
    pthread_cond_destroy(&share->cond_consumer);
    free(share);

    return 0;
}
