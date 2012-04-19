/**************************************************************************************************
SimpleSATc -- Copyright (c) 2012, Matthew Markwell
   Parser code is from MiniSat-C v1.14.1.
   Used with permission, as stated below.


Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <stdio.h>
#include "solver.h"


//=================================================================================================
// Useful for Debug:

static void printlits(lit* begin, lit* end)
{
    int i;
    for (i = 0; i < end - begin; i++)
        printf(L_LIT" ",L_lit(begin[i]));
}


//=================================================================================================
// Clause struct and associated functions

struct clause_t
{
   int size;
   lit lits[0];
   int level_sat;
}

static inline int   clause_size       (clause* c)          { return c->size; }
static inline lit*  clause_begin      (clause* c)          { return c->lits; }
static inline int   clause_level      (clause* c)          { return c->level_sat; }

//=================================================================================================
// Minor (solver) functions:

void solver_setnvars(solver* s,int n)
{
    int var;

    if (s->cap < n){

        while (s->cap < n) s->cap = s->cap*2+1;

        s->decisions = (lbool*)  realloc(s->decisions,sizeof(lbool)*s->cap);
        s->assigns   = (lbool*)  realloc(s->assigns,  sizeof(lbool)*s->cap);
        s->levels    = (int*)    realloc(s->levels,   sizeof(int)*s->cap);
        s->counts    = (int*)    realloc(s->counts,   sizeof(int)*s->cap);
        s->level_choice = (bool*) realloc(s->level_choice, sizeof(bool)*s->cap);
    }

    for (var = s->size; var < n; var++){
        s->decisions    [var] = l_Undef;
        s->assigns      [var] = l_Undef;
        s->levels       [var] = -1;
        s->counts       [var] = 0;
        s->level_choice [var] = false;
    }

    s->size = n > s->size ? n : s->size;
}




