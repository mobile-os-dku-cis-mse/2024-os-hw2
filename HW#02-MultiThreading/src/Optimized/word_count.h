/*
* Author: Minhyuk Cho
* Date: 2024-10-12
* Description: Optimizer word_count.h
*/

#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>

#define BUFFER_SIZE  1024

using namespace std;

typedef struct {
    int index;
    int fd;
    off_t offset;
} so_t;

extern vector<string> buffer;
extern size_t fsize;
extern vector<pthread_mutex_t> full;
extern vector<pthread_mutex_t> empty_slots;
extern int terminate_pro;
extern int Nprod;
extern int Ncons;

void* producer(void* arg);
void* consumer(void* arg);
