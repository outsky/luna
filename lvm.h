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
    V_ValueStream subf;
} V_Func;

typedef struct {
    int fnidx;
    V_ValueStream uv;
} V_Closure;

typedef struct {
    int size;
    int top;
    Value *values;
} V_Stack;

typedef struct {
    int func;
    int ip;
    int retb;   /* return to reg begin */
    int rete;   /*               end */
    int base; /* stack slot of this func */
} V_CallInfo;

typedef struct {
    int size;
    int count;
    V_CallInfo **values;
} V_CallInfoStream;

typedef struct {
    int major;
    int minor;

    ltable *globals;

    list *funcs;    /* main function always at head */

    V_Closure *cl;
    V_Stack stk;
    V_CallInfoStream cis;
    V_CallInfo *curci;
} V_State;

V_State* V_newstate(int stacksize);
void V_freestate(V_State *vs);

void V_load(V_State *vs, const char *binfile);

void V_run(V_State *vs);

#endif
