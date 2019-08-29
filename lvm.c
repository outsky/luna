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
    printf("regcount: %d\n", vs->regcount);
    printf("counts: %d\n", vs->k.count);
    for (int i = 0; i < vs->k.count; ++i) {
        printf("%d.\t", i);
        const A_Const *k = &vs->k.consts[i];
        if (k->t == A_CT_INT) {
            printf("%d\n", k->u.n);
        } else if (k->t == A_CT_FLOAT) {
            printf("%lf\n", k->u.f);
        } else {
            printf("%s\n", k->u.s);
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
    fread(&vs->regcount, 2, 1, f);

    /* CONSTS */
    fread(&vs->k.count, 4, 1, f);
    if (vs->k.count > 0) {
        vs->k.consts = NEW_ARRAY(A_Const, vs->k.count);
        for (int i = 0; i < vs->k.count; ++i) {
            A_Const *k = &vs->k.consts[i];
            fread(&k->t, 1, 1, f);
            if (k->t == A_CT_INT) {
                fread(&k->u.n, 4, 1, f);
            } else if (k->t == A_CT_FLOAT) {
                fread(&k->u.f, 4, 1, f);
            } else {
                int len = 0;
                fread(&len, 4, 1, f);
                k->u.s = NEW_SIZE(char, len + 1);
                fread(k->u.s, 1, len, f);
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

void V_run(V_State *vs) {
}
