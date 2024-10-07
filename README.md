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
It consists of three threads: the main thread is for admin job 
    second thread serves as a producer: reads lines from a file and puts the line string on the shared buffer
    third thread serves as a consumer:  gets strings from the shared buffer and prints the line out on the screen

Unfortunately, the code is not working because threads runs independently from others.
The result is that different threads access invalid memory, have wrong values, and crash or wait for the terminated thread infinitely.
To make it work, you have to touch the code so that the threads have the correct value.

To have correct values in threads, you need to keep consistency for data touched by multiple threads.
To keep consistency, you should carefully control the execution among threads, which is called synchronization.

pthread_mutex_lock()/pthread_mutex_unlock are the functions for threads synchronization.
For condition variable, you may need to look up functions such as pthread_cond_wait()/pthread_cond_signal().

The goals from HW2 are 

1. correct the code for prod_cons.c so that it works with 1 producer and 1 consumer

2. enhance it to support multiple consumers.

3. Make consumer(s) to gather some statistics of the given text file. 
Basically, count the number of each alphabet character in the line.
char_stat.c can be a hint for gathering statistics.
At the end of execution, you should print out the statistics of the entire text.
Beat the fastest execution, maximizing the concurrency!

To run a program, you may give a filename to read and # of producers and # of consumers.
In the case of a single producer, two consumers, reading 'sample file'; you may need to execute your program by
./prod_cons ./sample 1 2 

You can download some example input source code from the link: [http://mobile-os.dankook.ac.kr/data/FreeBSD9-orig.tar] or  
you can use /opt/FreeBSD9-orig.tar from our server.

Please make a document so that I can follow it to build/compile and run the code.
It would be better if the document included an introduction and some important implementation details of your program structure.

To measure the execution time, consider using clock_gettime() or gettimeofday().

htop is a program that shows the execution of threads in the system.

Measure & compare the execution time for different # of threads

Happy hacking!
Seehwan
