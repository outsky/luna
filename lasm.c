#include <ctype.h>
#include "luna.h"
#include "lasm.h"

#define A_FATAL(...) snapshot(as->src, as->curidx, as->curline); error(__VA_ARGS__)

#define expect(tt) do {\
    if (A_nexttok(as) != tt) {\
        A_FATAL("`%s' expected, got `%s'", _toknames[tt], _toknames[as->curtok.t]);\
    }\
} while (0)


A_OpMode A_OpModes[] = {
/*     A        B       C     mode		   opcode	*/
    {OpArgR, OpArgR, OpArgN, iABC},     /* OP_MOVE */
    {OpArgR, OpArgK, OpArgN, iABx},		/* OP_LOADK */
    {OpArgR, OpArgU, OpArgU, iABC}, 	/* OP_LOADBOOL */
    {OpArgR, OpArgR, OpArgN, iABC}, 	/* OP_LOADNIL */
    {OpArgR, OpArgU, OpArgN, iABC}, 	/* OP_GETUPVAL */
    {OpArgR, OpArgK, OpArgN, iABx}, 	/* OP_GETGLOBAL */
    {OpArgR, OpArgR, OpArgK, iABC}, 	/* OP_GETTABLE */
    {OpArgR, OpArgK, OpArgN, iABx}, 	/* OP_SETGLOBAL */
    {OpArgR, OpArgU, OpArgN, iABC}, 	/* OP_SETUPVAL */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_SETTABLE */
    {OpArgR, OpArgU, OpArgU, iABC}, 	/* OP_NEWTABLE */
    {OpArgR, OpArgR, OpArgK, iABC}, 	/* OP_SELF */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_ADD */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_SUB */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_MUL */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_DIV */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_MOD */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_POW */
    {OpArgR, OpArgR, OpArgN, iABC}, 	/* OP_UNM */
    {OpArgR, OpArgR, OpArgN, iABC}, 	/* OP_NOT */
    {OpArgR, OpArgR, OpArgN, iABC}, 	/* OP_LEN */
    {OpArgR, OpArgR, OpArgR, iABC}, 	/* OP_CONCAT */
    {OpArgN, OpArgR, OpArgN, iAsBx},	/* OP_JMP */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_EQ */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_LT */
    {OpArgR, OpArgK, OpArgK, iABC}, 	/* OP_LE */
    {OpArgR, OpArgR, OpArgU, iABC}, 	/* OP_TEST */
    {OpArgR, OpArgR, OpArgU, iABC}, 	/* OP_TESTSET */
    {OpArgR, OpArgU, OpArgU, iABC}, 	/* OP_CALL */
    {OpArgR, OpArgU, OpArgU, iABC}, 	/* OP_TAILCALL */
    {OpArgR, OpArgU, OpArgN, iABC}, 	/* OP_RETURN */
    {OpArgR, OpArgR, OpArgN, iAsBx},	/* OP_FORLOOP */
    {OpArgR, OpArgR, OpArgN, iAsBx},	/* OP_FORPREP */
    {OpArgR, OpArgN, OpArgU, iABC}, 	/* OP_TFORLOOP */
    {OpArgR, OpArgU, OpArgU, iABC}, 	/* OP_SETLIST */
    {OpArgR, OpArgN, OpArgN, iABC}, 	/* OP_CLOSE */
    {OpArgR, OpArgU, OpArgN, iABx}, 	/* OP_CLOSURE */
    {OpArgR, OpArgU, OpArgN, iABC}		/* OP_VARARG */
};

const char *const A_opnames[] = {
  "MOVE",
  "LOADK",
  "LOADBOOL",
  "LOADNIL",
  "GETUPVAL",
  "GETGLOBAL",
  "GETTABLE",
  "SETGLOBAL",
  "SETUPVAL",
  "SETTABLE",
  "NEWTABLE",
  "SELF",
  "ADD",
  "SUB",
  "MUL",
  "DIV",
  "MOD",
  "POW",
  "UNM",
  "NOT",
  "LEN",
  "CONCAT",
  "JMP",
  "EQ",
  "LT",
  "LE",
  "TEST",
  "TESTSET",
  "CALL",
  "TAILCALL",
  "RETURN",
  "FORLOOP",
  "FORPREP",
  "TFORLOOP",
  "SETLIST",
  "CLOSE",
  "CLOSURE",
  "VARARG",
  NULL
};

enum A_LexState {
    A_LS_INIT,
    A_LS_UNARY,     // + | -
    A_LS_INT,       // 123
    A_LS_INTDOT,    // 123.
    A_LS_FLOAT,     // 123.123
    A_LS_STR_HALF,  // "abc
    A_LS_STR_ESC,   // "abc\?
    A_LS_COMMENT,   // ; comment
    A_LS_IDENT,
};

static void _add_func(A_State *as, const char *name) {
    A_Func *f = NEW(A_Func);
    strncpy(f->name, name, MAX_NAME_LEN);
    f->regcount = 2;    /* default */
    f->consts = list_new();
    f->instrs = list_new();

    list_pushback(as->funcs, f);
    as->curfunc = as->funcs->count;
}

A_State* A_newstate(const char *srcfile) {
    A_State *as = NEW(A_State);
    as->srcfile = srcfile;
    as->src = load_file(srcfile);

    as->funcs = list_new();

    return as;
}

void A_freestate(A_State *as) {
    FREE(as->src);
    FREE(as);
}

static void _freetok(A_Token *t) {
    if (t->t == A_TT_STRING) {
        free(t->u.s); t->u.s = NULL;
    }
    t->t = A_TT_INVALID;
}

static int _getopcode(const char *opname) {
    for (int i = 0; A_opnames[i] != NULL; ++i) {
        if (strcmp(opname, A_opnames[i]) == 0) {
            return i;
        }
    }
    return -1;
}

A_TokenType A_nexttok(A_State *as) {
    int maxidx = strlen(as->src);
    if (as->curidx >= maxidx) {
        return A_TT_EOT;
    }
    int ls = A_LS_INIT;
    int begin = 0;
    _freetok(&as->curtok);
    for (;;) {
        char c = as->src[as->curidx++];
        switch (ls) {
            case A_LS_INIT: {
                if (c == '\0') {
                    return A_TT_EOT;
                }

                if (c == '\r' || isblank(c))  {
                    break;
                }

                if (isdigit(c)) {
                    begin = as->curidx - 1;
                    ls = A_LS_INT;
                    break;
                }

                if (c == '_' || isalpha(c)) {
                    begin = as->curidx - 1;
                    ls = A_LS_IDENT;
                    break;
                }

                if (c == '"') {
                    begin = as->curidx;
                    ls = A_LS_STR_HALF;
                    break;
                }

                if (c == ';') {
                    ls = A_LS_COMMENT;
                    break;
                }

                if (c == '+' || c == '-') {
                    begin = as->curidx - 1;
                    ls = A_LS_UNARY;
                    break;
                }

                if (c == ',') {as->curtok.t = A_TT_COMMA; return as->curtok.t;}
                if (c == '\n') {as->curtok.t = A_TT_NEWLINE; ++as->curline; return as->curtok.t;}
                if (c == '{') {as->curtok.t = A_TT_OPEN_BRACE; return as->curtok.t;}
                if (c == '}') {as->curtok.t = A_TT_CLOSE_BRACE; return as->curtok.t;}

                A_FATAL("invalid char `%c'", c);
            } break;

            case A_LS_UNARY: {
                if (!isdigit(c)) {
                    A_FATAL("number expected, got `%c'", c);
                }
                ls = A_LS_INT;
            } break;

            case A_LS_INT: {
                if (isdigit(c)) {
                    break;
                }

                if (c == '.') {
                    ls = A_LS_INTDOT;
                    break;
                }

                if (c == ']' || c == ',' || c == ';' || isspace(c)) {
                    --as->curidx;
                    as->curtok.t = A_TT_INT;
                    char *tmp = strndup(as->src + begin, as->curidx - begin);
                    as->curtok.u.n = atoi(tmp);
                    free(tmp);
                    return A_TT_INT;
                }

                A_FATAL("unexpect char `%c'", c);
            } break;

            case A_LS_INTDOT: {
                if (isdigit(c)) {
                    ls = A_LS_FLOAT;
                    break;
                }
                A_FATAL("unexpect char `%c'", c);
            } break;

            case A_LS_FLOAT: {
                if (isdigit(c)) {
                    break;
                }
                
                if (isspace(c)) {
                    --as->curidx;
                    as->curtok.t = A_TT_FLOAT;
                    char *tmp = strndup(as->src + begin, as->curidx - begin);
                    as->curtok.u.f = atof(tmp);
                    free(tmp);
                    return A_TT_FLOAT;
                }

                A_FATAL("unexpect char `%c'", c);
            } break;

            case A_LS_STR_HALF: {
                if (c == '\0') {
                    A_FATAL("unfinished string");
                }

                if (c == '\\') {
                    ls = A_LS_STR_ESC;
                    break;
                }
                if (c == '"') {
                    as->curtok.t = A_TT_STRING;
                    as->curtok.u.s = strndup(as->src + begin, as->curidx - begin - 1);
                    return A_TT_STRING;
                }
            } break;

            case A_LS_STR_ESC: {
                ls = A_LS_STR_HALF;
            } break;

            case A_LS_IDENT: {
                if (c != '_' && !isalnum(c)) {
                    --as->curidx;

                    _freetok(&as->curtok);

                    char *tmp = strndup(as->src + begin, as->curidx - begin);
                    if (strcmp(tmp, "K") == 0) {FREE(tmp); as->curtok.t = A_TT_CONST; return as->curtok.t;}
                    if (strcmp(tmp, "R") == 0) {FREE(tmp); as->curtok.t = A_TT_REGCOUNT; return as->curtok.t;}
                    if (strcmp(tmp, "PARAM") == 0) {FREE(tmp); as->curtok.t = A_TT_PARAM; return as->curtok.t;}
                    if (strcmp(tmp, "FUNC") == 0) {FREE(tmp); as->curtok.t = A_TT_FUNC; return as->curtok.t;}

                    int oc = _getopcode(tmp);
                    if (oc >= 0) {
                        FREE(tmp);
                        as->curtok.t = A_TT_INSTR;
                        as->curtok.u.n = oc;
                        return as->curtok.t;
                    }

                    as->curtok.t = A_TT_IDENT;
                    as->curtok.u.s = tmp;
                    return as->curtok.t;
                }
            } break;

            case A_LS_COMMENT: {
                if (c == '\n') {
                    --as->curidx;
                    ls = A_LS_INIT;
                    break;
                }
            } break;

            default: {
                A_FATAL("invalid state");
            } break;
        }
    }
}

void A_ptok(const A_Token *tok) {
    switch (tok->t) {
        case A_TT_INT: {
            printf("<D:%d> ", tok->u.n);
        } break;
        case A_TT_FLOAT: {
            printf("<F:%lf> ", tok->u.f);
        } break;
        case A_TT_STRING: {
            printf("<S:%s> ", tok->u.s);
        } break;
        case A_TT_COMMA: {
            printf("<,> ");
        } break;
        case A_TT_NEWLINE: {
            printf("<NL>\n");
        } break;
        case A_TT_CONST: {
            printf("<K> ");
        } break;
        case A_TT_REGCOUNT: {
            printf("<R> ");
        } break;
        case A_TT_INSTR: {
            printf("<I:%s> ", A_opnames[tok->u.n]);
        } break;
        case A_TT_EOT: {
            printf("<EOT> ");
        } break;
        default: {
            error("unexpected token type `%d'", tok->t);
        } break;
    }
}

static void _resetstate(A_State *as) {
    as->curline = 1;
    as->curidx = 0;
    as->curfunc = 0;
    _freetok(&as->curtok);
}

static const char *_toknames[] = {
    "INVALID",
    "INT",
    "FLOAT",
    "STRING",
    "COMMA",     /* , */
    "{",
    "}",
    "NEWLINE",
    "CONST",
    "REGCOUNT",
    "FUNC",
    "INSTR",     /* instruction */
    "IDENT",
    "EOT",
};

static A_Func* _get_curfunc(const A_State *as) {
    if (as->curfunc == 0) {
        A_FATAL("not in function scope");
    }
    if (as->curfunc > as->funcs->count) {
        A_FATAL("function idx overflow: %d of %d", as->curfunc, as->funcs->count);
    }
    lnode *n = as->funcs->head;
    for (int i = 0; i < as->curfunc - 1; ++i) {
        n = n->next;
    }
    return CAST(A_Func*, n->data);
}

static void _parse_const(A_State *as) {
    A_TokenType kt = A_nexttok(as);
    Value *k = NEW(Value);
    if (kt == A_TT_INT) {
        k->t = VT_INT;
        k->u.n = as->curtok.u.n;
    } else if (kt == A_TT_FLOAT) {
        k->t = VT_FLOAT;
        k->u.f = as->curtok.u.f;
    } else if (kt == A_TT_STRING) {
        k->t = VT_STRING;
        k->u.s = strdup(as->curtok.u.s);
    } else {
        FREE(k);
        A_FATAL("const can only be int, float and string");
    }

    A_Func *fn = _get_curfunc(as);
    list_pushback(fn->consts, k);

    expect(A_TT_NEWLINE);
}

static void _parse_func(A_State *as) {
    if (as->curfunc != 0) {
        A_FATAL("nested function is not allowed");
    }
    expect(A_TT_IDENT);
    _add_func(as, as->curtok.u.s);
    expect(A_TT_OPEN_BRACE);
}

static void _parse_instr(A_State *as) {
    A_OpCode oc = as->curtok.u.n;
    const A_OpMode *om = &A_OpModes[oc];

    int a, b, c;
    a = b = c = 0;
    if (om->a != OpArgN) {
        expect(A_TT_INT);
        a = as->curtok.u.n;
        if (om->b != OpArgN || om->c != OpArgN) {
            expect(A_TT_COMMA);
        }
    }

    if (om->b != OpArgN) {
        expect(A_TT_INT);
        b = as->curtok.u.n;
        if (om->c != OpArgN) {
            expect(A_TT_COMMA);
        }
    }

    if (om->c != OpArgN) {
        expect(A_TT_INT);
        c = as->curtok.u.n;
    }

    expect(A_TT_NEWLINE);

    A_Instr *ins = NEW(A_Instr);
    ins->t = oc;
    ins->a = CAST(char, a);
    switch (om->m) {
        case iABC: {
            ins->u.bc.b = CAST(short, b);
            ins->u.bc.c = CAST(short, c);
        } break;

        case iABx:
        case iAsBx: {
            ins->u.bx = b;
        } break;
    }

    A_Func *fn = _get_curfunc(as);
    list_pushback(fn->instrs, ins);
}

static void _parse_param(A_State *as) {
    expect(A_TT_INT);

    A_Func *fn = _get_curfunc(as);
    fn->param = as->curtok.u.n;

    expect(A_TT_NEWLINE);
}

static void _parse_regcount(A_State *as) {
    expect(A_TT_INT);

    A_Func *fn = _get_curfunc(as);
    fn->regcount = as->curtok.u.n;

    expect(A_TT_NEWLINE);
}

void A_parse(A_State *as) {
    _resetstate(as);

    for (;;) {
        A_TokenType tt = A_nexttok(as);
        switch (tt) {
            case A_TT_FUNC: {_parse_func(as);} break;
            case A_TT_CLOSE_BRACE: {
                if (as->curfunc == 0) {
                    A_FATAL("unexpected `}'");
                }
                as->curfunc = 0;
            } break;
            case A_TT_CONST: {_parse_const(as);} break;
            case A_TT_INSTR: {_parse_instr(as);} break;
            case A_TT_PARAM: {_parse_param(as);} break;
            case A_TT_REGCOUNT: {_parse_regcount(as);} break;
            case A_TT_NEWLINE: {} break;
            case A_TT_EOT: {return;}
            default: {A_FATAL("unexpected token");} break;
        }
    }
}

/*==================================================
HEADER:
    "LUNA" (4 bytes)
    VER_MAJOR (2 bytes)
    VER_MINOR (2 bytes)

FUNCTIONS:
    count (4 bytes)
    {
        NAME:
            len (1 bytes)
            data (name len bytes)

        PARAM (2 bytes)
        REGCOUNT (2 bytes)

        CONSTS:
            count (4 bytes)
            {
                type (1 byte)
                [size (4 bytes) only for string]
                data (4 bytes for number)
            }

        INSTRUCTIONS:
            count (4 bytes)
            {
                opcode (1 byte)
                args (a: 1 byte; b: 2 bytes; c: 2 bytes; bx 4 bytes)
            }
    }
==================================================*/
void A_createbin(const A_State *as, const char *outfile) { 
    FILE *f = fopen(outfile, "wb");
    if (f == NULL) {
        perror("");
        error("Open %s failed: %s", outfile, strerror(errno));
    }

    int num = 0;

    /* HEADER */
    fwrite("LUNA", 1, 4, f);
    num = A_VER_MAJOR;
    fwrite(&num, 2, 1, f);
    num = A_VER_MINOR;
    fwrite(&num, 2, 1, f);

    /* FUNCTIONS */
    fwrite(&as->funcs->count, 4, 1, f);
    for (const lnode *n = as->funcs->head; n != NULL; n = n->next) {
        const A_Func *fn = CAST(const A_Func*, n->data);

        /* NAME */
        int namelen = strlen(fn->name);
        if (namelen > MAX_NAME_LEN) {
            namelen = MAX_NAME_LEN;
        }
        fwrite(&namelen, 1, 1, f);
        fwrite(fn->name, 1, namelen, f);

        /* PARAM */
        fwrite(&fn->param, 2, 1, f);

        /* REGCOUNT */
        fwrite(&fn->regcount, 2, 1, f);

        /* CONSTS */
        fwrite(&fn->consts->count, 4, 1, f);
        for (lnode *n = fn->consts->head; n != NULL; n = n->next) {
            Value *k = CAST(Value*, n->data);
            fwrite(&k->t, 1, 1, f);
            if (k->t == VT_INT) {
                fwrite(&k->u.n, 4, 1, f);
            } else if (k->t == VT_FLOAT) {
                fwrite(&k->u.f, 4, 1, f);
            } else if (k->t == VT_STRING) {
                int len = strlen(k->u.s);
                fwrite(&len, 4, 1, f);
                fwrite(k->u.s, 1, len, f);
            } else {
                error("unexpected const type: %d", k->t);
            }
        }

        /* INSTRUCTIONS */
        fwrite(&fn->instrs->count, 4, 1, f);
        for (lnode *n = fn->instrs->head; n != NULL; n = n->next) {
            A_Instr *instr = CAST(A_Instr*, n->data);
            const A_OpMode *om = &A_OpModes[instr->t];
            fwrite(&instr->t, 1, 1, f);
            if (om->a != OpArgN) {
                fwrite(&instr->a, 1, 1, f);
            }
            switch (om->m) {
                case iABC: {
                    if (om->b != OpArgN) {
                        fwrite(&instr->u.bc.b, 2, 1, f);
                    }
                    if (om->c != OpArgN) {
                        fwrite(&instr->u.bc.c, 2, 1, f);
                    }
                } break;

                case iABx:
                case iAsBx: {
                    if (om->b != OpArgN) {
                        fwrite(&instr->u.bx, 4, 1, f);
                    }
                } break;
            }
        }
    }

    fclose(f); f = NULL;
}

