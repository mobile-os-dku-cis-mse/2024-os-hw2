#include "word_count.h"

void* producer(void* arg) {
    auto* so = static_cast<so_t*>(arg);  
    int processed = 0, chunk;

    lseek(so->fd, so->offset, SEEK_SET);
    vector<char> buffer(BUFFER_SIZE); 

    while (processed < fsize / Nprod) {
        pthread_mutex_lock(&empty_slots[so->index]);

        chunk = min(BUFFER_SIZE, static_cast<int>(fsize / Nprod - processed));
        if (read(so->fd, buffer.data(), chunk) <= 0) break;

        ::buffer[so->index] = string(buffer.begin(), buffer.begin() + chunk);
        processed += chunk;

        pthread_mutex_unlock(&full[so->index]);
    }
    close(so->fd);
    pthread_exit(nullptr);
}

void* consumer(void* arg) {
    for (int idx = 0;; idx = (idx + 1) % Nprod) {
        if (pthread_mutex_trylock(&full[idx]) == 0 && !buffer[idx].empty()) {
            buffer[idx].clear(); 
            pthread_mutex_unlock(&empty_slots[idx]);
        } else if (terminate_pro >= Nprod) break; 
    }
    pthread_exit(nullptr);
}
