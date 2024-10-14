/*
* Author: Minhyuk Cho
* Date: 2024-10-12
* Description: Semaphore Ver. main.cpp
*/

#include "word_count.h"

vector<string> buffer(BUFFER_SIZE);
size_t fsize;
vector<pthread_mutex_t> full(BUFFER_SIZE), empty_slots(BUFFER_SIZE);
int terminate_pro = 0, Nprod, Ncons;

int main(int argc, char* argv[]) {
    if (argc < 2) return cerr << "Usage: ./prod_cons <readfile> #Producer #Consumer\n", EXIT_FAILURE;

    FILE* rfile = fopen(argv[1], "r");
    if (!rfile) return perror("rfile"), EXIT_FAILURE;

    auto start = chrono::high_resolution_clock::now();

    Nprod = (argc > 2) ? min(atoi(argv[2]), 100) : 1;
    Ncons = (argc > 3) ? min(atoi(argv[3]), 100) : 1;

    vector<so_t> so(Nprod);
    vector<pthread_t> prod(Nprod), cons(Ncons);

    fseek(rfile, 0, SEEK_END); // 끝 위치에 놓음
    fsize = ftell(rfile); // 끝 위치
    rewind(rfile); // 시작 위치 돌려놓음

    for (int i = 0; i < BUFFER_SIZE; ++i) {
        pthread_mutex_init(&full[i], nullptr);
        pthread_mutex_init(&empty_slots[i], nullptr);
        pthread_mutex_lock(&full[i]);
    }

    for (int i = 0; i < Nprod; ++i) { // 생산자들에게 각 처리
        so[i] = {i, open(argv[1], O_RDONLY), 
         (off_t)((static_cast<long>(fsize) - static_cast<long>(i * fsize) / Nprod) <= 0) 
            ? static_cast<off_t>(fsize) 
            : static_cast<off_t>(i * fsize / Nprod)};

        pthread_create(&prod[i], nullptr, producer, &so[i]);
    }

    for (auto& c : cons) pthread_create(&c, nullptr, consumer, nullptr);

    for (auto& p : prod) pthread_join(p, nullptr), ++terminate_pro;
    for (auto& c : cons) pthread_join(c, nullptr);

    for (int i = 0; i < BUFFER_SIZE; ++i) pthread_mutex_destroy(&full[i]), pthread_mutex_destroy(&empty_slots[i]);
    fclose(rfile);

    cout << "Execution Time: " 
         << chrono::duration<double>(chrono::high_resolution_clock::now() - start).count() 
         << " seconds\n";

    return EXIT_SUCCESS;
}
