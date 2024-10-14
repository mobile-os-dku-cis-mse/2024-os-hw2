/*
* Author: Minhyuk Cho
* Date: 2024-10-12
* Description: Semaphore Ver. word_count.h
*/

#pragma once

#include "bits/stdc++.h"

#include <pthread.h>

#include <semaphore.h>

#include <stdlib.h>
#include <unistd.h>

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256
#define BUFFER_SIZE 4096

using namespace std;

struct SharedObject {
    FILE * rfile;
    int linenum = 0;
    char * line[BUFFER_SIZE];

    bool finished = false;
    int stat[MAX_STRING_LENGTH];
    int stat2[ASCII_SIZE];

    sem_t empty;
    sem_t full;
    
    pthread_mutex_t lock;

    int producer_idx;
    int consumer_idx;

    SharedObject(){
        memset(stat, 0, sizeof(stat));
        memset(stat2, 0, sizeof(stat2));
    }
};



void process_line(SharedObject& so, const string& line);
void print_statistics(SharedObject& so);
void producer(shared_ptr<SharedObject> so, int *ret);
void consumer(shared_ptr<SharedObject> so, int *ret);