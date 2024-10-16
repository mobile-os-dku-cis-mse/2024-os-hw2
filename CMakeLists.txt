cmake_minimum_required(VERSION 3.22)
project(2024-os-hw1 C)

set(CMAKE_C_STANDARD 11)

add_executable(prod_cond src/single_buffer/prod_cons.c)

add_executable(char_stat src/char_stat.c)

add_executable(shared_queue
        src/multi_shared_queue/shared_queue.c
        src/multi_shared_queue/prod_cons.c
)

add_executable(multi_buffer_prod_cons src/multi_buffer/prod_cons.c)

add_executable(single_shared_queue
        src/single_shared_queue/prod_cons.c
        src/single_shared_queue/shared_queue.c
)
