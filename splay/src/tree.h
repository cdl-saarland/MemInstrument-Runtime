#pragma once

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
} Tree;

void splayInsert(Tree* t, uintptr_t base, uintptr_t bound);
void splayRemove(Tree* t, uintptr_t val);
Node* splayFind(Tree* t, uintptr_t val);


