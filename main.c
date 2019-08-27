#include "lib.h"
#include "lasm.h"

void usage(const char *pname) {
    printf("%s [-op] filename\n", pname);
    printf("op:\n"
            "\tla: lexer .lasm\n"
    );
}

void la(const char *filename) {
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

int main(int argc, const char **argv) {
    const char* pname = argv[0];
    if (argc != 3) {
        usage(pname);
        exit(-1);
    }

    const char *opt = argv[1];
    const char *filename = argv[2];
    if (strcmp(opt, "-la") == 0) {
        la(filename);
    } else {
        usage(pname);
        exit(-1);
    }

    return 0;
}
