/*
* Author: Minhyuk Cho
* Date: 2024-10-12
* Description: Semaphore Ver. minhyuk_prod_cons.cpp
*/

#include "word_count.h"

void producer(shared_ptr<SharedObject> so, int *ret) {
    int i = 0;
    char * line;
    ssize_t read = 0;
    size_t len = 0;

    while (true) {
        sem_wait(&so->empty);

        pthread_mutex_lock(&so->lock);
        
        read = getdelim(&line, &len, '\n', so->rfile);

        if (read == -1) {
            so->line[so->producer_idx] = NULL;
            pthread_mutex_unlock(&so->lock);
            sem_post(&so->full);
            break;
        }   
        
        so->linenum = i;
        so->line[so->producer_idx] = strdup(line);

        so->producer_idx = (so->producer_idx + 1) % BUFFER_SIZE;

        i++;
        pthread_mutex_unlock(&so->lock);
        sem_post(&so->full);
    }

    *ret = i;
    // cout<<"Prod_"<<this_thread::get_id()<<": "<<i<<"lines\n";
}

void consumer(shared_ptr<SharedObject> so, int *ret) {
    int i = 0;
    char * line;
    while (true) {
        sem_wait(&so->full);

        pthread_mutex_lock(&so->lock);

        line = so->line[so->consumer_idx];
        if (line == NULL) {
            pthread_mutex_unlock(&so->lock);
            sem_post(&so->empty);
            sem_post(&so->full);
            break;
        }

        process_line(*so, so->line[so->consumer_idx]);

        int tmp_idx = so->consumer_idx;

        so->consumer_idx  = (so->consumer_idx + 1) % BUFFER_SIZE;

        if(so->linenum % 5000000 == 0)
            ::cout<<"Cons_"<<this_thread::get_id()<<":["<<i<<": "<<so->linenum<<"] "<<so->line[tmp_idx]<<endl;

        i++;

        pthread_mutex_unlock(&so->lock);
        sem_post(&so->empty);
    }

    *ret = i;
    ::cout<<"Cons: "<<i<<" lines\n";
}