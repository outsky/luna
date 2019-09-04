#include "luna.h"

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

char* load_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        perror("Open file failed");
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char *fdata = NEW_SIZE(char, fsize + 1);
    fread(fdata, sizeof(char), fsize, f);
    fclose(f); f = NULL;
    return fdata;
}

void _copy_value(Value *dest, const Value *src) {
    if (dest->t == VT_STRING) {
        FREE(dest->u.s);
    }
    if (src == NULL) {
        dest->t = VT_NIL;
        return;
    }
    dest->t = src->t;
    switch (src->t) {
        case VT_INT: {dest->u.n = src->u.n;} break;
        case VT_FLOAT: {dest->u.f = src->u.f;} break;
        case VT_STRING: {dest->u.s = strdup(src->u.s);} break;

        case VT_TABLE:
        case VT_CLOSURE: {dest->u.o = src->u.o;} break;

        default: {/* nothing to copy */} break;
    }
}

