#include "lib.h"
#include "lasm.h"

static void usage(const char *pname) {
    printf("%s [-op] filename\n", pname);
    printf("op:\n"
            "\tla: lexer .lasm\n"
            "\tas: assemble .lasm to .lbin\n"
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
    } else {
        usage(pname);
        exit(-1);
    }

    return 0;
}
