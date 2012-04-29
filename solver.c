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

static void printvalues(lit* begin, lit* end)
{
    int i;
    for (i = 0; i < end - begin; i++)
        printf("%d ",begin[i]);
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

        s->decisions = (bool*)  realloc(s->decisions,sizeof(bool)*s->cap);
        s->assigns   = (lbool*)  realloc(s->assigns,  sizeof(lbool)*s->cap);
        s->levels    = (int*)    realloc(s->levels,   sizeof(int)*s->cap);
        s->counts    = (int*)    realloc(s->counts,   sizeof(int)*s->cap);
        s->level_choice = (lit*) realloc(s->level_choice, sizeof(lit)*s->cap);
    }

    for (var = s->size*2; var < s->cap; var++){
        s->decisions    [var] = false;
        s->assigns      [var] = l_Undef;
        s->levels       [var] = -1;
        s->counts       [var] = 0;
        s->level_choice [var] = -1;
    }

    s->size = n > s->size ? n : s->size;
}


void printsolution(solver* s, FILE* out) {
   int i,val;
   for(i = 0; i < s->size*2; i = i+2){
      val = (s->assigns[i] == l_False)? 0 : 1;
      fprintf(out,"x%d=%d ",i>>1,val);
   }
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
   s->cur_level      = -1;
   s->satisfied      = false;

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

    //if(DEBUG){printlits(begin,end); printf("\n");}
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

    if(DEBUG){printf("Adding clause\n");printvalues(begin,end); printf("\n");}

    // create new clause
    vecp_push(&s->clauses,clause_new(s,begin,end,0));
    s->tail++;  // tail == # of clauses at first.

    return true;
}


bool update_counts(solver* s)
{
   int i,j;
   clause* c;
   // reset all counts to 0 initially
   for(i = 0; i < (s->size)*2; i++) {
      s->counts[i] = 0;
   }
   // now recount
   for(i = 0; i < s->tail;i++) {
      c = vecp_begin(&s->clauses)[i];
      for(j = 0; j < clause_size(c); j++) {
         // A true literal should not be in the working set of clauses!
         if(s->assigns[c->lits[j]] == l_True) return false;
         else
            if(s->assigns[c->lits[j]] == l_Undef) // Only count if not False
            s->counts[c->lits[j]]++;
      }
   }
   return true;
}

lit make_decision(solver* s)
{
   int i, maxval;
   lit maxlit;
   if(!update_counts(s))
      fprintf(stderr, "ERROR! Failed to update literal counts at level %d\n", s->cur_level),
      exit(1);
   maxval = -1;
   maxlit = -1;
   for(i = 0; i < s->size*2; i++){
      if (s->counts[i] > maxval){
         maxval = s->counts[i];
         maxlit = i;
      }
   }
   if (maxval == 0 || s->assigns[maxlit] == l_False)
      fprintf(stderr, "ERROR! make_decision failed to find a lit that exists and isn't false!\n"),
      exit(1);

   return maxlit;

}

// returns false if there is a conflict due to this decision
bool propogate_decision(solver* s, lit decision, bool new_level){
   bool no_conflict = true;
   int i,j,false_count;
   clause* c;


   if(new_level){
      s->cur_level++;
      s->level_choice[s->cur_level] = decision;
      s->decisions[decision] = true;// only change 'decisions' on level decisions.
   }
   s->levels[decision] = s->cur_level;
   s->assigns[decision] = l_True;
   s->assigns[lit_neg(decision)] = l_False;

   for(i = 0; i < s->tail; i++){
      c = vecp_begin(&s->clauses)[i];
      for(j = 0; j < clause_size(c); j++){
         if(j == 0) false_count = 0;
         if(s->assigns[c->lits[j]] == l_False) {
            printf("Found a false assignment, %d, now incrementing false_count from %d\n",c->lits[j],false_count);
            false_count++;
         }
         else if(s->assigns[c->lits[j]] == l_True) {
            if(DEBUG) {
               printf("Clause satisfied! clause is:\n");
               printvalues(c->lits, c->lits + c->size);
               printf("\n");
            }
            if(s->tail == 1) {
               s->satisfied = true;
               return true;
            }
            vecp_begin(&s->clauses)[i] = vecp_begin(&s->clauses)[--s->tail];
            vecp_begin(&s->clauses)[s->tail] = c;
            c->level_sat = s->cur_level;
            i = i--; // be sure to check the current i again - it isn't the same one it was!
            break;
         }
         if(false_count == clause_size(c)) {
            no_conflict = false; //Conflict found!
            if(DEBUG) printf("Found a conflict, false_count = %d\n",false_count);
         }
      }
   }

   return no_conflict;
}

// returns the level_choice of the level backtracked to
lit backtrack(solver* s){
   int i;
   lit lev_choice;
   clause* c;

   for(i = 0; i < s->size*2; i++){
      if(s->levels[i] == s->cur_level){
         s->assigns[i] = l_Undef;
         s->assigns[lit_neg(i)] = l_Undef;
      }
   }
   for(i = s->tail; i < vecp_size(&s->clauses); i++){
      c = vecp_begin(&s->clauses)[i];
      if(c->level_sat == s->cur_level){
         c->level_sat = -1;
         s->tail++;
      }
      else break;
   }

   return s->level_choice[s->cur_level--];

}

bool solver_solve(solver* s){
   lit decision;
   bool forced = false;

//   while(true) {

      // pick a variable to decide on (based on counts)
      if(!forced) {decision = make_decision(s);}
      if(DEBUG) {printf("decision made is %d. It %s a forced decision\n", decision, forced?"IS":"IS NOT");}
      else forced = false;
      if(!propogate_decision(s, decision, true)){
         // CONFLICT FOUND
         if(DEBUG)printf("Hooray!  I found a conflict due to literal %d!!\n\n", decision);
         lit lev_choice = backtrack(s);
            while(s->decisions[lit_neg(lev_choice)] == l_True && s->decisions[lev_choice] == l_True) {
               if(s->cur_level == 0) { return false;} //UNSATISFIABLE
               s->decisions[lit_neg(lev_choice)] = l_Undef;
               s->decisions[lev_choice] = l_Undef;
               lev_choice = backtrack(s);
            }
         decision = lit_neg(lev_choice);
         forced = true;
//         continue;
      }
      else {
         // NO CONFLICT, FIND UNIT CLAUSES... might wanna refactor this whole junk. Maybe just refactor backtracking.
      }
      if(s->satisfied) return true;
//    find_units();
      // make manipulate solver due to decision
      // find necessary decisions due to this decision (unit clauses)
      //    Do this again and again until there are no more unit clauses
      // If tail ever reaches 0, solved. if a clause is ever entirely false, backtrack.
      // If you backtrack, you choose the opposite value of the one you backtracked to.
      // once all unit clauses are satisfied, return to top of loop to pick next value.


//   }
}


