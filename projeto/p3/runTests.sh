#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;92m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'
NORM='\033[0m'

if [ $# -lt 4 ] ; then
    echo "Usage: $0 inputdir outputdir maxthreads numbuckets"
    echo "Example: ${0} inputs outputs 4 5"
    exit 1
fi

input=$1
output=$2
maxthreads=$3
numbuckets=$4

make clean > /dev/null
make > /dev/null                    #compile
mkdir $output 2> /dev/null          #makes output directory if it doesn't exist yet
mkdir $output/temp 2> /dev/null     #makes temp directory 

for test_in in `ls ${input}`; do        #percorre ficheiros input
    for ((i=1 ; i<=$maxthreads ; i++))  #percorre threads
    do
        if [ $i == 1 ] ; then               #1 thread: nosync; 1 bucket;
            prog_name="tecnicofs-nosync"
            buckets=1
        else
            prog_name="tecnicofs-mutex"
            buckets=$numbuckets
        fi

        test_out="${output}/${test_in}-${i}.txt"            #output teste
        test_stdout="${output}/temp/${test_in}-${i}.stdout" #stdout para ficheiro

        echo "InputFile=${test_in} NumThreads=${i}"
        ./${prog_name} ${input}/${test_in} ${test_out} ${i} ${buckets} > ${test_stdout}
        rv_student=$?   #output do programa

        if [ ${rv_student} == 139 ]; then   #segfault
            echo -e "${RED}ERROR${NC}: ${YELLOW}SEGFAULT${NC}: ${test_in}"
            continue
        fi
        if [ ${rv_student} != 0 ]; then     #other error
            echo -e "${test_in}:${RED}ERROR${NC}: Program return ${YELLOW}${rv_student}${NC}!"
            continue
        fi
        cat ${test_stdout} | grep '^TecnicoFS'              #print execution time
    
    done
done
rm -f -r $output/temp       #removes temp folder and .stdout files
