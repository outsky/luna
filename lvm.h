#ifndef lvm_h
#define lvm_h

#include "lasm.h"
#include "ltable.h"

typedef struct {
    int count;
    Value *values;
} V_ValueStream;

typedef struct {
    int count;
    A_Instr *instrs;
} V_InstrStream;

typedef struct {
    char name[MAX_NAME_LEN];
    int param;
    int regcount;
    V_ValueStream k;
    V_InstrStream ins;
} V_Func;

typedef struct {
    int fnidx;
} V_Closure;

typedef struct {
    int size;
    int top;
    Value *values;
} V_Stack;

typedef struct {
    int major;
    int minor;

    ltable *globals;

    list *funcs;    /* main function always at head */
    int curfunc;    /* 0: main */
    int curframe;
    int ip;

    V_Stack stk;
} V_State;

V_State* V_newstate(int stacksize);
void V_freestate(V_State *vs);

void V_load(V_State *vs, const char *binfile);

void V_run(V_State *vs);

#endif
