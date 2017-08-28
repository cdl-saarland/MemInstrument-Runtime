#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

void splayInit(Tree *t);
void splayInsert(Tree* t, uintptr_t base, uintptr_t bound, bool extend);
void splayRemove(Tree* t, uintptr_t val);
Node* splayFind(Tree* t, uintptr_t val);


