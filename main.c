#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

void usage(const char *pname) {
    printf("%s [-op] filename\n", pname);
    printf("op:\n"
            "\tla: lexer .lasm\n"
    );
}

static const char* _load_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        perror("Open file failed");
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char *source = (char *)malloc(fsize + 1);
    memset(source, 0, fsize + 1);
    fread(source, sizeof(char), fsize, f);
    fclose(f); f = NULL;
    return source;
}

void la(const char *filename) {
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
