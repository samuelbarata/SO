#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;92m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BLINK='\e[5m'
NB='\e[25m'
NC='\033[0m'
BOLD='\033[1m'
NORM='\033[0m'

if [ $# -lt 4 ] ; then
    echo "Usage: $0 tecnicofs threads numbuckets cycleCount"
    echo "Example: ${0} tecnicofs-mutex 2 100"
    exit 1
fi


if  ! ( which `cut <<<valgrind -f1 -d\ ` >/dev/null 2>&1 ) ; then		#se falhar aqui 
    BLINK=''
    NB=''
fi

mkdir output 2> /dev/null
mkdir output/tecnicofs-nosync 2> /dev/null
mkdir output/tecnicofs-mutex 2> /dev/null
mkdir output/tecnicofs-rwlock 2> /dev/null
mkdir output/tecnicofs-nosync/inputs 2> /dev/null
mkdir output/tecnicofs-mutex/inputs 2> /dev/null
mkdir output/tecnicofs-rwlock/inputs 2> /dev/null

prog_name=$1
threads=$2
buckets=$3
cycleCount=$(($4 + 1 - 1))

test_dir='inputs'

error_file="${prog_name}.error"
touch $error_file
for ((i=0 ; i<$cycleCount ; i++))
do
    for test_in in `ls -rS ${test_dir}/*`; do
        test_out="output/${prog_name}/${test_in}.${threads}.out"
        test_stdout="output/${prog_name}/${test_in}.${threads}.stdout"
        ./${prog_name} ${test_in} ${test_out} ${threads} ${buckets} > ${test_stdout}
        rv_student=$?

        if [ ${rv_student} == 139 ]; then
            echo -e "${RED}${BOLD}ERROR${NORM}${NC}: ${YELLOW}SEGFAULT${NC}: ${test_in%.in}" >> $error_file
            echo -e "${RED}${BOLD}ERROR${NORM}${NC}: ${YELLOW}SEGFAULT${NC}: ${test_in%.in}"
            continue
        fi

        if [ ${rv_student} != 0 ]; then
            echo -e "${test_in%.in}:${RED}ERROR${NC}: Program return ${YELLOW}${rv_student}${NC}!" >> $error_file
            echo -e "${test_in%.in}:${RED}ERROR${NC}: Program return ${YELLOW}${rv_student}${NC}!"
            continue
        fi

    done > /dev/null
done

errorCount=$(wc -l < ${error_file})
errors=$(printf "%d" $errorCount)
echo -e ${RED}$1${NC} ran with ${RED}$2${NC} threads ${RED}$3${NC} times
if [ ${errors} == 0 ]; then
    echo -e "${YELLOW}╔════════════════╗${NC}"
    echo -e "${YELLOW}║   ${GREEN}${BLINK}No errors!${NB}${YELLOW}   ║${NC}"
    echo -e "${YELLOW}╚════════════════╝${NC}"

else
    errors=$(printf "%03d" $errorCount)
    echo -e "${YELLOW}╔══════════════════════╗${NC}"
    echo -e "${YELLOW}║          ${RED}${BLINK}FAIL${NB}${YELLOW}        ║${NC}"
    echo -e "${YELLOW}║   ${RED}FAILED ${BLUE}${BLINK}${errors}${NB} ${RED}tasks${YELLOW}   ║${NC}"
    echo -e "${YELLOW}╚══════════════════════╝${NC}"
    cat $error_file
    echo
fi
rm -f ${error_file}