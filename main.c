#include "lib.h"
#include "lasm.h"
#include "lvm.h"

static void usage(const char *pname) {
    printf("%s [-op] filename\n", pname);
    printf("op:\n"
            "\tla: lexer .lasm\n"
            "\tas: assemble .lasm to .lbin\n"
            "\tvm: run .lbin\n"
    );
}

static void lexer_asm(const char *filename) {
    A_State *as = A_newstate(filename);

    for (;;) {
        if (A_nexttok(as) == A_TT_EOT) {
            printf("<EOT>\n");
            break;
        }
        A_ptok(&as->curtok);
    }

    A_freestate(as); as = NULL;
}

static void assemble_asm(const char *filename) {
    A_State *as = A_newstate(filename);
    A_parse(as);
    A_createbin(as, "a.lbin");
    A_freestate(as); as = NULL;
}

static void vm_bin(const char *filename) {
    V_State *vs = V_newstate(filename);
    V_load(vs, filename);
    V_run(vs);
    V_freestate(vs); vs = NULL;
}

int main(int argc, const char **argv) {
    const char* pname = argv[0];
    if (argc != 3) {
        usage(pname);
        exit(-1);
    }

    const char *opt = argv[1];
    const char *filename = argv[2];
    if (strcmp(opt, "-la") == 0) {
        lexer_asm(filename);
    } else if (strcmp(opt, "-as") == 0) {
        assemble_asm(filename);
    } else if (strcmp(opt, "-vm") == 0) {
        vm_bin(filename);
    } else {
        usage(pname);
        exit(-1);
    }

    return 0;
}
