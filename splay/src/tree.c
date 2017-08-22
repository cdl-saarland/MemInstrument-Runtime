#include "tree.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

extern void *__libc_malloc(size_t size);
extern void __libc_free(void*);

void replaceInParent(Tree* t, Node* res, Node* new) {
    assert(res != NULL);
    if (new != NULL) {
        new->parent = res->parent;
    }
    if (res->parent == NULL) {
        t->root = new;
        return;
    }

    if (res->isLeft) {
        res->parent->leftChild = new;
    } else {
        res->parent->rightChild = new;
    }
}

/**
 *      p
 *     / \
 *    x   c
 *   / \
 *  a   b
 *
 *  becomes
 *
 *      x
 *     / \
 *    a   p
 *       / \
 *      b   c
 */
void rotateRight(Tree* t, Node* p) {
    assert(p != NULL);
    Node* x = p->leftChild;
    assert(x != NULL);
    Node* b = x->rightChild;

    replaceInParent(t, p, x);
    replaceInParent(t, b, p);
    p->leftChild = b;

    if (b != NULL) {
        b->parent = p;
    }
}

/**
 *      p
 *     / \
 *    a   x
 *       / \
 *      b   c
 *
 *  becomes
 *
 *      x
 *     / \
 *    p   c
 *   / \
 *  a   b
 */
void rotateLeft(Tree* t, Node* p) {
    assert(p != NULL);
    Node* x = p->rightChild;
    assert(x != NULL);
    Node* b = x->leftChild;

    replaceInParent(t, p, x);
    replaceInParent(t, b, p);
    p->rightChild = b;

    if (b != NULL) {
        b->parent = p;
    }
}

void splay(Tree* t, Node* x) {
    assert(x != NULL);
    while (x->parent != NULL) {
        Node* p = x->parent;
        Node* g = p->parent;
        if (g == NULL) { // ZIG
            if (x->isLeft) {
                rotateRight(t, p);
            } else {
                rotateLeft(t, p);
            }
            continue;
        }
        if (x->isLeft == p->isLeft) { // ZIG-ZIG
            if (x->isLeft) {
                rotateRight(t, g);
                rotateRight(t, p);
            } else {
                rotateLeft(t, g);
                rotateLeft(t, p);
            }
            continue;
        }
        // ZIG-ZAG
        if (x->isLeft) {
            rotateRight(t, p);
            rotateLeft(t, g);
        } else {
            rotateLeft(t, p);
            rotateRight(t, g);
        }
    }
    assert(t->root == x);
    assert(x->parent == NULL);
}

Node* findMin(Node* n) {
    assert(n != NULL);
    Node* current = n;
    while (current->leftChild != NULL) {
        current = current->leftChild;
    }
    return current;
}

void copyData(Node* to, Node* from) {
    to->base = from->base;
    to->bound = from->bound;
}

void remove(Tree* t, uintptr_t val) {
    Node* res = find(t, val);
    assert(res != NULL && "Trying to remove non-existing element!");

    if (res->leftChild == NULL && res->rightChild == NULL) {
        replaceInParent(t, res, NULL);
    } else if (res->leftChild == NULL) {
        replaceInParent(t, res, res->rightChild);
    } else if (res->rightChild == NULL) {
        replaceInParent(t, res, res->leftChild);
    } else {
        Node* E = findMin(res->rightChild);
        copyData(res, E);
        if (E->rightChild != NULL) {
            replaceInParent(t, E, E->rightChild);
        }
        res = E;
    }

    __libc_free(res);
    return;
}

Node* find(Tree* t, uintptr_t val) {
    Node* current = t->root;

    while (current != NULL) {
        if (val < current->base) {
            current = current->leftChild;
        } else if (val >= current->bound) {
            current = current->rightChild;
        } else {
            break;
        }
    }

    splay(t, current);
    return current;
}

void insert(Tree* t, uintptr_t base, uintptr_t bound) {
    Node* parent = NULL;
    Node* current = t->root;
    Node* newNode = __libc_malloc(sizeof(Node));

    newNode->leftChild = NULL;
    newNode->rightChild = NULL;
    newNode->base = base;
    newNode->bound = bound;

    bool left = false;

    while (current != NULL) {
        parent = current;
        if (bound <= current->base) {
            current = current->leftChild;
            left = true;
        } else if (base > current->bound) {
            current = current->rightChild;
            left = false;
        } else {
            assert(false && "Trying to insert a conflicting element!");
        }
    }

    newNode->parent = parent;
    newNode->isLeft = left;

    if (parent == NULL) {
        t->root = newNode;
    }
    splay(t, current);
}
