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
    V_ValueStream reg;
    V_ValueStream k;
    V_InstrStream ins;
} V_Func;

typedef struct {
    int fnidx;
} V_Closure;

typedef struct {
    int major;
    int minor;

    ltable *globals;

    list *funcs;    /* main function always at head */
    int curfunc;    /* 0: main */
    int ip;
} V_State;

V_State* V_newstate();
void V_freestate(V_State *vs);

void V_load(V_State *vs, const char *binfile);

void V_run(V_State *vs);

#endif
