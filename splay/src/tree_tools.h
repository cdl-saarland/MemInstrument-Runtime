#pragma once

#include <stdio.h>

#include "tree.h"

void dumpSplayAllocMap(FILE *F, const Tree *T);

#if ASSERT_LEVEL >= AL_TREE_COMPLEX
bool validateTree(Tree *t);
#endif

#ifdef PRINT_TREE_INTERVAL
void dotTree(Tree *t);
#endif
