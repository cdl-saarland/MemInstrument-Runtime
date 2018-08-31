#include "tree.h"

#include <stdio.h>
#include <stdlib.h>

#include "macros.h"
#include "tracer.h"

#include "tree_tools.h"

extern void *__libc_malloc(size_t size);
extern void __libc_free(void *);

static void replaceInParent(Tree *t, Node *res, Node *new) {
    ASSERTION(res != NULL, AL_TREE)
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
static void rotateRight(Tree *t, Node *p) {
    ASSERTION(p != NULL, AL_TREE)
    Node *x = p->leftChild;
    ASSERTION(x != NULL, AL_TREE)
    Node *b = x->rightChild;

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
static void rotateLeft(Tree *t, Node *p) {
    ASSERTION(p != NULL, AL_TREE)
    Node *x = p->rightChild;
    ASSERTION(x != NULL, AL_TREE)
    Node *b = x->leftChild;

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

static void splay(Tree *t, Node *x) {
    if (x == NULL) {
        return;
    }
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    while (x->parent != NULL) {
        Node *p = x->parent;
        Node *g = p->parent;
        if (g == NULL) { // ZIG
            if (x->isLeft) {
                rotateRight(t, p);
            } else {
                rotateLeft(t, p);
            }
            ASSERTION(validateTree(t), AL_TREE_COMPLEX)
            continue;
        }
        if (x->isLeft && p->isLeft) { // ZIG-ZIG (all left)
            rotateRight(t, g);
            rotateRight(t, p);
            ASSERTION(validateTree(t), AL_TREE_COMPLEX)
            continue;
        }
        if (!(x->isLeft || p->isLeft)) { // ZIG-ZIG (all right)
            rotateLeft(t, g);
            rotateLeft(t, p);
            ASSERTION(validateTree(t), AL_TREE_COMPLEX)
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
        ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    }
    ASSERTION(t->root == x, AL_TREE)
    ASSERTION(x->parent == NULL, AL_TREE)
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)

#ifdef PRINT_TREE_INTERVAL
    static unsigned int ctr = 0;
    if (ctr >= PRINT_TREE_INTERVAL) {
        dotTree(t);
        ctr = 0;
    }
    ctr++;
#endif
}

static Node *findMin(Node *n) {
    ASSERTION(n != NULL, AL_TREE)
    Node *current = n;
    while (current->leftChild != NULL) {
        current = current->leftChild;
    }
    return current;
}

static void copyData(Node *to, Node *from) {
    to->base = from->base;
    to->bound = from->bound;
}

void splayInit(Tree *t) {
    t->root = NULL;
    t->numNodes = 0;
}

static Node *find_from_node(Node *n, uintptr_t val) {
    Node *current = n;

    while (current != NULL) {
        if (val < current->base) {
            current = current->leftChild;
        } else if (val >= current->bound) {
            current = current->rightChild;
        } else {
            break;
        }
    }

    return current;
}

static Node *find_impl(Tree *t, uintptr_t val) {
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    return find_from_node(t->root, val);
}

static void removeNode(Tree *t, Node *n) {
    Node *res = n;
    ASSERTION(res != NULL && "Trying to remove non-existing element!", AL_TREE)

    tracerRegisterDelete(n->base, n->bound);

    if (res->leftChild == NULL && res->rightChild == NULL) {
        replaceInParent(t, res, NULL);
    } else if (res->leftChild == NULL) {
        replaceInParent(t, res, res->rightChild);
    } else if (res->rightChild == NULL) {
        replaceInParent(t, res, res->leftChild);
    } else {
        Node *E = findMin(res->rightChild);
        copyData(res, E);
        replaceInParent(t, E, E->rightChild);
        res = E;
    }

    __libc_free(res);

    t->numNodes--;

    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
}

bool splayRemove(Tree *t, uintptr_t val) {
    Node *res = find_impl(t, val);
    if (res == NULL) {
        return false;
    }
    removeNode(t, res);
    return true;
}

size_t splayRemoveInterval_from_node(Tree *t, Node *n, uintptr_t low,
                                     uintptr_t high) {
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    if (n == NULL) {
        return 0;
    }
    if (low >= n->base && n->bound >= high) {
        // the interval is fully contained in this node, therefore we do not
        // need to consider its children
        removeNode(t, n);
        return 1;
    }

    size_t res = 0;

    if (!(n->base <= low)) {
        // no node on the left of n can intersect with the interval otherwise
        res += splayRemoveInterval_from_node(t, n->leftChild, low, high);
    }
    if (!(n->bound >= high)) {
        // no node on the right of n can intersect with the interval otherwise
        res += splayRemoveInterval_from_node(t, n->rightChild, low, high);
    }

    if ((low <= n->base && n->bound <= high) ||
        // the node is fully contained in the interval
        (n->base <= low && n->bound > low) ||
        // the node partially intersects with the lower part of the interval
        (n->base < high && n->bound >= high)) {
        // the node partially intersects with the upper part of the interval
        removeNode(t, n);
        ++res;
    }
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    return res;
}

/**
 * Removes all nodes in t that intersect with [low, high)
 */
size_t splayRemoveInterval(Tree *t, uintptr_t low, uintptr_t high) {
    return splayRemoveInterval_from_node(t, t->root, low, high);
}

Node *splayFind(Tree *t, uintptr_t val) {
    Node *current = find_impl(t, val);
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    splay(t, current);
    return current;
}

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

void splayInsert(Tree *t, uintptr_t base, uintptr_t bound, InsertBehavior ib) {
    ASSERTION(base < bound, AL_TREE)
    Node *parent = NULL;
    Node *current = t->root;
    Node *newNode = __libc_malloc(sizeof(Node));

    newNode->leftChild = NULL;
    newNode->rightChild = NULL;
    newNode->base = base;
    newNode->bound = bound;

#ifdef TREE_ANNOTATE_NODES
    char kind = 'u'; // unknown
    switch (ib) {
    case IB_EXTEND:
        kind = 'g'; // global/function
        tracerRegisterInsert(base, bound, "global");
        break;

    case IB_ERROR:
        kind = 'h'; // heap
        tracerRegisterInsert(base, bound, "heap");
        break;

    case IB_REPLACE:
        kind = 's'; // stack
        tracerRegisterInsert(base, bound, "stack");
        break;
    }
    newNode->kind = kind;
#else
    tracerRegisterInsert(base, bound, "unknown");
#endif

    bool left = false;

    while (current != NULL) {
        parent = current;
        if (bound <= current->base) {
            current = current->leftChild;
            left = true;
        } else if (base >= current->bound) {
            current = current->rightChild;
            left = false;
        } else {
            switch (ib) {
            case IB_ERROR:
                UNREACHABLE("Trying to insert a conflicting element!");
                __libc_free(newNode);
                return;
            case IB_EXTEND: {

#ifdef TREE_ANNOTATE_NODES
                ASSERTION(current->kind == 'g', AL_TREE);
#endif
                bool fullyContained =
                    current->base <= base && current->bound >= bound;
                if (!fullyContained) {
                    splayRemoveInterval_from_node(t, current->leftChild, base,
                                                  bound);
                    splayRemoveInterval_from_node(t, current->rightChild, base,
                                                  bound);
                }
                tracerRegisterDelete(current->base, current->bound);
                current->base = min(current->base, base);
                current->bound = max(current->bound, bound);

                splay(t, current);
                __libc_free(newNode);
                return;
            }
            case IB_REPLACE: {
#ifdef TREE_ANNOTATE_NODES
                ASSERTION(current->kind == 's', AL_TREE);
#endif
                bool fullyContained =
                    current->base <= base && current->bound >= bound;
                if (!fullyContained) {
                    splayRemoveInterval_from_node(t, current->leftChild, base,
                                                  bound);
                    splayRemoveInterval_from_node(t, current->rightChild, base,
                                                  bound);
                }
                tracerRegisterDelete(current->base, current->bound);

                current->base = base;
                current->bound = bound;

                splay(t, current);
                __libc_free(newNode);
                return;
            }
            }
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
    t->numNodes++;
    ASSERTION(validateTree(t), AL_TREE_COMPLEX)
    splay(t, newNode);
}
