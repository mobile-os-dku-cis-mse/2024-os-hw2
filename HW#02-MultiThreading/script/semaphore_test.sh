READFILE="/home/minhyuk/OperatingSystem/HW#02-Multi-Threading/HW#02-MultiThreading/utils/FreeBSD9-orig.tar"
READFILE_2="/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/utils/LICENSE"
PRODUCER_COUNTS=(1)
CONSUMER_COUNTS=(1)
OUTPUT_FILE="/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/src/Semaphore/semaphore_execution_time.txt"

echo "Execution Time Results" >> $OUTPUT_FILE

for P in "${PRODUCER_COUNTS[@]}"; do
    for C in "${CONSUMER_COUNTS[@]}"; do
        echo "Running with $P producer(s) and $C consumer(s):"

        EXEC_TIME=$(/home/minhyuk/OperatingSystem/HW#02-Multi_Threaded_Word_Count/HW#02-MultiThreading/src/Semaphore/word_count $READFILE $P $C | grep "Execution Time" | awk '{print $3}')

        printf "Execution Time: %f seconds\n" $EXEC_TIME >> $OUTPUT_FILE

        echo "Producers: $P, Consumers: $C" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
    done
done