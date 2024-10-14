#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <sys/stat.h>

#include <unistd.h>

struct SharedData {
    std::vector<std::vector<char>> bufferQueue;
    std::vector<char> buffer;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
    bool done;
    int prodRound;
    int consRound;
};

struct ProdArgs {
    std::ifstream *file;
    int nProd;
    int nbytes;
    int id;
    int offset;
    SharedData *sharedData;
};

struct ConsArgs {
    int start;
    int nbytes;
    int id;
    SharedData *sharedData;
};

void *producer(void *arg) {
    ProdArgs *args = (ProdArgs *)arg;
    std::ifstream &file = *(args->file);
    int nProd = args->nProd;
    int nbytes = args->nbytes;
    int id = args->id;
    int offset = args->offset;
    SharedData *sharedData = args->sharedData;

    while (sharedData->prodRound != id) {
        pthread_mutex_lock(&sharedData->mtx);
        while (sharedData->prodRound != id) {
            pthread_cond_wait(&sharedData->cv, &sharedData->mtx);
        }
        pthread_mutex_unlock(&sharedData->mtx);
    }
    std::vector<char> buffer(nbytes);
    file.seekg(offset, std::ios::beg);
    file.read(buffer.data(), nbytes);
    std::streamsize bytesRead = file.gcount();
    std::vector<char> tmp = std::vector<char>(buffer.begin(), buffer.begin() + bytesRead);

    pthread_mutex_lock(&sharedData->mtx);
    sharedData->bufferQueue[id] = tmp;
    sharedData->prodRound++;
    pthread_mutex_unlock(&sharedData->mtx);
    pthread_cond_signal(&sharedData->cv);

    return nullptr;
}

void *consumer(void *arg) {
    ConsArgs *args = (ConsArgs *)arg;
    SharedData *sharedData = args->sharedData;

    while (true) {
        pthread_mutex_lock(&sharedData->mtx);
        while (sharedData->buffer.empty() && !sharedData->done) {
            pthread_cond_wait(&sharedData->cv, &sharedData->mtx);
        }
        while (sharedData->consRound != args->id) {
            pthread_cond_wait(&sharedData->cv, &sharedData->mtx);
        }
        if (!sharedData->buffer.empty()) {
            std::string str(sharedData->buffer.begin() + args->start, sharedData->buffer.begin() + args->start + args->nbytes);
            std::cout << str;
            sharedData->consRound++;
            pthread_cond_broadcast(&sharedData->cv);
            pthread_mutex_unlock(&sharedData->mtx);
            break;
        } else {
            pthread_mutex_unlock(&sharedData->mtx);
        }
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    pthread_t prod[100];
    pthread_t cons[100];
    int nCons = 0;
    int nProd = 0;
    struct stat filestat;

    if (argc != 4) {
        std::cerr << "Usage: ./prod_cons <file> <nProd> <nCons>\n" << std::endl;
        return 1;
    }
    if (stat(argv[1], &filestat) != 0) {
        std::cerr << "Error: could not stat file " << argv[1] << std::endl;
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file " << argv[1] << std::endl;
        return 1;
    }
    try {
        nProd = std::stoi(argv[2]);
        nCons = std::stoi(argv[3]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: invalid argument" << std::endl;
        return 1;
    }
    if (nProd < 1) {
        nProd = 1;
    } else if (nProd > 100) {
        nProd = 100;
    }
    if (nCons < 1) {
        nCons = 1;
    } else if (nCons > 100) {
        nCons = 100;
    }
    int nbytes = filestat.st_size;

    SharedData sharedData;
    sharedData.bufferQueue.resize(nProd);
    sharedData.consRound = -1;
    sharedData.prodRound = 0;
    pthread_mutex_init(&sharedData.mtx, nullptr);
    pthread_cond_init(&sharedData.cv, nullptr);
    sharedData.done = false;
    for (int i = 0; i < nProd; i++) {
        if (i == nProd - 1) {
            ProdArgs *args = new ProdArgs{&file, nProd, nbytes / nProd + nbytes % nProd, i, (nbytes / nProd) * i, &sharedData};
            pthread_create(&prod[i], nullptr, producer, args);
        } else {
            ProdArgs *args = new ProdArgs{&file, nProd, nbytes / nProd, i, (nbytes / nProd) * i, &sharedData};
            pthread_create(&prod[i], nullptr, producer, args);
        }
    }
    for (int i = 0; i < nCons; i++) {
        if (i == nCons - 1) {
            ConsArgs *args = new ConsArgs{i * (nbytes / nCons), nbytes / nCons + nbytes % nCons, i, &sharedData};
            pthread_create(&cons[i], nullptr, consumer, args);
        } else {
            ConsArgs *args = new ConsArgs{i * (nbytes / nCons), nbytes / nCons, i, &sharedData};
            pthread_create(&cons[i], nullptr, consumer, args);
        }
    }
    for (int i = 0; i < nProd; i++) {
        pthread_join(prod[i], nullptr);
    }
    pthread_mutex_lock(&sharedData.mtx);
    for (int i = 0; i < nProd; i++) {
        sharedData.buffer.insert(sharedData.buffer.end(), sharedData.bufferQueue[i].begin(), sharedData.bufferQueue[i].end());
    }
    sharedData.done = true;
    sharedData.consRound = 0;
    pthread_mutex_unlock(&sharedData.mtx);
    pthread_cond_broadcast(&sharedData.cv);
    for (int i = 0; i < nCons; i++) {
        pthread_join(cons[i], nullptr);
    }
    std::cout << std::flush;
    pthread_mutex_destroy(&sharedData.mtx);
    pthread_cond_destroy(&sharedData.cv);
    file.close();
    return 0;
}