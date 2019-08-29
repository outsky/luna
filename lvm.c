#include "lib.h"
#include "lvm.h"

V_State* V_newstate() {
    V_State *vs = NEW(V_State);
    return vs;
}

void V_freestate(V_State *vs) {
    FREE(vs);
}

void _show_status(const V_State *vs) {
    printf("version: %d.%d\n", vs->major, vs->minor);
    printf("regcount: %d\n", vs->reg.count);
    printf("counts: %d\n", vs->k.count);
    for (int i = 0; i < vs->k.count; ++i) {
        printf("%d.\t", i);
        const Value *k = &vs->k.consts[i];
        switch (k->t) {
            case VT_INT: {printf("%d\n", k->u.n);} break;
            case VT_FLOAT: {printf("%lf\n", k->u.f);} break;
            case VT_STRING: {printf("%s\n", k->u.s);} break;
            default: {error("unexpected const value type: %d", k->t);} break;
        }
    }
    printf("instrutions: %d\n", vs->ins.count);
    for (int i = 0; i < vs->ins.count; ++i) {
        const A_Instr *ins = &vs->ins.instrs[i];
        printf("%d.\t", i);
        printf("%d\t%d, %d, %d\n", ins->t, ins->a, ins->b, ins->c);
    }
}

void V_load(V_State *vs, const char *binfile) {
    FILE *f = fopen(binfile, "rb");
    if (f == NULL) {
        error("Load %s failed: %s", binfile, strerror(errno));
    }

    /* HEADER */
    char ident[4];
    fread(ident, 1, 4, f);
    if (strncmp(ident, "LUNA", 4) != 0) {
        error("File format not support: `%s'", ident);
    }
    fread(&vs->major, 2, 1, f);
    fread(&vs->minor, 2, 1, f);
    fread(&vs->reg.count, 2, 1, f);
    vs->reg.regs = NEW_ARRAY(Value, vs->reg.count);

    /* CONSTS */
    fread(&vs->k.count, 4, 1, f);
    if (vs->k.count > 0) {
        vs->k.consts = NEW_ARRAY(Value, vs->k.count);
        for (int i = 0; i < vs->k.count; ++i) {
            Value *k = &vs->k.consts[i];
            fread(&k->t, 1, 1, f);
            switch (k->t) {
                case VT_INT: {fread(&k->u.n, 4, 1, f);} break;
                case VT_FLOAT: {fread(&k->u.f, 4, 1, f);} break;
                case VT_STRING: {
                    int len = 0;
                    fread(&len, 4, 1, f);
                    k->u.s = NEW_SIZE(char, len + 1);
                    fread(k->u.s, 1, len, f);
                } break;
                default: {error("unexpected const value type: %d", k->t);} break;
            }
        }
    }

    /* INSTRUCTIONS */
    fread(&vs->ins.count, 4, 1, f);
    if (vs->ins.count > 0) {
        vs->ins.instrs = NEW_ARRAY(A_Instr, vs->ins.count);
        for (int i = 0; i < vs->ins.count; ++i) {
            A_Instr *ins = &vs->ins.instrs[i];
            int n = 0;
            fread(&n, 4, 1, f);
            ins->t = n << (A_SIZE_A + A_SIZE_B + A_SIZE_C) >> (A_SIZE_A + A_SIZE_B + A_SIZE_C);
            ins->a = n << (A_SIZE_B + A_SIZE_C) >> (A_SIZE_OP + A_SIZE_B + A_SIZE_C);
            const A_OpMode *om = &A_OpModes[ins->t];
            if (om->m == iABC) {
                ins->b = n << (A_SIZE_C) >> (A_SIZE_OP + A_SIZE_A + A_SIZE_C);
                ins->c = n >> (A_SIZE_OP + A_SIZE_A + A_SIZE_B);
            } else {
                ins->b = n >> (A_SIZE_OP + A_SIZE_A);
            }
        }
    }

    fclose(f); f = NULL;

    _show_status(vs);
}

static void _resetstate(V_State *vs) {
    vs->ins.ip = 0;
}

static void _pstate(const V_State *vs) {
    printf("{\n");
    printf("  IP: %d\n", vs->ins.ip);
    printf("  REGISTERS:\n");
    for (int i = 0; i < vs->reg.count; ++i) {
        const Value *r = &vs->reg.regs[i];
        printf("    %d.\t", i);
        switch (r->t) {
            case VT_INT: {printf("%d\n", r->u.n);} break;
            case VT_FLOAT: {printf("%lf\n", r->u.f);} break;
            case VT_STRING: {printf("%s\n", r->u.s);} break;
            case VT_BOOL: {printf("%s\n", r->u.n == 0 ? "false" : "true");} break;
            case VT_NIL: {printf("nil\n");} break;
            default: {printf("?\n");} break;
        }
    }
    printf("}\n\n");
}

static void _copy_value(Value *dest, const Value *src) {
    if (dest->t == VT_STRING) {
        FREE(dest->u.s);
    }
    dest->t = src->t;
    switch (src->t) {
        case VT_INT: {dest->u.n = src->u.n;} break;
        case VT_FLOAT: {dest->u.f = src->u.f;} break;
        case VT_STRING: {dest->u.s = strdup(src->u.s);} break;
        default: {/* nothing to copy */} break;
    }
}

static void _exec_ins(V_State *vs, const A_Instr *ins) {
    printf("-> %d: <%s %d, %d, %d>\n", vs->ins.ip, A_opnames[ins->t], ins->a, ins->b, ins->c);
    switch (ins->t) {
        case OP_MOVE: {
            _copy_value(&vs->reg.regs[ins->a], &vs->reg.regs[ins->b]);
        } break;

        case OP_LOADK: {
            _copy_value(&vs->reg.regs[ins->a], &vs->k.consts[ins->b]);
        } break;

        case OP_LOADBOOL: {} break;
        case OP_LOADNIL: {} break;
        case OP_GETUPVAL: {} break;
        case OP_GETGLOBAL: {} break;
        case OP_GETTABLE: {} break;
        case OP_SETGLOBAL: {} break;
        case OP_SETUPVAL: {} break;
        case OP_SETTABLE: {} break;
        case OP_NEWTABLE: {} break;
        case OP_SELF: {} break;
        case OP_ADD: {} break;
        case OP_SUB: {} break;
        case OP_MUL: {} break;
        case OP_DIV: {} break;
        case OP_MOD: {} break;
        case OP_POW: {} break;
        case OP_UNM: {} break;
        case OP_NOT: {} break;
        case OP_LEN: {} break;
        case OP_CONCAT: {} break;
        case OP_JMP: {} break;
        case OP_EQ: {} break;
        case OP_LT: {} break;
        case OP_LE: {} break;
        case OP_TEST: {} break;
        case OP_TESTSET: {} break;
        case OP_CALL: {} break;
        case OP_TAILCALL: {} break;
        case OP_RETURN: {} break;
        case OP_FORLOOP: {} break;
        case OP_FORPREP: {} break;
        case OP_TFORLOOP: {} break;
        case OP_SETLIST: {} break;
        case OP_CLOSE: {} break;
        case OP_CLOSURE: {} break;
        case OP_VARARG: {} break;
        default: {
            error("unknown instruction type: %d", ins->t);
        } break;
    }

}

void V_run(V_State *vs) {
    _resetstate(vs);
    _pstate(vs);

    for (;;) {
        int oldip = vs->ins.ip;
        if (oldip >= vs->ins.count) {
            break;
        }
        const A_Instr *ins = &vs->ins.instrs[vs->ins.ip];
        _exec_ins(vs, ins);

        if (oldip == vs->ins.ip) {
            ++vs->ins.ip;
        }
        _pstate(vs);
    }
}

