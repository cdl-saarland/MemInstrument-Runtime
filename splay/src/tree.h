#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// #define DBG 1
// #define PRINT_TREE_INTERVAL 1000

#ifdef DBG
#define DEBUG(x) do {x;} while (false);
#else
#define DEBUG(x) do {} while (false);
#endif


typedef struct Node {
    struct Node* leftChild;
    struct Node* rightChild;
    struct Node* parent;
    bool isLeft;

    uintptr_t base; // including
    uintptr_t bound; // excluding
} Node;

typedef struct Tree {
    Node* root;
    size_t numNodes;
} Tree;

typedef enum {
    IB_EXTEND,
    IB_REPLACE,
    IB_ERROR,
} InsertBehavior;

void splayInit(Tree *t);
void splayInsert(Tree* t, uintptr_t base, uintptr_t bound, InsertBehavior ib);
void splayRemove(Tree* t, uintptr_t val);
Node* splayFind(Tree* t, uintptr_t val);


