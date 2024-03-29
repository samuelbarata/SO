/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

/* bst.h */
#ifndef BST_H
#define BST_H
#include <stdio.h>
#include "../lib/globals.h"


typedef struct node {
    char* key;
    int inumber;
    struct node* left;
    struct node* right;
} node;

void insertDelay(int cycles);
node *search(node *p, char* key);
node *insert(node *p, char* key, int inumber);
node *find_min(node *p);
node *remove_min(node *p);
node *remove_item(node *p, char* key);
node *inumberLookup(node *p, int inumber);
void free_tree(node *p);
void print_tree(FILE* fp, node *p);

#endif /* BST_H */
