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

#include "vec.h"
#include "solver.h"

#include <stdio.h>
#include <stdlib.h>

//=================================================================================================
// DIMACS Parser from MiniSat-C v1.14.1:

char* readFile(FILE *  in)
{
    char*   data = malloc(65536);
    int     cap  = 65536;
    int     size = 0;

    while (!feof(in)){
        if (size == cap){
            cap *= 2;
            data = realloc(data, cap); }
        size += fread(&data[size], 1, 65536, in);
    }
    data = realloc(data, size+1);
    data[size] = '\0';

    return data;
}


static inline void skipWhitespace(char** in) {
    while ((**in >= 9 && **in <= 13) || **in == 32)
        (*in)++; }

static inline void skipLine(char** in) {
    for (;;){
        if (**in == 0) return;
        if (**in == '\n') { (*in)++; return; }
        (*in)++; } }

static inline int parseInt(char** in) {
    int     val = 0;
    int    _neg = 0;
    skipWhitespace(in);
    if      (**in == '-') _neg = 1, (*in)++;
    else if (**in == '+') (*in)++;
    if (**in < '0' || **in > '9') fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", **in), exit(1);
    while (**in >= '0' && **in <= '9')
        val = val*10 + (**in - '0'),
        (*in)++;
    return _neg ? -val : val; }

static void readClause(char** in, veci* lits) {
    int parsed_lit, var;
    veci_resize(lits,0);
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        veci_push(lits, (parsed_lit > 0 ? toLit(var) : lit_neg(toLit(var))));
    }
}

static lbool parse_DIMACS_main(char* in, solver* s) {
    veci lits;
    veci_new(&lits);

    for (;;){
        skipWhitespace(&in);
        if (*in == 0)
            break;
        else if (*in == 'c' || *in == 'p')
            skipLine(&in);
        else{
            lit* begin;
            readClause(&in, &lits);
            begin = veci_begin(&lits);
            if (!solver_addclause(s, begin, begin+veci_size(&lits))){
                veci_delete(&lits);
                return l_False;
            }
        }
    }
    veci_delete(&lits);
    return l_True;
}


// Inserts problem into solver. Returns FALSE upon immediate conflict.
//
static lbool parse_DIMACS(FILE * in, solver* s) {
    char* text = readFile(in);
    lbool ret  = parse_DIMACS_main(text, s);
    free(text);
    return ret; }


//=================================================================================================


int main(int argc, char** argv)
{
   solver* s = solver_new();
   lbool st;
   FILE* in;
   FILE* out;

   if (argc != 2)
     fprintf(stderr, "ERROR! Not enough command line arguments.\n"),
     exit(1);
   out = fopen("SimpleSATc.out","a");
   in = fopen(argv[1], "rb");
   if (in == NULL)
     fprintf(stderr, "ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]),
     exit(1);
   st = parse_DIMACS(in, s);
   fclose(in);

   if (st == l_False){
     solver_delete(s);
     printf("Trivial problem\nUNSATISFIABLE\n");
     exit(20);
   }

   if(DEBUG) {printf("Solver size: %d Tail: %d\nBeginning solver_solve\n",s->size,s->tail);}
   st = solver_solve(s);

   fprintf(out,"################################# SimpleSATc #################################\n");
   fprintf(out,"Input file: %s\n",argv[1]);
   if(s->satisfied) {
      fprintf(out,"Result: SATISFIABLE\n");
      fprintf(out,"Satisfying Solution:\n");
      printsolution(s,out);
      fprintf(out, "\n\n\n\n\n\n");
   }
   else {
      fprintf(out,"Result: UNSATISFIABLE\n\n\n\n\n\n");
   }

   //if(DEBUG) {
   //   int i;
   //   printf("Values in 'counts':\n");
   //   for (i = 0; i < (s->size)*2; i++) {
   //     printf("counts[%d] = %d\n", i, s->counts[i]);
   //   }
   //}
   if(DEBUG) {
      printf("Final status of solver:\n");
      printsolver(s);
      printclauses(s);
   }
   fclose(out);
   solver_delete(s);
   return 0;
}
