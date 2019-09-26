#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 ______________________________ 
< All constants are variables. >
 ------------------------------ 
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/


//aula 24/09/19
void aula00(){
    /*pid() devolve o pid do outro processo
    getpid() devolve o pid do proprio processo*/
    printf("parent process id = %d\n", getpid());
    fork();
    {   //corrido por ambos os processos
    printf("my process id = %d\n", getpid()); //vai ser corrido por cada fork;
    sleep(5);
    exit(0);
    }
}

void comboios(){
    int pid, estado;
    int time0, time1, time2;
    time0=clock();
    pid = fork();
    /*quando se pede "pid", é devolvido o pid do OUTRO processo
    ou seja, ao pai é devolvido o pid do filho,
    ao filho é devolvido o pid do pai (0)*/
    if(pid==0){
        /*filho*/
        //printf("pai: %d\n", pid/*do pai*/);
        system("sl");
        exit(0);
    } else {
        /*pai*/
        //printf("filho: %d\n", pid/*do filho*/);
        system("sl");
        pid = wait(&estado);
    }
    time1 = clock();

    system("sl");
    time2 = clock();

    printf("2 comboios ao mesmo tempo: %d\n", time1-time0);
    printf("1 comboio: %d\n", time2-time1);
}

//aula 26/09/19
void aula01(){
    //execl(executavel, comandos,...)
    //execv(executavel, [comandos])
    //corre main de outro executavel
    FILE *fp;
    int *mydata;
    fp=fopen("gdb.md", "r");
    if(!fp){
        perror("gdb.md");
    }
    fclose(fp);
    fp=fopen("test.txt", "w");
    if(!fp){
        perror("test.txt");
        exit(1);
    }
    fprintf(fp, "Hi file!\n");  //guarda em RAM e so escreve o mais tarde possivel => really fast
    fputs(fp, "hallo");
    fscanf(fp,"%f" , &mydata);
    fseek(fp, 200,1);
    ftell(fp);
    fflush(fp);                 //força as alterações a serem gravadas
    fclose(fp);



    //pthread_create() ???
}

int main(){
    //aula00();
    //comboios();
    aula01();
}
