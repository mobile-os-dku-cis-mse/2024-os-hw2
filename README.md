# Producer-Consumer Multithreaded Program

This project implements a Producer-Consumer problem using multithreading in C. It showcases the synchronization of multiple producer and consumer threads that share a circular buffer. The implementation uses mutexes and condition variables to prevent race conditions and ensure safe access to shared data.

Features

	• Supports configurable number of producers and consumers.

	• Utilizes a circular buffer to store and transfer data between threads.

	• Implements thread synchronization with mutexes and condition variables.

	• Graceful handling of end-of-file and thread termination.

## Compilation

To compile the prod_cons program, simply use the following command:

```bash
make
```
This will build the project and create the prod_cons executable.

### Other Useful Makefile Rules:

* clean: Removes object files (.o files).
* fclean: Removes object files and the executable.
* re: Cleans everything and recompiles the project from scratch.
* debug: Compiles the program with debugging symbols.
* redebug: Cleans everything and recompiles with debugging symbols.
* tests_run: Compiles and runs the test suite.
