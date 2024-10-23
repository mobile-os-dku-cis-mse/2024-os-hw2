### Producer-Consumer Multi-threaded Program

This program is a multi-threaded C implementation of the producer-consumer problem using `pthread`. Multiple producers and consumers are synchronized using `pthread_mutex_t` for mutual exclusion and `pthread_cond_t` for condition signaling to avoid busy waiting. The program reads lines from an input file and writes processed lines to an output file, with both producers and consumers handling the data through shared memory.

#### Features
- Supports multiple producers and consumers.
- Uses mutexes to prevent race conditions.
- Uses condition variables to avoid busy waiting.
- Producers read lines from the input file and notify consumers.
- Consumers process the data and write to an output file.
- Proper handling of end-of-file (EOF) scenarios.

### Files
- **Input file:** Producers read data line by line from this file.
- **Output file:** Consumers write processed data to this file.

### Program Flow
1. **Producers** read lines from the input file, store them in shared memory, and signal consumers when data is available.
2. **Consumers** wait for producers to provide data, process each line, and write the output to the specified file.

### Compilation

To compile the program, use `gcc` with the `-pthread` option:

```bash
gcc -o prod_cons prod_cons.c -pthread
```

### Usage

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

### Synchronization

- **Mutexes:** Used to ensure mutual exclusion when producers and consumers access the shared memory (`so_t` structure).
- **Condition Variables:** Employed to signal consumers when new data is available and to notify producers when consumers have finished processing data.

### Important Functions

- `producer(void *arg)`: Reads lines from the input file and places them into the shared memory. Signals consumers once a line is available.
- `consumer(void *arg)`: Waits for a signal from producers, processes the line from shared memory, and writes the result to the output file.

### Example

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

### Error Handling

- If the input or output files cannot be opened, the program will terminate with an error message.
- The program handles EOF scenarios by ensuring producers and consumers exit cleanly.

### Cleanup

- The input and output files are properly closed after processing.
- Memory allocated for thread returns and shared data is freed.
- Proper resource management ensures that no memory leaks or file descriptor leaks occur.

### Limitations and Future Improvements
- The program currently limits the number of producers and consumers to a maximum of 100 each.
- The program can be extended to support dynamic buffer sizes or more advanced scheduling techniques for producers and consumers.


>>>>>>> 1a80ef8 (chore: Add Readme)
