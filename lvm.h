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
    int ip;
    A_Instr *instrs;
} V_InstrStream;

typedef struct {
    int count;
    Value *regs;
} V_RegStream;

typedef struct {
    int major;
    int minor;

    ltable *globals;

    V_RegStream reg;
    V_ValueStream k;
    V_InstrStream ins;
} V_State;

V_State* V_newstate();
void V_freestate(V_State *vs);

void V_load(V_State *vs, const char *binfile);

void V_run(V_State *vs);

#endif
