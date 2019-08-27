#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "lib.h"

static void print_error(const char *where, const char *label, const char *fmt, va_list args) {
    fprintf(stderr, "[%s] ", label);
    fprintf(stderr, "%s: ", where);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void errorf(const char *where, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_error(where, "x", fmt, args);
    va_end(args);
    exit(-1);
}

void snapshot(const char* code, int pos, int line) {
    int begin, end;
    for (begin = pos - 1; begin > 0; --begin) {
        if (code[begin] == '\n') {
            ++begin;
            break;
        }
    }
    for (end = pos; ; ++end) {
        char c = code[end];
        if (c == '\0' || c == '\n') {
            --end;
            break;
        }
    }
    char *snap = strndup(code + begin, end - begin + 1);
    printf("line: %d\n%s\n", line, snap);
    free(snap);
    for (int i = begin; i < end + 1; ++i) {
        if (i == pos - 1) {
            printf("^");
        } else {
            printf("%c", isblank(code[i]) ? code[i] : ' ');
        }
    }
    printf("\n'''\n");
}
