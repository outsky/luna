#include "luna.h"
#include "lvm.h"
#include "ltable.h"

static void _printins(const V_State *vs, const A_Instr *ins);
static V_Func* _get_curfunc(const V_State *vs);
static V_Func* _get_func(const V_State *vs, int idx);

V_State* V_newstate() {
    V_State *vs = NEW(V_State);
    /* TODO: any better size? */
    vs->globals = ltable_new(0);
    vs->funcs = list_new();
    return vs;
}

void V_freestate(V_State *vs) {
    ltable_free(vs->globals);
    FREE(vs);
}

void _show_status(const V_State *vs) {
    printf("version: %d.%d\n", vs->major, vs->minor);

    for (lnode *n = vs->funcs->head; n != NULL; n = n->next) {
        const V_Func *fn = CAST(const V_Func*, n->data);

        printf("%s (%d instructions, %d regs, %d consts)\n", fn->name, fn->ins.count, fn->reg.count, fn->k.count);

        printf("CONSTS:\n");
        for (int i = 0; i < fn->k.count; ++i) {
            printf("%d.\t", i);
            const Value *k = &fn->k.values[i];
            switch (k->t) {
                case VT_INT: {printf("%d\n", k->u.n);} break;
                case VT_FLOAT: {printf("%lf\n", k->u.f);} break;
                case VT_STRING: {printf("%s\n", k->u.s);} break;
                default: {error("unexpected const value type: %d", k->t);} break;
            }
        }
        printf("INSTRUCTIONS:\n");
        for (int i = 0; i < fn->ins.count; ++i) {
            const A_Instr *ins = &fn->ins.instrs[i];
            printf("%d.\t", i);
            _printins(vs, ins);
        }
        printf("\n");
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

    /* FUNCTIONS */
    int n = 0;
    fread(&n, 4, 1, f);
    for (int i = 0; i < n; ++i) {
        V_Func *fn = NEW(V_Func);

        /* NAME */
        n = 0;
        fread(&n, 1, 1, f);
        fread(fn->name, 1, n, f);

        /* REGCOUNT */
        fread(&fn->reg.count, 2, 1, f);
        fn->reg.values = NEW_ARRAY(Value, fn->reg.count);
 
        /* CONSTS */
        fread(&fn->k.count, 4, 1, f);
        if (fn->k.count > 0) {
            fn->k.values = NEW_ARRAY(Value, fn->k.count);
            for (int i = 0; i < fn->k.count; ++i) {
                Value *k = &fn->k.values[i];
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
        fread(&fn->ins.count, 4, 1, f);
        if (fn->ins.count > 0) {
            fn->ins.instrs = NEW_ARRAY(A_Instr, fn->ins.count);
            for (int i = 0; i < fn->ins.count; ++i) {
                A_Instr *ins = &fn->ins.instrs[i];
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

        if (strcmp(fn->name, "main") == 0) {
            list_pushfront(vs->funcs, fn);
        } else {
            list_pushback(vs->funcs, fn);
        }
    }

    fclose(f); f = NULL;

    _show_status(vs);
}

static void _resetstate(V_State *vs) {
    vs->ip = 0;
}

static void _pvalue(const V_State *vs, const Value *v) {
    switch (v->t) {
        case VT_INT: {printf("%d\n", v->u.n);} break;
        case VT_FLOAT: {printf("%lf\n", v->u.f);} break;
        case VT_STRING: {printf("%s\n", v->u.s);} break;
        case VT_BOOL: {printf("%s\n", v->u.n == 0 ? "false" : "true");} break;
        case VT_NIL: {printf("nil\n");} break;
        case VT_TABLE: {
            const ltable *lt = v->u.o;
            int hashcount = 0;
            for (int i = 0; i < lt->hash->size; ++i) {
                const list *l = lt->hash->slots[i];
                hashcount += l->count;
            }
            printf("table(%d, %d):%p\n", lt->arraysize, hashcount, lt);
        } break;
        case VT_CLOSURE: {
            const V_Closure *c = v->u.o;
            const V_Func *fn = _get_func(vs, c->fnidx);
            printf("closure(%d):%s\n", c->fnidx, fn->name);
        } break;
        default: {printf("?(%d)\n", v->t);} break;
    }
}

static void _pstate(const V_State *vs) {
    const V_Func *fn = _get_curfunc(vs);

    printf("{\n");
    printf("  %s(%d) IP: %d\n", fn->name, vs->curfunc, vs->ip);
    printf("  REGISTERS:\n");
    for (int i = 0; i < fn->reg.count; ++i) {
        printf("    %d.\t", i);
        const Value *v = &fn->reg.values[i];
        _pvalue(vs, v);
    }
    printf("  GLOBALS:\n");
    for (int i = 0; i < vs->globals->arraysize; ++i) {
        const Value *v = &vs->globals->array[i];
        printf("    [%d]\t", i);
        _pvalue(vs, v);
    }
    for (int i = 0; i < vs->globals->hash->size; ++i) {
        const list *l = vs->globals->hash->slots[i];
        for (lnode *n = l->head; n != NULL; n = n->next) {
            const hnode *hn = CAST(const hnode*, n->data);
            printf("    [\"%s\"]\t", hn->key);
            const Value *v = CAST(const Value*, hn->value);
            _pvalue(vs, v);
        }
    }

    printf("}\n\n");
}

#define Kst(x) (-x - 1)

static Value* RK(V_Func *fn, int x) {
    if (x < 0) {
        return &fn->k.values[Kst(x)];
    }
    return &fn->reg.values[x];
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

static void _exec_step(V_State *vs) {
    V_Func *fn = _get_curfunc(vs);
    const A_Instr *ins = &fn->ins.instrs[vs->ip];
    _printins(vs, ins);

    switch (ins->t) {
        case OP_MOVE: {
            _copy_value(&fn->reg.values[ins->a], &fn->reg.values[ins->u.bc.b]);
        } break;

        case OP_LOADK: {
            _copy_value(&fn->reg.values[ins->a], &fn->k.values[Kst(ins->u.bx)]);
        } break;

        case OP_LOADBOOL: {
            Value src;
            src.t = VT_BOOL;
            src.u.n = ins->u.bc.b != 0;
            _copy_value(&fn->reg.values[ins->a], &src);
            if (ins->u.bc.c) {
                ++vs->ip;
            }
        } break;

        case OP_LOADNIL: {
            Value src;
            src.t = VT_NIL;
            for (int i = ins->a; i <= ins->u.bc.b; ++i) {
                _copy_value(&fn->reg.values[i], &src);
            }
        } break;

        case OP_GETUPVAL: {NOT_IMP;} break;

        case OP_GETGLOBAL: {
            const Value *k = &fn->k.values[Kst(ins->u.bx)];
            if (k->t != VT_STRING) {
                error("string expected by GETGLOBAL, got %d", k->t);
            }

            const void *data = ltable_gettable(vs->globals, k->u.s);
            Value *a = &fn->reg.values[ins->a];
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
            Value *a = &fn->reg.values[ins->a];
            const Value *b = &fn->reg.values[ins->u.bc.b];
            /* TODO: check b table */
            const Value *c = RK(fn, ins->u.bc.c);
            /* TODO: check c string */
            const Value *v = ltable_gettable(b->u.o, c->u.s);
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
            const Value *k = &fn->k.values[idx];
            if (k->t != VT_STRING) {
                error("string expected by SETGLOBAL, got %d idx %d(%d)", k->t, idx, ins->u.bx);
            }
            
            const Value *a = &fn->reg.values[ins->a];
            ltable_settable(vs->globals, k->u.s, a);
        } break;

        case OP_SETUPVAL: {NOT_IMP;} break;

        case OP_SETTABLE: {
            Value *a = &fn->reg.values[ins->a];
            /* TODO: check a table */
            const Value *b = RK(fn, ins->u.bc.b);
            /* TODO: check b string */
            const Value *c = RK(fn, ins->u.bc.c);
            ltable_settable(a->u.o, b->u.s, c);
        } break;

        case OP_NEWTABLE: {
            Value v;
            v.t = VT_TABLE;
            v.u.o = ltable_new(ins->u.bc.b);    /* TODO: param `c' not used */
            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_SELF: {
            const Value *b = &fn->reg.values[ins->u.bc.b];
            /* TODO: check b table */

            _copy_value(&fn->reg.values[ins->a + 1], b);

            const Value *c = RK(fn, ins->u.bc.c);
            const Value *v = ltable_gettable(b->u.o, c->u.s);
            _copy_value(&fn->reg.values[ins->a], v);
        } break;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_POW: {
            const Value *b = RK(fn, ins->u.bc.b);
            const Value *c = RK(fn, ins->u.bc.c);
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
            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_UNM: {
            Value v;
            v.t = VT_INVALID;
            _copy_value(&v, &fn->reg.values[ins->u.bc.b]);
            if (v.t == VT_INT) {
                v.u.n = -v.u.n;
            } else if (v.t == VT_FLOAT) {
                v.u.f = -v.u.f;
            } else {
                error("value type error: %d", v.t);
            }
            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_NOT: {
            Value v;
            v.t = VT_BOOL;
            const Value *b = &fn->reg.values[ins->u.bc.b];
            switch (b->t) {
                case VT_BOOL: {v.u.n = b->u.n == 0;} break;
                case VT_NIL: {v.u.n = 1;} break;
                default: {v.u.n = 0;} break;
            }
            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_LEN: {
            const Value *b = &fn->reg.values[ins->u.bc.b];
            /* TODO: only support table? */
            int len = ltable_len(b->u.o);
            Value v;
            v.t = VT_INT;
            v.u.n = len;
            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_CONCAT: {
            int maxlen = 128; /* TODO: better value? */
            int curlen = 0;
            char *buff = NEW_ARRAY(char, maxlen);
            for (int i = ins->u.bc.b; i <= ins->u.bc.c; ++i) {
                const Value *v = &fn->reg.values[i];
                /* TODO: check string type */
                const char *s = v->u.s;
                int len = strlen(s);
                if (curlen + len >= maxlen) {
                    maxlen = 2 * (curlen + len);
                    buff = realloc(buff, maxlen);
                }
                strcpy(buff + curlen, s);
                curlen += len;
            }
            Value vnew;
            vnew.t = VT_STRING;
            vnew.u.s = buff;
            _copy_value(&fn->reg.values[ins->a], &vnew);
            FREE(buff);
        } break;

        case OP_JMP: {
            vs->ip += ins->u.bx;
        } break;

        case OP_EQ:
        case OP_LT:
        case OP_LE: {
            const Value *b = RK(fn, ins->u.bc.b);
            const Value *c = RK(fn, ins->u.bc.c);
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
                ++vs->ip;
            }
        } break;

        case OP_TEST: {
            /* if not (R(A) <=> C) then pc++ */
            /* TODO: what does the `<=>' mean? I just consider it to `=='*/
            int a = (int)_get_value_float(&fn->reg.values[ins->a]);
            if (a != ins->u.bc.c) {++vs->ip;}
        } break;

        case OP_TESTSET: {
            /* TODO: as OP_TEST, confusing `<=>' */
            int b = (int)_get_value_float(&fn->reg.values[ins->u.bc.b]);
            if (b == ins->u.bc.c) {
                _copy_value(&fn->reg.values[ins->a], &fn->reg.values[ins->u.bc.b]);
            } else {
                ++vs->ip;
            }
        } break;

        case OP_CALL: {NOT_IMP;} break;
        case OP_TAILCALL: {NOT_IMP;} break;
        case OP_RETURN: {NOT_IMP;} break;

        case OP_FORLOOP: {
            float af = _get_value_float(&fn->reg.values[ins->a]);
            float a2f = _get_value_float(&fn->reg.values[ins->a + 2]);
            Value v;
            v.t = VT_FLOAT;
            v.u.f = af + a2f;
            _copy_value(&fn->reg.values[ins->a], &v);

            float a1f = _get_value_float(&fn->reg.values[ins->a + 1]);
            if (v.u.f <= a1f) {
                vs->ip += ins->u.bx;
                _copy_value(&fn->reg.values[ins->a + 3], &v);
            }
        } break;

        case OP_FORPREP: {
            float af = _get_value_float(&fn->reg.values[ins->a]);
            float a2f = _get_value_float(&fn->reg.values[ins->a + 2]);
            Value v;
            v.t = VT_FLOAT;
            v.u.f = af - a2f;
            _copy_value(&fn->reg.values[ins->a], &v);
            vs->ip += ins->u.bx;
        } break;

        case OP_TFORLOOP: {NOT_IMP;} break;

        case OP_SETLIST: {
            /* TODO: check R[a] ltale type */
            for (int i = 1; i <= ins->u.bc.b; ++i) {
                ltable_setarray(fn->reg.values[ins->a].u.o, i - 1, &fn->reg.values[ins->a + i]);
            }
        } break;

        case OP_CLOSE: {NOT_IMP;} break;

        case OP_CLOSURE: {
            V_Closure *c = NEW(V_Closure);
            c->fnidx = ins->u.bx + 1;   /* skip `main' */

            Value v;
            v.t = VT_CLOSURE;
            v.u.o = c;

            _copy_value(&fn->reg.values[ins->a], &v);
        } break;

        case OP_VARARG: {NOT_IMP;} break;
        default: {
            error("unknown instruction type: %d", ins->t);
        } break;
    }

}

static V_Func* _get_func(const V_State *vs, int idx) {
    if (idx < 0 || idx >= vs->funcs->count) {
        return NULL;
    }
    lnode *n = vs->funcs->head;
    for (int i = 0; i < idx; ++i) {
        n = n->next;
    }
    return CAST(V_Func*, n->data);
}

static V_Func* _get_curfunc(const V_State *vs) {
    if (vs->curfunc >= vs->funcs->count) {
        error("curfunc overflow: %d of %d", vs->curfunc, vs->funcs->count);
    }

    return _get_func(vs, vs->curfunc);
}

void V_run(V_State *vs) {
    _resetstate(vs);
    _pstate(vs);

    for (;;) {
        V_Func *fn = _get_curfunc(vs);
        if (vs->curfunc == 0 && vs->ip >= fn->ins.count) {
            break;
        }
        _exec_step(vs);

        ++vs->ip;
        _pstate(vs);
    }
}

