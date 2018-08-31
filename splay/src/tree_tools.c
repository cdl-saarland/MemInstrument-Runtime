#include "tree_tools.h"

#include <string.h>

#include "config.h"
#include "macros.h"
#include "statistics.h"

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

void dumpSplayAllocMap(FILE *F, const Tree *T) {
    fprintf(F, "dumped allocation map: {{{\n");
    dumpAllocationMapForNode(F, T->root);
    fprintf(F, "end of dumped allocation map }}}\n");
}

#if ASSERT_LEVEL >= AL_TREE_COMPLEX
// Functions for expensive debug checks

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

bool validateTree(Tree *t) {
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

void dotTree(Tree *t) {
    static unsigned num = 0; // TODO atomic
    size_t pn_sz = strlen(__get_prog_name());
    size_t sz = pn_sz + strlen("splay..dot") + 1 + 10;
    char buf[sz];
    sprintf(buf, "splay.%s.%u.dot", __get_prog_name(), ++num);
    FILE *F = fopen(buf, "w");

    fprintf(F, "digraph splaytree_%s_%u\n{\n", __get_prog_name(), num);
    fprintf(F, "  label=\"splaytree %s %u\";\n", __get_prog_name(), num);
    fprintf(F, "  labelloc=top;\n");
    fprintf(F, "  labeljust=left;\n\n");
    dotNodeLabel(F, t->root);
    fprintf(F, "\n");
    dotFromNode(F, t->root);
    fprintf(F, "}\n");

    fclose(F);
}

#endif
