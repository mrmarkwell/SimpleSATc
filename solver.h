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


static inline lit  toLit   (int v) { return v + v; }
static inline lit  lit_neg (lit l) { return l ^ 1; }
static inline int  lit_var (lit l) { return l >> 1; }
static inline int  lit_sign(lit l) { return (l & 1); }


//================================================================================================
// Public interface:
//
// put the public stuff from solver.c in here.
//

//================================================================================================
// Solver Representation:


struct clause_t;
typedef struct clause_t clause;




//Put the solver struct here














#endif /* solver.h */
