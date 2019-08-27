#ifndef lib_h
#define lib_h

#define STR2(x) #x
#define STR(x) STR2(x)
#define error(...) errorf(__FILE__ ":" STR(__LINE__), __VA_ARGS__)

void errorf(const char *where, const char *fmt, ...);
void snapshot(const char* code, int pos, int line);

#endif
