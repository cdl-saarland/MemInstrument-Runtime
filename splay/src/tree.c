#include "tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_TRACER
#include "tracer.h"
#endif

extern void *__libc_malloc(size_t size);
extern void __libc_free(void *);

#ifdef STATISTICS
#define STATS(x)                                                               \
    do {                                                                       \
        x;                                                                     \
    } while (false);
#else
#define STATS(x)                                                               \
    do {                                                                       \
    } while (false);
#endif

#ifdef ASSERTS
#define ASSERTION(x)                                                           \
    do {                                                                       \
        x;                                                                     \
    } while (false);
#else
#define ASSERTION(x)                                                           \
    do {                                                                       \
    } while (false);
#endif

#ifdef DBG
#define DEBUG(x)                                                               \
    do {                                                                       \
        x;                                                                     \
    } while (false);
#else
#define DEBUG(x)                                                               \
    do {                                                                       \
    } while (false);
#endif

#ifdef DBG
// Functions for expensive debug checks and printing

static bool validateNode(Node *parent, Node *n, size_t *expectedNodes) {
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
            if (parent->base < n->bound) {
                fprintf(stderr,
                        "Left node [%p, %p) with wrong order w.r.t parent node "
                        "[%p, %p) found!\n",
                        (void *)n->base, (void *)n->bound, (void *)parent->base,
                        (void *)parent->bound);
                return false;
            }
        }
        if (!n->isLeft) {
            if (n->parent->rightChild != n) {
                fprintf(stderr, "Node with wrong right information found!\n");
                return false;
            }
            if (parent->bound > n->base) {
                fprintf(stderr,
                        "Right node [%p, %p) with wrong order w.r.t parent "
                        "node [%p, %p) found!\n",
                        (void *)n->base, (void *)n->bound, (void *)parent->base,
                        (void *)parent->bound);
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
#endif

#ifdef PRINT_TREE_INTERVAL
extern char *__progname;

static void dotNodeLabel(FILE *F, Node *n) {
    if (n == NULL) {
        return;
    }
    fprintf(F, "  n%lx [label=\"base:  %08lx\nbound: %08lx\"];\n", n->base,
            n->base, n->bound);
    dotNodeLabel(F, n->leftChild);
    dotNodeLabel(F, n->rightChild);
}

static void dotFromNode(FILE *F, Node *n) {
    if (n == NULL) {
        return;
    }
    if (n->leftChild != NULL) {
        fprintf(F, "  n%08lx -> n%08lx [label=%s];\n", n->base,
                n->leftChild->base, "l");
    }
    if (n->rightChild != NULL) {
        fprintf(F, "  n%08lx -> n%08lx [label=%s];\n", n->base,
                n->rightChild->base, "r");
    }
    dotFromNode(F, n->leftChild);
    dotFromNode(F, n->rightChild);
}

static void dotTree(Tree *t) {
    static unsigned num = 0; // TODO atomic
    size_t pn_sz = strlen(__progname);
    size_t sz = pn_sz + strlen("splay..dot") + 1 + 10;
    char buf[sz];
    sprintf(buf, "splay.%s.%u.dot", __progname, ++num);
    FILE *F = fopen(buf, "w");

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

#endif

static void replaceInParent(Tree *t, Node *res, Node *new) {
    ASSERTION(assert(res != NULL);)
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
    ASSERTION(assert(p != NULL);)
    Node *x = p->leftChild;
    ASSERTION(assert(x != NULL);)
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
    ASSERTION(assert(p != NULL);)
    Node *x = p->rightChild;
    ASSERTION(assert(x != NULL);)
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
    DEBUG(assert(validateTree(t)))
    while (x->parent != NULL) {
        Node *p = x->parent;
        Node *g = p->parent;
        if (g == NULL) { // ZIG
            if (x->isLeft) {
                rotateRight(t, p);
            } else {
                rotateLeft(t, p);
            }
            DEBUG(assert(validateTree(t)))
            continue;
        }
        if (x->isLeft && p->isLeft) { // ZIG-ZIG (all left)
            rotateRight(t, g);
            rotateRight(t, p);
            DEBUG(assert(validateTree(t)))
            continue;
        }
        if (!(x->isLeft || p->isLeft)) { // ZIG-ZIG (all right)
            rotateLeft(t, g);
            rotateLeft(t, p);
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
    ASSERTION(assert(t->root == x);)
    ASSERTION(assert(x->parent == NULL);)
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

static Node *findMin(Node *n) {
    ASSERTION(assert(n != NULL);)
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
#ifdef DBG
    t->numNodes = 0;
#endif
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
    DEBUG(assert(validateTree(t)))
    return find_from_node(t->root, val);
}

static void removeNode(Tree *t, Node *n) {
    Node *res = n;
    ASSERTION(assert(res != NULL && "Trying to remove non-existing element!");)

#ifdef ENABLE_TRACER
    tracerRegisterDelete(n->base, n->bound);
#endif

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

#ifdef DBG
    t->numNodes--;
#endif

    DEBUG(assert(validateTree(t)))
}

bool splayRemove(Tree *t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayRemove(%8lx)\n", val))
    Node *res = find_impl(t, val);
    if (res == NULL) {
        return false;
    }
    removeNode(t, res);
    DEBUG(fprintf(stderr, "  return splayRemove(%8lx)\n", val))
    return true;
}

size_t splayRemoveInterval_from_node(Tree *t, Node *n, uintptr_t low,
                                     uintptr_t high) {
    DEBUG(assert(validateTree(t)))
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
    DEBUG(assert(validateTree(t)))
    return res;
}

/**
 * Removes all nodes in t that intersect with [low, high)
 */
size_t splayRemoveInterval(Tree *t, uintptr_t low, uintptr_t high) {
    return splayRemoveInterval_from_node(t, t->root, low, high);
}

Node *splayFind(Tree *t, uintptr_t val) {
    DEBUG(fprintf(stderr, "  call splayFind(%8lx)\n", val))

    Node *current = find_impl(t, val);

    DEBUG(assert(validateTree(t)))
    splay(t, current);
    DEBUG(fprintf(stderr, "  return splayFind(%8lx)\n", val))
    return current;
}

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

void splayInsert(Tree *t, uintptr_t base, uintptr_t bound, InsertBehavior ib) {
    DEBUG(fprintf(stderr, "  call splayInsert(%8lx, %8lx)\n", base, bound))
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
#ifdef ENABLE_TRACER
        tracerRegisterInsert(base, bound, "global");
#endif
        break;

    case IB_ERROR:
        kind = 'h'; // heap
#ifdef ENABLE_TRACER
        tracerRegisterInsert(base, bound, "heap");
#endif
        break;

    case IB_REPLACE:
        kind = 's'; // stack
#ifdef ENABLE_TRACER
        tracerRegisterInsert(base, bound, "stack");
#endif
        break;
    }
    newNode->kind = kind;
#else
#ifdef ENABLE_TRACER
    tracerRegisterInsert(base, bound, "unknown");
#endif
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
                assert(false && "Trying to insert a conflicting element!");
                __libc_free(newNode);
                return;
            case IB_EXTEND: {

#ifdef TREE_ANNOTATE_NODES
                assert(current->kind == 'g');
#endif
                bool fullyContained =
                    current->base <= base && current->bound >= bound;
                if (!fullyContained) {
                    splayRemoveInterval_from_node(t, current->leftChild, base,
                                                  bound);
                    splayRemoveInterval_from_node(t, current->rightChild, base,
                                                  bound);
                }
#ifdef ENABLE_TRACER
                tracerRegisterDelete(current->base, current->bound);
#endif
                current->base = min(current->base, base);
                current->bound = max(current->bound, bound);

                splay(t, current);
                DEBUG(fprintf(stderr,
                              "  return splayInsert(%8lx, %8lx) -- extended\n",
                              base, bound))
                __libc_free(newNode);
                return;
            }
            case IB_REPLACE: {
#ifdef TREE_ANNOTATE_NODES
                assert(current->kind == 's');
#endif
                bool fullyContained =
                    current->base <= base && current->bound >= bound;
                if (!fullyContained) {
                    splayRemoveInterval_from_node(t, current->leftChild, base,
                                                  bound);
                    splayRemoveInterval_from_node(t, current->rightChild, base,
                                                  bound);
                }
#ifdef ENABLE_TRACER
                tracerRegisterDelete(current->base, current->bound);
#endif

                current->base = base;
                current->bound = bound;

                splay(t, current);
                DEBUG(fprintf(stderr,
                              "  return splayInsert(%8lx, %8lx) -- replaced\n",
                              base, bound))
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
#ifdef DBG
    t->numNodes++;
#endif
    DEBUG(assert(validateTree(t)))
    splay(t, newNode);
    DEBUG(fprintf(stderr, "  return splayInsert(%8lx, %8lx)\n", base, bound))
}

static void dumpAllocationMapForNode(FILE *F, const Node *N) {
    if (N->leftChild) {
        dumpAllocationMapForNode(F, N->leftChild);
    }
    fprintf(F, "  %p - %p (%luB)", (void *)N->base, (void *)N->bound,
            N->bound - N->base);
#ifdef TREE_ANNOTATE_NODES
    fprintf(F, ": %c", N->kind);
#endif
    fprintf(F, "\n");
    if (N->rightChild) {
        dumpAllocationMapForNode(F, N->rightChild);
    }
}

void __dumpAllocationMap(FILE *F, const Tree *T) {
    fprintf(F, "splay: dumped allocation map: {{{\n");
    dumpAllocationMapForNode(F, T->root);
    fprintf(F, "splay: end of dumped allocation map }}}\n");
}
