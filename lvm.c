#include "lib.h"
#include "lvm.h"

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
            ins->t = GET_OP(n);
            ins->a = GET_A(n);
            const A_OpMode *om = &A_OpModes[ins->t];
            if (om->m == iABC) {
                ins->b = GET_B(n);
                ins->c = GET_C(n);
            } else {
                ins->b = GET_Bx(n);
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
                default: {printf("?\n");} break;
            }
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

static Value* RK(V_State *vs, int x) {
    if (ISK(x)) {
        return &vs->k.consts[INDEXK(x)];
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

static void _exec_ins(V_State *vs, const A_Instr *ins) {
    printf("-> %d: <%s %d, %d, %d>\n", vs->ins.ip, A_opnames[ins->t], ins->a, ins->b, ins->c);
    switch (ins->t) {
        case OP_MOVE: {
            _copy_value(&vs->reg.regs[ins->a], &vs->reg.regs[ins->b]);
        } break;

        case OP_LOADK: {
            _copy_value(&vs->reg.regs[ins->a], &vs->k.consts[Kst(ins->b)]);
        } break;

        case OP_LOADBOOL: {
            /*	A B C	R(A) := (Bool)B; if (C) pc++			*/
            /* TODO: what does this `if (C) pc++' mean?
            The pc will increase automatically however. I wonder */
            Value src;
            src.t = VT_BOOL;
            src.u.n = ins->b != 0;
            _copy_value(&vs->reg.regs[ins->a], &src);
            if (ins->c) {
                ++vs->ins.ip;
            }
        } break;

        case OP_LOADNIL: {
            Value src;
            src.t = VT_NIL;
            for (int i = ins->a; i <= ins->b; ++i) {
                _copy_value(&vs->reg.regs[i], &src);
            }
        } break;

        case OP_GETUPVAL: {} break;

        case OP_GETGLOBAL: {
            const Value *k = &vs->k.consts[Kst(ins->b)];
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

        case OP_GETTABLE: {} break;

        case OP_SETGLOBAL: {
            int idx = Kst(ins->b);
            const Value *k = &vs->k.consts[idx];
            if (k->t != VT_STRING) {
                error("string expected by SETGLOBAL, got %d idx %d(%d)", k->t, idx, ins->b);
            }
            
            const Value *a = &vs->reg.regs[ins->a];
            Value *nv = NEW(Value);
            _copy_value(nv, a);
            htable_add(vs->globals, k->u.s, nv);
        } break;

        case OP_SETUPVAL: {} break;
        case OP_SETTABLE: {} break;
        case OP_NEWTABLE: {} break;
        case OP_SELF: {} break;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_POW: {
            const Value *b = RK(vs, ins->b);
            const Value *c = RK(vs, ins->c);
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
            _copy_value(&v, &vs->reg.regs[ins->b]);
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
            const Value *b = &vs->reg.regs[ins->b];
            switch (b->t) {
                case VT_BOOL: {v.u.n = b->u.n == 0;} break;
                case VT_NIL: {v.u.n = 1;} break;
                default: {v.u.n = 0;} break;
            }
            _copy_value(&vs->reg.regs[ins->a], &v);
        } break;

        case OP_LEN: {} break;
        case OP_CONCAT: {} break;

        case OP_JMP: {
            vs->ins.ip += ins->b;
        } break;

        case OP_EQ:
        case OP_LT:
        case OP_LE: {
            /* TODO: again, confusing */
            const Value *b = RK(vs, ins->b);
            const Value *c = RK(vs, ins->c);
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
            if (a != ins->c) {++vs->ins.ip;}
        } break;

        case OP_TESTSET: {
            /* TODO: as OP_TEST, confusing `<=>' */
            int b = (int)_get_value_float(&vs->reg.regs[ins->b]);
            if (b == ins->c) {
                _copy_value(&vs->reg.regs[ins->a], &vs->reg.regs[ins->b]);
            } else {
                ++vs->ins.ip;
            }
        } break;

        case OP_CALL: {} break;
        case OP_TAILCALL: {} break;
        case OP_RETURN: {} break;

        case OP_FORLOOP: {
            float af = _get_value_float(&vs->reg.regs[ins->a]);
            float a2f = _get_value_float(&vs->reg.regs[ins->a + 2]);
            Value v;
            v.t = VT_FLOAT;
            v.u.f = af + a2f;
            _copy_value(&vs->reg.regs[ins->a], &v);

            float a1f = _get_value_float(&vs->reg.regs[ins->a + 1]);
            if (v.u.f <= a1f) {
                vs->ins.ip += ins->b;
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
            vs->ins.ip += ins->b;
        } break;

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
        if (vs->ins.ip >= vs->ins.count) {
            break;
        }
        const A_Instr *ins = &vs->ins.instrs[vs->ins.ip];
        _exec_ins(vs, ins);

        ++vs->ins.ip;
        _pstate(vs);
    }
}

