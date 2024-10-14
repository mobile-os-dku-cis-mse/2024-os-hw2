READFILE="/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/utils/FreeBSD9-orig.tar"
READFILE_2="/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/utils/LICENSE"

N=1
OUTPUT_FILE="/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/src/Optimized/optimized_execution_time.txt"

echo "Execution Time Results" >> $OUTPUT_FILE

    while((N <= 1)); do        
        PRODUCER_COUNTS=$N
        CONSUMER_COUNTS=$N
        echo "Running with $N producer(s) and $N consumer(s):"

        EXEC_TIME=$(/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/src/Optimized/word_count $READFILE $P $C | grep "Execution Time" | awk '{print $3}')

        printf "Execution Time: %f seconds\n" $EXEC_TIME >> $OUTPUT_FILE

        echo "Producers: $N, Consumers: $N" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
        ((N+=10)) 
    done