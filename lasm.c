#include <ctype.h>
#include "lib.h"
#include "lasm.h"

#define A_FATAL(...) snapshot(as->src, as->curidx, as->curline); error(__VA_ARGS__)

static const char *const _opnames[] = {
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

A_State* A_newstate(const char *srcfile) {
    A_State *as = NEW(A_State*);
    as->srcfile = srcfile;
    as->src = load_file(srcfile);

    //as->consts = list_new();
    as->instrs = list_new();
    //printf("--consts--%p %d\n", as->consts, as->consts->count);
    printf("--instrs--%p %d\n", as->instrs, as->instrs->count);

    return as;
}

void A_freestate(A_State *as) {
    FREE(as->src);
    FREE(as);
}

static void _freetok(A_Token *t) {
    if (t->t == A_TT_STRING || t->t == A_TT_INSTR) {
        free(t->u.s); t->u.s = NULL;
    }
    t->t = A_TT_INVALID;
}

static int _getopcode(const char *opname) {
    for (int i = 0; _opnames[i] != NULL; ++i) {
        if (strcmp(opname, _opnames[i]) == 0) {
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

                if (c == ',') {as->curtok.t = A_TT_COMMA; return A_TT_COMMA;}
                if (c == '\n') {as->curtok.t = A_TT_NEWLINE; ++as->curline; return A_TT_NEWLINE;}

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

                if (c == ']' || c == ',' || isspace(c)) {
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
                    if (strcmp(tmp, "K") == 0) {free(tmp); as->curtok.t = A_TT_CONST; return as->curtok.t;}

                    if (_getopcode(tmp) >= 0) {
                        as->curtok.t = A_TT_INSTR;
                        as->curtok.u.s = tmp;
                        return as->curtok.t;
                    }
                    A_FATAL("unexpected ident `%s'", tmp);
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
        case A_TT_INSTR: {
            printf("<I:%s> ", tok->u.s);
        } break;
        case A_TT_EOT: {
            printf("<EOT> ");
        } break;
        default: {
            error("unexpected token type `%d'", tok->t);
        } break;
    }
}

void A_parse(A_State *as) {
}

static A_OpMode _opmodes[] = {
/*      B       C     mode		   opcode	*/
    {OpArgR, OpArgN, iABC},     /* OP_MOVE */
    {OpArgK, OpArgN, iABx},		/* OP_LOADK */
    {OpArgU, OpArgU, iABC}, 	/* OP_LOADBOOL */
    {OpArgR, OpArgN, iABC}, 	/* OP_LOADNIL */
    {OpArgU, OpArgN, iABC}, 	/* OP_GETUPVAL */
    {OpArgK, OpArgN, iABx}, 	/* OP_GETGLOBAL */
    {OpArgR, OpArgK, iABC}, 	/* OP_GETTABLE */
    {OpArgK, OpArgN, iABx}, 	/* OP_SETGLOBAL */
    {OpArgU, OpArgN, iABC}, 	/* OP_SETUPVAL */
    {OpArgK, OpArgK, iABC}, 	/* OP_SETTABLE */
    {OpArgU, OpArgU, iABC}, 	/* OP_NEWTABLE */
    {OpArgR, OpArgK, iABC}, 	/* OP_SELF */
    {OpArgK, OpArgK, iABC}, 	/* OP_ADD */
    {OpArgK, OpArgK, iABC}, 	/* OP_SUB */
    {OpArgK, OpArgK, iABC}, 	/* OP_MUL */
    {OpArgK, OpArgK, iABC}, 	/* OP_DIV */
    {OpArgK, OpArgK, iABC}, 	/* OP_MOD */
    {OpArgK, OpArgK, iABC}, 	/* OP_POW */
    {OpArgR, OpArgN, iABC}, 	/* OP_UNM */
    {OpArgR, OpArgN, iABC}, 	/* OP_NOT */
    {OpArgR, OpArgN, iABC}, 	/* OP_LEN */
    {OpArgR, OpArgR, iABC}, 	/* OP_CONCAT */
    {OpArgR, OpArgN, iAsBx},	/* OP_JMP */
    {OpArgK, OpArgK, iABC}, 	/* OP_EQ */
    {OpArgK, OpArgK, iABC}, 	/* OP_LT */
    {OpArgK, OpArgK, iABC}, 	/* OP_LE */
    {OpArgR, OpArgU, iABC}, 	/* OP_TEST */
    {OpArgR, OpArgU, iABC}, 	/* OP_TESTSET */
    {OpArgU, OpArgU, iABC}, 	/* OP_CALL */
    {OpArgU, OpArgU, iABC}, 	/* OP_TAILCALL */
    {OpArgU, OpArgN, iABC}, 	/* OP_RETURN */
    {OpArgR, OpArgN, iAsBx},	/* OP_FORLOOP */
    {OpArgR, OpArgN, iAsBx},	/* OP_FORPREP */
    {OpArgN, OpArgU, iABC}, 	/* OP_TFORLOOP */
    {OpArgU, OpArgU, iABC}, 	/* OP_SETLIST */
    {OpArgN, OpArgN, iABC}, 	/* OP_CLOSE */
    {OpArgU, OpArgN, iABx}, 	/* OP_CLOSURE */
    {OpArgU, OpArgN, iABC}		/* OP_VARARG */
};

static int _instr_num(const A_Instr *instr) {
    const A_OpMode *om = &_opmodes[instr->t];
    switch (om->m) {
        case iABC: {
            return (instr->t << A_POS_OP) + (instr->A << A_POS_A) + (instr->B << A_POS_B) + (instr->C << A_POS_C);
        }

        case iABx: 
        case iAsBx: {
            return (instr->t << A_POS_OP) + (instr->A << A_POS_A) + (instr->B << A_POS_BX);
        }
    }
    return 0;
}

/*==================================================
HEADER:
    "LUNA" (4 bytes)
    VER_MAJOR (2 bytes)
    VER_MINOR (2 bytes)

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
        data (4 bytes)
    }
==================================================*/
void A_createbin(const A_State *as, const char *outfile) { 
    //printf("--consts1--%p %d\n", as->consts, as->consts->count);
    printf("--instrs1--%p %d\n", as->instrs, as->instrs->count);

    FILE *f = fopen(outfile, "wb");
    //printf("--consts2--%p %d\n", as->consts, as->consts->count);
    printf("--instrs2--%p \n", as->instrs);

    if (f == NULL) {
        perror("");
        error("Open %s failed: %s", outfile, strerror(errno));
    }
    printf("--consts1--%p %d\n", as->consts, as->consts->count);
    printf("--instrs1--%p %d\n", as->instrs, as->instrs->count);

    int num = 0;

    /* HEADER */
    fwrite("LUNA", 1, 4, f);
    num = A_VER_MAJOR;
    fwrite(&num, 2, 1, f);
    num = A_VER_MINOR;
    fwrite(&num, 2, 1, f);

    /* CONSTS */
    fwrite(&as->consts->count, 4, 1, f);
    for (lnode *n = as->consts->head; n != NULL; n = n->next) {
        A_Const *k = CAST(A_Const*, n->data);
        fwrite(&k->t, 1, 1, f);
        if (k->t == A_CT_INT) {
            fwrite(&k->u.n, 4, 1, f);
        } else if (k->t == A_CT_FLOAT) {
            fwrite(&k->u.f, 4, 1, f);
        } else {
            int len = strlen(k->u.s);
            fwrite(&len, 4, 1, f);
            fwrite(k->u.s, 1, len, f);
        }
    }

    /* INSTRUCTIONS */
    printf("--consts--%p %d\n", as->consts, as->consts->count);
    printf("--instrs--%p\n", as->instrs);


    printf("=-==%d\n", as->instrs->count);
    fwrite(&as->instrs->count, 4, 1, f);
    for (lnode *n = as->instrs->head; n != NULL; n = n->next) {
        A_Instr *instr = CAST(A_Instr*, n->data);
        int in = _instr_num(instr);
        fwrite(&in, 4, 1, f);
    }

    fclose(f); f = NULL;
}

