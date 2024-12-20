# os_hw2
HW2: Multi-threaded word count

**Due date: Oct. 19th**

The second homework is about multi-thread programming with some synchronization.
Thread is a unit of execution; a thread has execution context, 
    which includes the registers, stack.
Note that address space (memory) is shared among threads in a process, 
    so there is no clear separation and protection for memory access among threads.

The example code includes some primitive code for multiple threads usage.
It basically tries to read a file and print it out on the screen.
It consists of three threads: main thread for admin job 
    second thread serves as a producer: reads lines from a file, and put the line string on the shared buffer
    third thread serves as a consumer:  get strings from the shared buffer, and print the line out on the screen

Unfortunately, the code is not working because threads runs independently from others.
the result is that different threads access invalid memory, and have wrong value, and crash or waiting for terminated thread infinitely.
To make it working, you have to touch the code so that the threads have correct value.

To have correct values in threads, you need to keep consistency for data touched by multiple threads.
To keeping consistency, you should carefully control the execution among threads, which is called as synchronization.

pthread_mutex_lock()/pthread_mutex_unlock are the functions for pthreads synchorinization.
For condition variable, you may need to look up functions such as pthread_cond_wait()/pthread_cond_signal().

To run a program, you may give filename to read and # of producers and # of consumers.
In case of single producer, 2 consumers, reading 'sample file'; you may need to execute your program by
./prod_cons ./sample 1 2 

You can download some example input source code from the link: [https://mobile-os.dankook.ac.kr/data/FreeBSD9-orig.tar] or  
you can use /opt/FreeBSD9-orig.tar from our server.

# Documentation:

## Prerequisites

- [GCC](https://gcc.gnu.org/)
- [Make](https://www.gnu.org/software/make/)
- [Git](https://git-scm.com/)

## Installation:

To install the project, clone the repository and navigate to the project directory:

```bash
git clone
cd os_hw2
git switch 32229349
```

## Compilation:

To compile the project, run the following command:

```bash
make
```

## Execution:

To run the project, run the following command:

```bash
./threading ./sample 1 2
```


# Implementation

```c
typedef struct {
    char* lines[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} buffer_t;

typedef struct {
    buffer_t* buffer;
    char* filename;
    int num_producers;
    int producer_index;
} producer_args_t;

typedef struct {
    buffer_t* buffer;
} consumer_args_t;
```

this is the structure of my code especialy for the consummer
