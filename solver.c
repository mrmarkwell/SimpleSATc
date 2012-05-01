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

void printsolver(solver* s)
{
   int i;
   printf("Printing solver:\n");
   printf("size: %d\tcap: %d\ttail: %d\tcur_level: %d\tsatisfied: %d\t\n",s->size,s->cap,s->tail,s->cur_level,s->satisfied);
   for(i = 0; i < s->size*2; i++){
      printf("decisions[%d] = %d\tassigns[%d] = %d  \tlevels[%d] = %d  \tcounts[%d] = %d\n",i,s->decisions[i],i,s->assigns[i],i,s->levels[i],i,s->counts[i]);
   }
   for(i = 0; i <= s->cur_level; i++){
      printf("level_choice[%d] = %d\n",i,s->level_choice[i]);
   }
   printf("\n");
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

void printclauses(solver* s)
{
   int i;
   clause* c;
   printf("Printing clauses:\n");
   for(i = 0; i < vecp_size(&s->clauses); i++){
      c = vecp_begin(&s->clauses)[i];
      printf("Clause %d:\t\t",i);
      printvalues(c->lits,c->lits + c->size);
      printf("\t\tsize: %d\tlevel_sat: %d\n",c->size,c->level_sat);
   }
}

//=================================================================================================
// Clause functions:

static clause* clause_new(solver* s, lit* begin, lit* end)
{
    int size;
    clause* c;
    int i;

    //assert(end - begin > 1);
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

    if (s->cap < n*2){

        while (s->cap < n*2) s->cap = s->cap*2+1;

        s->decisions = (bool*)   realloc(s->decisions,sizeof(bool)*s->cap);
        s->assigns   = (lbool*)  realloc(s->assigns,  sizeof(lbool)*s->cap);
        s->levels    = (int*)    realloc(s->levels,   sizeof(int)*s->cap);
        s->counts    = (int*)    realloc(s->counts,   sizeof(int)*s->cap);
        s->level_choice = (lit*) realloc(s->level_choice, sizeof(lit)*s->cap);
    }

    for (var = 0; var < s->cap; var++){
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

    // insertion sort
    maxvar = lit_var(*begin);
    for (i = begin + 1; i < end; i++){
        lit l = *i;
        maxvar = lit_var(l) > maxvar ? lit_var(l) : maxvar;
        for (j = i; j > begin && *(j-1) > l; j--)
            *j = *(j-1);
        *j = l;
    }
    if(DEBUG && s->size == 0) printf("Adding clauses:\n");
    solver_setnvars(s,maxvar+1);
    if(DEBUG){printvalues(begin,end); printf("\n");}

    // create new clause
    vecp_push(&s->clauses,clause_new(s,begin,end));
    s->tail++;  // tail == # of clauses at first.

    return true;
}


bool update_counts(solver* s)
{
   int i,j;
   clause* c;
   // reset all counts to 0 initially
   for(i = 0; i < s->size*2; i++) {
      s->counts[i] = 0;
   }
   // now recount
   for(i = 0; i < s->tail;i++) {
      c = vecp_begin(&s->clauses)[i];
      for(j = 0; j < clause_size(c); j++) {
         // A true literal should not be in the working set of clauses!
         if(s->assigns[c->lits[j]] == l_True) return false;
         else if(s->assigns[c->lits[j]] == l_Undef) // Only count if not False
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
bool propagate_decision(solver* s, lit decision, bool new_level){
   bool no_conflict = true;
   int i,j,false_count;
   clause* c;


   if(new_level){
      if(DEBUG) printf("It is a level decision\n");
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
            false_count++;
         }
         else if(s->assigns[c->lits[j]] == l_True) {
            if(DEBUG) {
               printf("Clause satisfied! clause is:\n");
               printvalues(c->lits, c->lits + c->size);
               printf("\n");
            }
            c->level_sat = s->cur_level;
            if(s->tail == 1) {
               s->tail--;
               s->satisfied = true;
               return true;
            }
            vecp_begin(&s->clauses)[i] = vecp_begin(&s->clauses)[--s->tail];
            vecp_begin(&s->clauses)[s->tail] = c;
            i = i--; // be sure to check the current i again - it isn't the same one it was!
            break;
         }
         if(false_count == clause_size(c)) {
            no_conflict = false; //Conflict found!
            if(DEBUG) {
               printf("Found a CONFLICT, false_count = %d and clause is:\n",false_count);
               printvalues(c->lits, c->lits + c->size);
               printf("\n");
            }
         }
      }
   }
   return no_conflict;
}

// returns the level_choice of the level backtracked to
lit backtrack_once(solver* s){
   int i;
   clause* c;

   for(i = 0; i < s->size*2; i++){
      if(s->levels[i] == s->cur_level){
         s->assigns[i] = l_Undef;
         s->assigns[lit_neg(i)] = l_Undef;
         s->levels[i] = -1;
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

// returns true if backtrack worked, false if top of tree is hit (UNSATISFIABLE)
bool backtrack(solver* s, lit* decision) {
// CONFLICT FOUND
   if(s->cur_level == 0 && s->decisions[lit_neg(s->level_choice[0])] == true) return false; //UNSATISFIABLE (boundary case)
   lit lev_choice = backtrack_once(s);
      while(s->decisions[lit_neg(lev_choice)] == true && s->decisions[lev_choice] == true) {
         if(s->cur_level+1 == 0) { return false;} //UNSATISFIABLE
         s->decisions[lit_neg(lev_choice)] = false;
         s->decisions[lev_choice] = false;
         lev_choice = backtrack_once(s);
      }
   *decision = lit_neg(lev_choice);
   if(DEBUG) printf("Backtracked to level %d where level choice was %d\n",s->cur_level+1,lev_choice);
   assert(s->decisions[lev_choice] == true);
   assert(s->decisions[lit_neg(lev_choice)] == false);
   return true;
}

// finds a unit clause if there is one.  Returns true if so, where unit_lit is the literal in that clause.
bool find_unit(solver* s, lit* unit_lit){
   int i,j, false_count;
   clause* c;

   for(i = 0; i < s->tail; i++){
      c = vecp_begin(&s->clauses)[i];
      for(j = 0; j < clause_size(c); j++){
         if(j == 0) false_count = 0;
         assert(s->assigns[c->lits[j]] != l_True);
         if(s->assigns[c->lits[j]] == l_False) false_count++;
         else *unit_lit = c->lits[j]; // If this is a unit clause, this will be the unit lit.
         if(j == clause_size(c) - 1 && false_count == clause_size(c) - 1) {
            if(DEBUG) {
               printf("In find_unit, found a unit clause! Literal is %d, and clause is:\n",*unit_lit);
               printvalues(c->lits,c->lits + c->size);
               printf("\n");
            }
            return true; //UNIT CLAUSE!
         }
      }
   }
   return false; // NO UNIT CLAUSES
}
// returns false if conflict is found.  True if not, or solved.
bool propagate_units(solver* s){
   lit unit_lit;
   if(DEBUG) printf("In propagate_units, trying to find a unit clause. Tail is %d, level is %d\n",s->tail,s->cur_level);
   while(find_unit(s, &unit_lit)){
      if(!propagate_decision(s,unit_lit,false)) return false; // CONFLICT
      if(s->tail == 0) return true; // SATISFIED
      if(DEBUG) printf("In propagate_units, no conflict after propagating unit clause decision! Checking again... Tail is %d, level is %d\n", s->tail, s->cur_level);
   }
   if(DEBUG) printf("In propagate_units, no more unit clauses found! Tail is %d, level is %d\n",s->tail,s->cur_level);
   return true;
}


bool solver_solve(solver* s){
   lit decision;
   bool forced = false;
   int timer = 0;

   while(true) {
      if(DEBUG) printf("Making a decision... it is%sforced\n",forced?" ":" NOT ");
      if(DEBUGLITE){
         timer++;
         if(timer == 100000) {
            printsolver(s);
            timer = 0;
         }
      }
      // pick a variable to decide on (based on counts)
      if(!forced) {decision = make_decision(s);}
      else forced = false;
      if(DEBUG) printf("Decision is %d at level %d\n",decision,s->cur_level+1);
      if(!propagate_decision(s, decision, true)){
         // CONFLICT
         if(DEBUG) printf("In solver_solve, after level decision, found conflict. Backtracking.\nTail is %d and level is %d\n",s->tail,s->cur_level);
         if(!backtrack(s,&decision)) return false;//UNSATISFIABLE
         else{ //Backtrack worked, decision must be forced
            if(DEBUG) printf("backtrack worked, tail is now %d and level is %d\n",s->tail,s->cur_level);
            forced = true;
            continue;
         }
      }
      else {
         // NO CONFLICT
         if(DEBUG) printf("In solver_solve, after level decision, NO CONFLICT. Tail is %d and level is %d\n",s->tail,s->cur_level);
         if(s->satisfied) return true;
         if(DEBUG) printf("Entering propagate_units.\n");
         if(!propagate_units(s)){
            // CONFLICT
            if(DEBUG) printf("In solver_solve, after propagate_units, found conflict. Backtracking.\nTail is %d and level is %d\n",s->tail,s->cur_level);
            if(!backtrack(s, &decision)) return false; //UNSATISFIABLE
            else {
               if(DEBUG) printf("backtrack due to propagate_units worked, tail is now %d and level is %d\n",s->tail,s->cur_level);
               forced = true;
               continue;
            }
         }
         else {
            // NO CONFLICT
            if(s->satisfied) return true;
         }
      }
      if(DEBUG) printsolver(s);
   }
   return true;
}


