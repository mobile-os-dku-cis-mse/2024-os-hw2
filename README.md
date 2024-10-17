# os_hw1 prod_cons
## Introduction

This is a multi-threaded program that reads the content of a file and then prints it to the console.\
The program uses a number of threads determined by the user to read the file and a number of threads determined by the user to print the content to the console.\
The program uses a buffer to store the content read from the file and to be printed to the console.

## Features

- The program reads the content of a file using multiple threads and stores it in a buffer.
- The program prints the content of the buffer to the console using multiple threads.

## Build

To build the program, run the following command:

```bash
make
```

This will create an executable file called `prod_cons`.

## Run

To run the program, execute the following command:

```bash
./prod_cons ./sample.txt 5 5 
```

The program takes three arguments:
- The first argument is the path to the file to be read.
- The second argument is the number of threads to read the file.
- The third argument is the number of threads to print the content to the console.

## Clean

To clean up the build files, run the following command:

```bash
make clean
```

This will remove the object files.

In order to also remove the executable file, run the following command:

```bash
make fclean
```

This will remove the object files and the executable file.

To rebuild the executable file from scratch, run the following command:

```bash
make re
```

This will remove the object files and the executable file, and then rebuild the executable file.
