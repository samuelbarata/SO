#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "bst.h"

void list_tree(node* p);

int main(void)
{
    node* root = NULL;
    int debug = 0;
    char c;
    int k;

    printf("> ");
    while ( scanf("%c", &c) == 1 && c != 'q' )
    {
        if ( c == 'a' )
        {
            scanf("%d", &k);
            root = insert(root, k, "empty");
	    if (debug) print_tree(root);
        }
        else if ( c == 's' )
        {
            scanf("%d", &k);
            node* n = search(root, k);
            if ( n ) 
                printf("%ld data=\"%s\"\n", n -> key, n -> data);
        }
        else if ( c == 'r' )
        {
            scanf("%d", &k);
            root = remove_item(root, k);
	    if (debug) print_tree(root);
        }
        else if ( c == 'd' ) debug = 1 - debug;
        else if ( c == 'p' ) print_tree(root);
        else if (c=='l') list_tree(root);
        else if (c=='f'){
            pid_t pid;
            pid = fork();
            if(pid>0)
                wait(NULL);
        }
        if( c == '\n')
            printf("> ");
    }

    free_tree(root);

    return 0;
}

void list_tree(node* p){
    if(p==NULL) return;
    list_tree(p->left);
    printf("%ld\n", p->key);
    list_tree(p->right);
}
