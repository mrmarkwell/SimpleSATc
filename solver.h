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

#ifndef solver_h
#define solver_h

#include "vec.h"
#include <stdio.h>
//================================================================================================
// Simple types:

typedef int bool;
static const bool true     = 1;
static const bool false    = 0;

typedef int lit;
typedef char  lbool;
// might add some specific consts for ints or lits here to help with// the arrays in the solver (decision, levels, counts)

static const lbool l_Undef   =  0;
static const lbool l_True    =  1;
static const lbool l_False   = -1;


static inline lit  toLit   (int v) { return v + v; } // changes value to literal int (other than sign bit)
static inline lit  lit_neg (lit l) { return l ^ 1; } // Changes the sign of the lit
static inline int  lit_var (lit l) { return l >> 1; } // Returns the value of the lit (ignoring sign)
static inline int  lit_sign(lit l) { return (l & 1); } // Returns 1 if sign is neg and 0 if it is pos.


//================================================================================================
// Public interface:

struct solver_t;
typedef struct solver_t solver;

extern void printsolution(solver* s, FILE* out);
extern solver* solver_new(void);
extern void    solver_delete(solver* s);
extern bool    update_counts(solver* s);
extern lit     make_decision(solver* s);
extern bool    propogate_decision(solver* s, lit decision, bool new_level);
extern lit     backtrack(solver* s);

extern bool    solver_addclause(solver* s, lit* begin, lit* end);

extern void    solver_setnvars(solver* s,int n);

//================================================================================================
// Solver Representation:


struct clause_t;
typedef struct clause_t clause;

struct solver_t
{
   int size;            // number of variables
   int cap;             // size of varmaps
   int tail;            // tail of clause vecp
   int cur_level;       // current level in decision tree
   bool satisfied;      // false if function is not satisfied
   vecp  clauses;       // vector of pointers to all clauses
   bool*  decisions;    // array of decisions to variables (use this to determine which directions
                        // down the tree you've gone. !!only make l_True!! l_Undef indicates this
                        // decision has not yet been made

   lit*   level_choice; // only one variable assignment is selected per level.
                        // The other level decisions are required by the unit clause rule.
                        // This array keeps track of which choice was made at each level

   lbool*  assigns;     // this is the current list of assignments (if a solution is found, this
                        //array has the solution in it)

   int*  levels;        // level that each lit was assigned
   int*  counts;        // number of occurrences of each literal

};


#endif /* solver.h */
