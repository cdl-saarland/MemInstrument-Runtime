#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"

typedef struct Node {
    struct Node *leftChild;
    struct Node *rightChild;
    struct Node *parent;
    bool isLeft;

    uintptr_t base;  // including
    uintptr_t bound; // excluding

#ifdef TREE_ANNOTATE_NODES
    char kind;
#endif
} Node;

typedef struct Tree {
    Node *root;
    size_t numNodes;
} Tree;

typedef enum {
    IB_EXTEND,
    IB_REPLACE,
    IB_ERROR,
} InsertBehavior;

void splayInit(Tree *t);
void splayInsert(Tree *t, uintptr_t base, uintptr_t bound, InsertBehavior ib);
bool splayRemove(Tree *t, uintptr_t val);
Node *splayFind(Tree *t, uintptr_t val);

void __dumpAllocationMap(FILE *F, const Tree *T);
