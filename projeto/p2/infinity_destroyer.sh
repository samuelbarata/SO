#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;92m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'
NORM='\033[0m'

#if [ $# -lt 1 ] ; then
#    echo "Usage: $0 inputdir outputdir maxthreads numbuckets"
#    echo "Example: ${0} inputs outputs 4 5"
#    exit 1
#fi

input="inputs"
output="output"
maxthreads=31
numbuckets=5

make clean > /dev/null
make > /dev/null
mkdir output 2> /dev/null
mkdir output/temp 2> /dev/null


for ((k=1 ; k>0 ; k++))
do
for test_in in `ls ${input}`; do
    for ((i=1 ; i<=$maxthreads ; i++))
    do
        if [ $i == 1 ] ; then
            prog_name="tecnicofs-nosync"
            buckets=1
        else
            prog_name="tecnicofs-mutex"
            buckets=$numbuckets
        fi

        test_out="${output}/${test_in}-${i}.txt"
        test_stdout="${output}/temp/${test_in}-${i}.stdout"

        #echo "InputFile=${test_in} NumThreads=${i}"
        ./${prog_name} ${input}/${test_in} ${test_out} ${i} ${buckets} > ${test_stdout} 2>/dev/stdout
        rv_student=$?

        if [ ${rv_student} == 139 ]; then
            echo -e "${RED}ERROR${NC}: ${YELLOW}SEGFAULT${NC}: ${test_in}"
            continue
        fi
        if [ ${rv_student} != 0 ]; then
            echo -e "${test_in}:${RED}ERROR${NC}: Program return ${YELLOW}${rv_student}${NC}!"
            continue
        fi
		echo ${k}.${test_in}.${i}
        #cat ${test_stdout} | grep '^TecnicoFS'
		rm -f -r ${test_stdout}
    done
done
done

