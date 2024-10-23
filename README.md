# Producer-Consumer Multi-threaded Program

This program is a multi-threaded C implementation of the producer-consumer problem using `pthread`. Multiple producers and consumers are synchronized using `pthread_mutex_t` for mutual exclusion and `pthread_cond_t` for condition signaling to avoid busy waiting. The program reads lines from an input file and writes processed lines to an output file, with both producers and consumers handling the data through shared memory.

#### Features
- Supports multiple producers and consumers.
- Uses mutexes to prevent race conditions.
- Uses condition variables to avoid busy waiting.
- Producers read lines from the input file and notify consumers.
- Consumers process the data and write to an output file.
- Proper handling of end-of-file (EOF) scenarios.

#### Files
- **Input file:** Producers read data line by line from this file.
- **Output file:** Consumers write processed data to this file.

#### Program Flow
1. **Producers** read lines from the input file, store them in shared memory, and signal consumers when data is available.
2. **Consumers** wait for producers to provide data, process each line, and write the output to the specified file.

## Compilation

To compile the program, use `gcc` with the `-pthread` option:

```bash
gcc -o prod_cons prod_cons.c -pthread
```

## Usage

The program expects at least three arguments:
- The input file to read from.
- The output file to write to.
- The number of producer threads.
- The number of consumer threads.

Example usage:

```bash
./prod_cons input.txt output.txt 2 3
```

This command starts 2 producer threads and 3 consumer threads, reading from `input.txt` and writing the processed lines to `output.txt`.

## Example

Consider an input file `input.txt` with the following content:

```
Line 1
Line 2
Line 3
```

Running the program with `./prod_cons input.txt output.txt 2 3` will create two producer threads and three consumer threads. The output file will look like:

```
Cons_xxxxx: [00:00] Line 1
Cons_xxxxx: [01:01] Line 2
Cons_xxxxx: [02:02] Line 3
```

Where `xxxxx` represents the thread ID.
