#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "$BASH_SOURCE")")
PJ_DIR=$(dirname "$SCRIPT_DIR")
echo "PJ=$PJ_DIR"
PROGRAM="${PJ_DIR}/build/shared_queue"
INPUT_FILE="${PJ_DIR}/FreeBSD9-orig.tar"
echo "INPUT_FILE=$INPUT_FILE"
OUTPUT_FILE="multi_thread_test.csv"
echo "Producers,Consumers,Real,User,Sys" > $OUTPUT_FILE

for i in {1..100}; do
    echo "Running test with Producers=$i Consumers=$i"
    
    RESULT=$( { time ($PROGRAM $INPUT_FILE $i $i > /dev/null); } 2>&1 )
    
    REAL_TIME=$(echo "$RESULT" | grep real | awk '{print $2}')
    USER_TIME=$(echo "$RESULT" | grep user | awk '{print $2}')
    SYS_TIME=$(echo "$RESULT" | grep sys | awk '{print $2}')
    
    echo "$i,$i,$REAL_TIME,$USER_TIME,$SYS_TIME" >> $OUTPUT_FILE
done

echo "Test complete. Results saved in $OUTPUT_FILE."

