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
#include <assert.h>
#include "solver.h"


//=================================================================================================
// Useful for Debug:

#define L_IND    "%-*d"
#define L_ind    solver_dlevel(s)*3+3,solver_dlevel(s)
#define L_LIT    "%sx%d"
#define L_lit(p) lit_sign(p)?"~":"", (lit_var(p))

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
   int level_sat;
   lit lits[0];
};

static inline int   clause_size       (clause* c)          { return c->size; }
static inline lit*  clause_begin      (clause* c)          { return c->lits; }
static inline int   clause_level      (clause* c)          { return c->level_sat; }

//=================================================================================================
// Clause functions:

static clause* clause_new(solver* s, lit* begin, lit* end, int learnt)
{
    int size;
    clause* c;
    int i;

    assert(end - begin > 1);
    size           = end - begin;
    c              = (clause*)malloc(sizeof(clause) + sizeof(lit) * size);

    for (i = 0; i < size; i++)
        c->lits[i] = begin[i];

    assert(begin[0] >= 0);
    assert(begin[0] < s->size*2);
    assert(begin[1] >= 0);
    assert(begin[1] < s->size*2);

    assert(lit_neg(begin[0]) < s->size*2);
    assert(lit_neg(begin[1]) < s->size*2);

    c->size = size;
    c->level_sat = -1;  // -1 means 'clause not yet satisfied'
    return c;
}

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



//=================================================================================================
// Solver functions

solver* solver_new(void)
{
   solver* s = (solver*)malloc(sizeof(solver));

   vecp_new(&s->clauses);

   // initialize arrays
   s->decisions      = 0;  // just setting all the pointers to NULL initially
   s->level_choice   = 0;
   s->assigns        = 0;
   s->levels         = 0;
   s->counts         = 0;

   // initialize other variables
   s->size           = 0;
   s->cap            = 0;
   s->tail           = 0;
   s->cur_level      = 0;

   return s;

}

void solver_delete(solver* s)
{
    int i;
    for (i = 0; i < vecp_size(&s->clauses); i++)  // free all clauses
        free(vecp_begin(&s->clauses)[i]);


    // delete vectors
    vecp_delete(&s->clauses);

    // delete arrays
    if (s->decisions != 0){

        // if one is different from null, all are
        free(s->decisions);
        free(s->level_choice);
        free(s->assigns  );
        free(s->levels   );
        free(s->counts   );
    }

    free(s);
}


bool solver_addclause(solver* s, lit* begin, lit* end)
{
    lit *i,*j;
    int maxvar;

    if (begin == end) return false; // Empty clause

    //printlits(begin,end); printf("\n");
    // insertion sort
    maxvar = lit_var(*begin);
    for (i = begin + 1; i < end; i++){
        lit l = *i;
        maxvar = lit_var(l) > maxvar ? lit_var(l) : maxvar;
        for (j = i; j > begin && *(j-1) > l; j--)
            *j = *(j-1);
        *j = l;
    }
    solver_setnvars(s,maxvar+1);

    //printlits(begin,end); printf("\n");

    // create new clause
    vecp_push(&s->clauses,clause_new(s,begin,end,0));
    s->tail++;  // tail == # of clauses at first.

    return true;
}



