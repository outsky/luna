#include "luna.h"
#include "lvm.h"
#include "ltable.h"

static void _printins(const V_State *vs, const A_Instr *ins);

V_State* V_newstate() {
    V_State *vs = NEW(V_State);
    /* TODO: any better size? */
    vs->globals = htable_new(1024);
    return vs;
}

void V_freestate(V_State *vs) {
    htable_free(vs->globals);
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
        _printins(vs, ins);
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
            fread(&ins->t, 1, 1, f);
            const A_OpMode *om = &A_OpModes[ins->t];
            if (om->a != OpArgN) {
                fread(&ins->a, 1, 1, f);
            }
            switch (om->m) {
                case iABC: {
                    if (om->b != OpArgN) {
                        fread(&ins->u.bc.b, 2, 1, f);
                    }
                    if (om->c != OpArgN) {
                        fread(&ins->u.bc.c, 2, 1, f);
                    }
                } break;

                case iABx:
                case iAsBx: {
                    if (om->b != OpArgN) {
                        fread(&ins->u.bx, 4, 1, f);
                    }
                } break;
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
    printf("  GLOBALS:\n");
    for (int i = 0; i < vs->globals->size; ++i) {
        const list *l = vs->globals->slots[i];
        for (lnode *n = l->head; n != NULL; n = n->next) {
            const hnode *hn = CAST(const hnode*, n->data);
            printf("    [%s]\t", hn->key);
            const Value *r = CAST(const Value*, hn->value);
            switch (r->t) {
                case VT_INT: {printf("%d\n", r->u.n);} break;
                case VT_FLOAT: {printf("%lf\n", r->u.f);} break;
                case VT_STRING: {printf("%s\n", r->u.s);} break;
                case VT_BOOL: {printf("%s\n", r->u.n == 0 ? "false" : "true");} break;
                case VT_NIL: {printf("nil\n");} break;
                case VT_TABLE: {
                    const ltable *lt = r->u.lt;
                    int hashcount = 0;
                    for (int i = 0; i < lt->hash->size; ++i) {
                        const list *l = lt->hash->slots[i];
                        hashcount += l->count;
                    }
                    printf("table(%d, %d):%p\n", lt->arraysize, hashcount, lt);
                } break;
                default: {printf("?(%d)\n", r->t);} break;
            }
        }
    }

    printf("}\n\n");
}

#define Kst(x) (-x - 1)

static Value* RK(V_State *vs, int x) {
    if (x < 0) {
        return &vs->k.consts[Kst(x)];
    }
    return &vs->reg.regs[x];
}

static double _get_value_float(const Value *v) {
    if (v->t == VT_INT) {
        return CAST(double, v->u.n);
    } else if (v->t == VT_FLOAT) {
        return v->u.f;
    }
    error("can't cast to float: %d", v->t);
    return 0.0;
}

#define NOT_IMP error("op not imp: %s(%d)", A_opnames[ins->t], ins->t)

static void _printins(const V_State *vs, const A_Instr *ins) {
    printf("<%s", A_opnames[ins->t]);
    const A_OpMode *om = &A_OpModes[ins->t];
    if (om->a != OpArgN) {
        printf(" %d", ins->a);
    }
    switch (om->m) {
        case iABC: {
            if (om->b != OpArgN) {
                printf(" %d", ins->u.bc.b);
            }
            if (om->c != OpArgN) {
                printf(" %d", ins->u.bc.c);
            }
        } break;

        case iABx:
        case iAsBx: {
            if (om->b != OpArgN) {
                printf(" %d", ins->u.bx);
            }
        } break;
    }
    printf(">\n");
}

static void _exec_ins(V_State *vs, const A_Instr *ins) {
    _printins(vs, ins);

    switch (ins->t) {
        case OP_MOVE: {
            _copy_value(&vs->reg.regs[ins->a], &vs->reg.regs[ins->u.bc.b]);
        } break;

        case OP_LOADK: {
            _copy_value(&vs->reg.regs[ins->a], &vs->k.consts[Kst(ins->u.bx)]);
        } break;

        case OP_LOADBOOL: {
            Value src;
            src.t = VT_BOOL;
            src.u.n = ins->u.bc.b != 0;
            _copy_value(&vs->reg.regs[ins->a], &src);
            if (ins->u.bc.c) {
                ++vs->ins.ip;
            }
        } break;

        case OP_LOADNIL: {
            Value src;
            src.t = VT_NIL;
            for (int i = ins->a; i <= ins->u.bc.b; ++i) {
                _copy_value(&vs->reg.regs[i], &src);
            }
        } break;

        case OP_GETUPVAL: {NOT_IMP;} break;

        case OP_GETGLOBAL: {
            const Value *k = &vs->k.consts[Kst(ins->u.bx)];
            if (k->t != VT_STRING) {
                error("string expected by GETGLOBAL, got %d", k->t);
            }

            Value *a = &vs->reg.regs[ins->a];
            const void *data = htable_find(vs->globals, k->u.s);
            if (data == NULL) {
                Value r;
                r.t = VT_NIL;
                _copy_value(a, &r);
            } else {
                const Value *r = CAST(const Value*, data);
                _copy_value(a, r);
            }
        } break;

        case OP_GETTABLE: {
            Value *a = &vs->reg.regs[ins->a];
            const Value *b = &vs->reg.regs[ins->u.bc.b];
            /* TODO: check b table */
            const Value *c = RK(vs, ins->u.bc.c);
            /* TODO: check c string */
            const Value *v = ltable_gettable(b->u.lt, c->u.s);
            if (v == NULL) {
                Value nil;
                nil.t = VT_NIL;
                _copy_value(a, &nil);
            } else {
                _copy_value(a, v);
            }
        } break;

        case OP_SETGLOBAL: {
            int idx = Kst(ins->u.bx);
            const Value *k = &vs->k.consts[idx];
            if (k->t != VT_STRING) {
                error("string expected by SETGLOBAL, got %d idx %d(%d)", k->t, idx, ins->u.bx);
            }
            
            const Value *a = &vs->reg.regs[ins->a];
            Value *nv = NEW(Value);
            _copy_value(nv, a);
            htable_add(vs->globals, k->u.s, nv);
        } break;

        case OP_SETUPVAL: {NOT_IMP;} break;

        case OP_SETTABLE: {
            Value *a = &vs->reg.regs[ins->a];
            /* TODO: check a table */
            const Value *b = RK(vs, ins->u.bc.b);
            /* TODO: check b string */
            const Value *c = RK(vs, ins->u.bc.c);
            ltable_settable(a->u.lt, b->u.s, c);
        } break;

        case OP_NEWTABLE: {
            Value v;
            v.t = VT_TABLE;
            v.u.lt = ltable_new(ins->u.bc.b);    /* TODO: param `c' not used */
            _copy_value(&vs->reg.regs[ins->a], &v);
        } break;

        case OP_SELF: {NOT_IMP;} break;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_POW: {
            const Value *b = RK(vs, ins->u.bc.b);
            const Value *c = RK(vs, ins->u.bc.c);
            double bf = _get_value_float(b);
            double cf = _get_value_float(c);
            double ca = 0.0;
            Value v;
            v.t = VT_FLOAT;
            switch (ins->t) {
                case OP_ADD: {ca = bf + cf;} break;
                case OP_SUB: {ca = bf - cf;} break;
                case OP_MUL: {ca = bf * cf;} break;
                case OP_DIV: {ca = bf / cf;} break;
                case OP_MOD: {
                    if (b->t != VT_INT || c->t != VT_INT) {
                        error("op mod only support int, got: %d, %d", b->t, c->t);
                    }
                    v.t = VT_INT;
                    ca = (int)bf % (int)cf;
                } break;
                case OP_POW: {
                    if (b->t != VT_INT || c->t != VT_INT) {
                        error("op mod only support int, got: %d, %d", b->t, c->t);
                    }
                    v.t = VT_INT;
                    ca = (int)bf ^ (int)cf;
                } break;
                default: {
                    error("impossible: %d", ins->t);
                } break;
            }
            if (v.t == VT_INT) {
                v.u.n = (int)ca;
            } else {
                v.u.f = ca;
            }
            _copy_value(&vs->reg.regs[ins->a], &v);
        } break;

        case OP_UNM: {
            Value v;
            v.t = VT_INVALID;
            _copy_value(&v, &vs->reg.regs[ins->u.bc.b]);
            if (v.t == VT_INT) {
                v.u.n = -v.u.n;
            } else if (v.t == VT_FLOAT) {
                v.u.f = -v.u.f;
            } else {
                error("value type error: %d", v.t);
            }
            _copy_value(&vs->reg.regs[ins->a], &v);
        } break;

        case OP_NOT: {
            Value v;
            v.t = VT_BOOL;
            const Value *b = &vs->reg.regs[ins->u.bc.b];
            switch (b->t) {
                case VT_BOOL: {v.u.n = b->u.n == 0;} break;
                case VT_NIL: {v.u.n = 1;} break;
                default: {v.u.n = 0;} break;
            }
            _copy_value(&vs->reg.regs[ins->a], &v);
        } break;

        case OP_LEN: {NOT_IMP;} break;
        case OP_CONCAT: {NOT_IMP;} break;

        case OP_JMP: {
            vs->ins.ip += ins->u.bx;
        } break;

        case OP_EQ:
        case OP_LT:
        case OP_LE: {
            const Value *b = RK(vs, ins->u.bc.b);
            const Value *c = RK(vs, ins->u.bc.c);
            float bf = _get_value_float(b);
            float cf = _get_value_float(c);
            int result = 0;
            switch (ins->t) {
                case OP_EQ: {result = (bf == cf) != ins->a;} break;
                case OP_LT: {result = (bf < cf) != ins->a;} break;
                case OP_LE: {result = (bf <= cf) != ins->a;} break;
                default: {error("impossible: %d", ins->t);} break;
            }
            if (result) {
                ++vs->ins.ip;
            }
        } break;

        case OP_TEST: {
            /* if not (R(A) <=> C) then pc++ */
            /* TODO: what does the `<=>' mean? I just consider it to `=='*/
            int a = (int)_get_value_float(&vs->reg.regs[ins->a]);
            if (a != ins->u.bc.c) {++vs->ins.ip;}
        } break;

        case OP_TESTSET: {
            /* TODO: as OP_TEST, confusing `<=>' */
            int b = (int)_get_value_float(&vs->reg.regs[ins->u.bc.b]);
            if (b == ins->u.bc.c) {
                _copy_value(&vs->reg.regs[ins->a], &vs->reg.regs[ins->u.bc.b]);
            } else {
                ++vs->ins.ip;
            }
        } break;

        case OP_CALL: {NOT_IMP;} break;
        case OP_TAILCALL: {NOT_IMP;} break;
        case OP_RETURN: {NOT_IMP;} break;

        case OP_FORLOOP: {
            float af = _get_value_float(&vs->reg.regs[ins->a]);
            float a2f = _get_value_float(&vs->reg.regs[ins->a + 2]);
            Value v;
            v.t = VT_FLOAT;
            v.u.f = af + a2f;
            _copy_value(&vs->reg.regs[ins->a], &v);

            float a1f = _get_value_float(&vs->reg.regs[ins->a + 1]);
            if (v.u.f <= a1f) {
                vs->ins.ip += ins->u.bx;
                _copy_value(&vs->reg.regs[ins->a + 3], &v);
            }
        } break;

        case OP_FORPREP: {
            float af = _get_value_float(&vs->reg.regs[ins->a]);
            float a2f = _get_value_float(&vs->reg.regs[ins->a + 2]);
            Value v;
            v.t = VT_FLOAT;
            v.u.f = af - a2f;
            _copy_value(&vs->reg.regs[ins->a], &v);
            vs->ins.ip += ins->u.bx;
        } break;

        case OP_TFORLOOP: {NOT_IMP;} break;

        case OP_SETLIST: {
            /* TODO: check R[a] ltale type */
            for (int i = 1; i <= ins->u.bc.b; ++i) {
                ltable_setarray(vs->reg.regs[ins->a].u.lt, i - 1, &vs->reg.regs[ins->a + i]);
            }
        } break;

        case OP_CLOSE: {NOT_IMP;} break;
        case OP_CLOSURE: {NOT_IMP;} break;
        case OP_VARARG: {NOT_IMP;} break;
        default: {
            error("unknown instruction type: %d", ins->t);
        } break;
    }

}

void V_run(V_State *vs) {
    _resetstate(vs);
    _pstate(vs);

    for (;;) {
        if (vs->ins.ip >= vs->ins.count) {
            break;
        }
        const A_Instr *ins = &vs->ins.instrs[vs->ins.ip];
        _exec_ins(vs, ins);

        ++vs->ins.ip;
        _pstate(vs);
    }
}

