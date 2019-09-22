###Analisar Core:
$ gdb <nome executavel> <core dump (core)>

backtrace   `bt`                //mostra o caminho percorrido até ao erro (pilha de chamadas)
frame       `frame <#>`         //mostra em detalhe o passo #
            //usar comando p para analisar variaveis


###Debug Exec:
$ gdb <nome executavel>

break   `b <filename>:<linha>`  //adiciona breakpoint na linha indicada
break   `b <nome função>`       //adiciona breakpoint na primeira linha da função
info    `info b`                //mostra os breakpoints existentes e o seu numero
        `disable <n>`               //desativa o breakpoin numero n
        `enable <n>`                //ativa o breakpoin numero n
        `delete <n>`                //apaga o breakpoin numero n
run     `r`                     //corre o ficheiro
list    `l`                     //mostra em detalhe o codigo onde parou
nest    `n`                     //salta para a instrução seguinte
?       `p <variavel>`          //mostra os detalhes da variavel selecionada
continue    `c`                 //continua o codigo até ao próximo brakepoint
step    `s`                     //entra na função
finish  `fin`                   //corre até ao fim da função atual
quit    `q`                     //sai do gdb
