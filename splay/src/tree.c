#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

extern void *__libc_malloc(size_t size);
extern void __libc_free(void*);

extern char* __progname;

#define DEBUG(x) do {x;} while (false);

static void dotNodeLabel(FILE* F, Node* n) {
    if (n == NULL) {
        return;
    }
    fprintf(F, "  n%lx [label=\"base:  %08lx\nbound: %08lx\"];\n", n->base,
            n->base, n->bound);
    dotNodeLabel(F, n->leftChild);
    dotNodeLabel(F, n->rightChild);
}


static void dotFromNode(FILE* F, Node* n) {
    if (n == NULL) {
        return;
    }
    if (n->leftChild != NULL) {
        fprintf(F, "  n%08lx -> n%08lx [label=%s];\n", n->base, n->leftChild->base, "l");
    }
    if (n->rightChild != NULL) {
        fprintf(F, "  n%08lx -> n%08lx [label=%s];\n", n->base, n->rightChild->base, "r");
    }
    dotFromNode(F, n->leftChild);
    dotFromNode(F, n->rightChild);
}

static void dotTree(Tree* t) {
    static unsigned num = 0;// TODO atomic
    size_t pn_sz = strlen(__progname);
    size_t sz = pn_sz + strlen("splay..dot") + 1 + 10;
    char buf[sz];
    sprintf(buf, "splay.%s.%u.dot", __progname, ++num);
    FILE* F = fopen(buf, "w");

    fprintf(F, "digraph splaytree_%s_%u\n{\n", __progname, num);
    fprintf(F, "  label=\"splaytree %s %u\";\n", __progname, num);
    fprintf(F, "  labelloc=top;\n");
    fprintf(F, "  labeljust=left;\n\n");
    dotNodeLabel(F, t->root);
    fprintf(F, "\n");
    dotFromNode(F, t->root);
    fprintf(F, "}\n");

    fclose(F);
}

static void replaceInParent(Tree* t, Node* res, Node* new) {
    assert(res != NULL);
    if (new != NULL) {
        new->parent = res->parent;
        new->isLeft = res->isLeft;
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
static void rotateRight(Tree* t, Node* p) {
    assert(p != NULL);
    Node* x = p->leftChild;
    assert(x != NULL);
    Node* b = x->rightChild;

    replaceInParent(t, p, x);
    x->rightChild = p;
    p->parent = x;
    p->isLeft = false;

    p->leftChild = b;
    if (b != NULL) {
        b->isLeft = true;
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
static void rotateLeft(Tree* t, Node* p) {
    assert(p != NULL);
    Node* x = p->rightChild;
    assert(x != NULL);
    Node* b = x->leftChild;

    replaceInParent(t, p, x);
    x->leftChild = p;
    p->parent = x;
    p->isLeft = true;

    p->rightChild = b;
    if (b != NULL) {
        b->isLeft = false;
        b->parent = p;
    }
}

static void splay(Tree* t, Node* x) {
    if (x == NULL) {
        return;
    }
    /* DEBUG(dotTree(t)) */
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
    dotTree(t);
}

static Node* findMin(Node* n) {
    assert(n != NULL);
    Node* current = n;
    while (current->leftChild != NULL) {
        current = current->leftChild;
    }
    return current;
}

static void copyData(Node* to, Node* from) {
    to->base = from->base;
    to->bound = from->bound;
}

void splayRemove(Tree* t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayRemove(%8lx)\n", val))
    Node* res = splayFind(t, val);
    if (res == NULL) {
        DEBUG(fprintf(stderr, "  early return splayRemove(%8lx)\n", val))
        return;
    }
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
    DEBUG(fprintf(stderr, "  return splayRemove(%8lx)\n", val))
    return;
}

Node* splayFind(Tree* t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayFind(%8lx)\n", val))
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
    DEBUG(fprintf(stderr, "  return splayFind(%8lx)\n", val))
    return current;
}

void splayInsert(Tree* t, uintptr_t base, uintptr_t bound) {
    DEBUG(fprintf(stderr, "  call splayInsert(%8lx, %8lx)\n", base, bound))
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
    } else {
        if (left) {
            parent->leftChild = newNode;
        } else {
            parent->rightChild = newNode;
        }
    }
    splay(t, newNode);
    DEBUG(fprintf(stderr, "  return splayInsert(%8lx, %8lx)\n", base, bound))
}
