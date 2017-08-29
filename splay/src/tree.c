#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


extern void *__libc_malloc(size_t size);
extern void __libc_free(void*);

extern char* __progname;

static bool validateNode(Node* parent, Node* n, size_t *expectedNodes) {
    if (n == NULL) {
        return true;
    }
    if (n->base > n->bound) {
        fprintf(stderr, "Node with invalid interval found!\n");
        return false;
    }
    if (n->parent != parent) {
        fprintf(stderr, "Node with wrong parent found!\n");
        return false;
    }
    if (parent != NULL) {
        if (n->isLeft) {
            if (n->parent->leftChild != n) {
                fprintf(stderr, "Node with wrong left information found!\n");
                return false;
            }
            if (parent->base <= n->bound) {
                fprintf(stderr, "Left node with wrong order found!\n");
                return false;
            }
        }
        if (!n->isLeft) {
            if (n->parent->rightChild != n) {
                fprintf(stderr, "Node with wrong right information found!\n");
                return false;
            }
            if (parent->base >= n->base) {
                fprintf(stderr, "Right node with wrong order found!\n");
                return false;
            }
        }
    }

    if (expectedNodes == 0) {
        fprintf(stderr, "Found more nodes than expected (or a loop)!\n");
        return false;
    }
    (*expectedNodes)--;
    return validateNode(n, n->leftChild, expectedNodes) &&
        validateNode(n, n->rightChild, expectedNodes);
}

static bool validateTree(Tree *t) {
    size_t expected = t->numNodes;
    bool res = validateNode(NULL, t->root, &expected);
    if (!res) {
        return false;
    }
    if (expected != 0) {
        fprintf(stderr, "Found fewer nodes than expected!\n");
        return false;
    }
    return true;
}

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
    /* assert(false); */
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
    DEBUG(assert(validateTree(t)))
    while (x->parent != NULL) {
        Node* p = x->parent;
        Node* g = p->parent;
        if (g == NULL) { // ZIG
            if (x->isLeft) {
                rotateRight(t, p);
            } else {
                rotateLeft(t, p);
            }
            DEBUG(assert(validateTree(t)))
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
            DEBUG(assert(validateTree(t)))
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
        DEBUG(assert(validateTree(t)))
    }
    assert(t->root == x);
    assert(x->parent == NULL);
    DEBUG(assert(validateTree(t)))

#ifdef PRINT_TREE_INTERVAL
    static unsigned int ctr = 0;
    if (ctr >= PRINT_TREE_INTERVAL) {
        dotTree(t);
        ctr = 0;
    }
    ctr++;
#endif
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

void splayInit(Tree *t) {
    t->root = NULL;
    t->numNodes = 0;
}

static Node* find_impl(Tree* t, uintptr_t val) {
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

    DEBUG(assert(validateTree(t)))
    return current;
}

static void removeNode(Tree* t, Node* n) {
    Node* res = n;
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
        replaceInParent(t, E, E->rightChild);
        res = E;
    }

    __libc_free(res);

    t->numNodes--;

    DEBUG(assert(validateTree(t)))
}

void splayRemove(Tree* t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayRemove(%8lx)\n", val))
    Node* res = find_impl(t, val);
    if (res == NULL) { //FIXME
        DEBUG(fprintf(stderr, "  early return splayRemove(%8lx)\n", val))
        return;
    }
    removeNode(t, res);
    DEBUG(fprintf(stderr, "  return splayRemove(%8lx)\n", val))
}

Node* splayFind(Tree* t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayFind(%8lx)\n", val))

    Node* current = find_impl(t, val);

    DEBUG(assert(validateTree(t)))
    splay(t, current);
    DEBUG(fprintf(stderr, "  return splayFind(%8lx)\n", val))
    return current;
}

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

void splayInsert(Tree* t, uintptr_t base, uintptr_t bound, InsertBehavior ib) {
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
        } else if (base >= current->bound) {
            current = current->rightChild;
            left = false;
        } else {
            switch (ib) {
            case IB_ERROR:
                assert(false && "Trying to insert a conflicting element!");
                return;
            case IB_EXTEND:
                current->base = min(current->base, base);
                current->base = max(current->bound, bound);
                splay(t, current);
                DEBUG(fprintf(stderr, "  return splayInsert(%8lx, %8lx) -- extended\n", base, bound))
                return;
            case IB_REPLACE:
                current->base = base;
                current->base = bound;
                splay(t, current);
                DEBUG(fprintf(stderr, "  return splayInsert(%8lx, %8lx) -- replaced\n", base, bound))
                return;
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
    DEBUG(assert(validateTree(t)))
    splay(t, newNode);
    DEBUG(fprintf(stderr, "  return splayInsert(%8lx, %8lx)\n", base, bound))
}
