/*
* Author: Minhyuk Cho
* Date: 2024-10-12
* Description: Semaphore Ver. main.cpp
*/

#include "word_count.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cerr << "usage: ./prod_cons <readfile> #Producer #Consumer\n";
        return 1;
    }

    auto start = chrono::high_resolution_clock::now();

    int Nprod = (argc > 2) ? min(atoi(argv[2]), 100) : 1;
    int Ncons = (argc > 3) ? min(atoi(argv[3]), 100) : 1;

    auto share = make_shared<SharedObject>();
    share->rfile = fopen((char *) argv[1], "rb");
    share->producer_idx = 0;
    share->consumer_idx = 0;

    sem_init(&share->full, 0, 0);
    pthread_mutex_init(&share->lock, NULL);
    sem_init(&share->empty, 0, BUFFER_SIZE);

    cout<<"Continue ...\n";
    vector<thread> producers;
    vector<thread> consumers; 
    vector<int> prod_results(Nprod);
    vector<int> cons_results(Ncons);
    int i = 0;
    int sum;
    for (int i = 0; i < Nprod; ++i) producers.emplace_back(producer, share, &prod_results[i]);

    for (int i = 0; i < Ncons; ++i) consumers.emplace_back(consumer, share, &cons_results[i]);
    
    for (auto& th : consumers) {
        if (th.joinable()) {
            th.join();
            // cout<<"main: consumer_"<<i<<" joined with "<<cons_results[i]<<endl;
        }
        // i++;
    }

    // i = 0;

    for (auto& th : producers) {
        if (th.joinable()) {
            th.join();
            // cout<<"main: producer_"<<i<<" joined with "<<prod_results[i]<<endl;
        }
        // i++;
    }

    print_statistics(*share);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = end - start;

    cout<<"Execution Time: "<<elapsed.count()<<" seconds\n";

    return 0;
}
