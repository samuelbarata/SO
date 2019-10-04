RED='\033[0;31m'
GREEN='\033[0;92m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BLINK='\e[5m'
NB='\e[25m'
NC='\033[0m'
BOLD='\033[1m'
NORM='\033[0m'

if [ $# -lt 2 ] ; then
    echo "Usage: $0 tecnicofs threads"
    echo "Example: ${0} tecnicofs-mutex 2"
    exit 1
fi

#if  ! ( which `cut <<<pv -f1 -d\ ` >/dev/null 2>&1 ) ; then		#se falhar aqui provavelmente não tem valgrind, provavelmente está num mac e apaga tralha desnecessária
#    echo -e "${RED}Error${NC}: please install pv"
#    echo -e "${YELLOW}Example${NC}: sudo apt install pv"
#    exit 2
#fi

if  ! ( which `cut <<<valgrind -f1 -d\ ` >/dev/null 2>&1 ) ; then		#se falhar aqui provavelmente não tem valgrind, provavelmente está num mac e apaga tralha desnecessária
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

test_dir='inputs'

error_file="${prog_name}.error"
touch $error_file
for test_in in `ls -rS ${test_dir}/*`; do
    test_out="output/${prog_name}/${test_in}.${threads}.out"
    test_stdout="output/${prog_name}/${test_in}.${threads}.stdout"
    ./${prog_name} ${test_in} ${test_out} ${threads} > ${test_stdout} 2>${error_file}
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

done > /dev/null # | pv -pt -i0.1 -l ${NOF} > /dev/null


lol=$(wc -l < ${error_file})
errors=$(printf "%d" $lol)
if [ ${errors} == 0 ]; then
    echo -e "${YELLOW}╔════════════════╗"
    echo -e "║   ${GREEN}${BLINK}No errors!${NB}${YELLOW}   ║"
    echo -e "╚════════════════╝${NC}"

else
    errors=$(printf "%03d" $lol)
    echo -e "${YELLOW}╔══════════════════════╗"
    echo -e "║   ${RED}${BLINK}GIGANTIC FAILURE${NB}${YELLOW}   ║"
    echo -e "║   ${RED}FAILED ${BLUE}${BLINK}${errors}${NB} ${RED}tasks${YELLOW}   ║"
    echo -e "╚══════════════════════╝${NC}"
    cat $error_file
    echo
fi
rm -f ${error_file}