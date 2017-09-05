#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// #define ASSERTS 1
// #define DBG 1
// #define PRINT_TREE_INTERVAL 500000

#ifdef ASSERTS
#define ASSERTION(x) do {x;} while (false);
#else
#define ASSERTION(x) do {} while (false);
#endif


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
#ifdef DBG
    size_t numNodes;
#endif
} Tree;

typedef enum {
    IB_EXTEND,
    IB_REPLACE,
    IB_ERROR,
} InsertBehavior;

void splayFail(const char* msg, void *faultingPtr);
void splayInit(Tree *t);
void splayInsert(Tree* t, uintptr_t base, uintptr_t bound, InsertBehavior ib);
void splayRemove(Tree* t, uintptr_t val);
Node* splayFind(Tree* t, uintptr_t val);


