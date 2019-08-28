#ifndef lvm_h
#define lvm_h

#include "lasm.h"

typedef struct {
    int count;
    A_Const *consts;
} V_ConstStream;

typedef struct {
    int count;
    int ip;
    A_Instr *instrs;
} V_InstrStream;

typedef struct {
    int major;
    int minor;

    int regcount;
    V_ConstStream k;
    V_InstrStream ins;
} V_State;

V_State* V_newstate();
void V_freestate(V_State *vs);

void V_load(V_State *vs, const char *binfile);

void V_run(V_State *vs);

#endif
