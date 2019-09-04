#ifndef lasm_h
#define lasm_h

#include "list.h"
#include "ltable.h"

#define A_VER_MAJOR 5
#define A_VER_MINOR 1

typedef enum {iABC, iABx, iAsBx} OpMode;   /* basic instruction format */

typedef enum {
  OpArgN,  /* argument is not used */
  OpArgU,  /* argument is used */
  OpArgR,  /* argument is a register or a jump offset */
  OpArgK   /* argument is a constant or register/constant */
} OpArgMask;

typedef struct {
    OpArgMask a;
    OpArgMask b;
    OpArgMask c;
    OpMode m;
} A_OpMode;

extern A_OpMode A_OpModes[];
extern const char *const A_opnames[];

/*
** R(x) - register
** Kst(x) - constant (in constant table)
** RK(x) == if ISK(x) then Kst(INDEXK(x)) else R(x)
*/

typedef enum {
/*----------------------------------------------------------------------
name		args	description
------------------------------------------------------------------------*/
OP_MOVE,/*	A B	R(A) := R(B)					*/
OP_LOADK,/*	A Bx	R(A) := Kst(Bx)					*/
OP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/
OP_LOADNIL,/*	A B	R(A) := ... := R(B) := nil			*/
OP_GETUPVAL,/*	A B	R(A) := UpValue[B]				*/

OP_GETGLOBAL,/*	A Bx	R(A) := Gbl[Kst(Bx)]				*/
OP_GETTABLE,/*	A B C	R(A) := R(B)[RK(C)]				*/

OP_SETGLOBAL,/*	A Bx	Gbl[Kst(Bx)] := R(A)				*/
OP_SETUPVAL,/*	A B	UpValue[B] := R(A)				*/
OP_SETTABLE,/*	A B C	R(A)[RK(B)] := RK(C)				*/

OP_NEWTABLE,/*	A B C	R(A) := {} (size = B,C)				*/

OP_SELF,/*	A B C	R(A+1) := R(B); R(A) := R(B)[RK(C)]		*/

OP_ADD,/*	A B C	R(A) := RK(B) + RK(C)				*/
OP_SUB,/*	A B C	R(A) := RK(B) - RK(C)				*/
OP_MUL,/*	A B C	R(A) := RK(B) * RK(C)				*/
OP_DIV,/*	A B C	R(A) := RK(B) / RK(C)				*/
OP_MOD,/*	A B C	R(A) := RK(B) % RK(C)				*/
OP_POW,/*	A B C	R(A) := RK(B) ^ RK(C)				*/
OP_UNM,/*	A B	R(A) := -R(B)					*/
OP_NOT,/*	A B	R(A) := not R(B)				*/
OP_LEN,/*	A B	R(A) := length of R(B)				*/

OP_CONCAT,/*	A B C	R(A) := R(B).. ... ..R(C)			*/

OP_JMP,/*	sBx	pc+=sBx					*/

OP_EQ,/*	A B C	if ((RK(B) == RK(C)) ~= A) then pc++		*/
OP_LT,/*	A B C	if ((RK(B) <  RK(C)) ~= A) then pc++  		*/
OP_LE,/*	A B C	if ((RK(B) <= RK(C)) ~= A) then pc++  		*/

OP_TEST,/*	A C	if not (R(A) <=> C) then pc++			*/ 
OP_TESTSET,/*	A B C	if (R(B) <=> C) then R(A) := R(B) else pc++	*/ 

OP_CALL,/*	A B C	R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) */
OP_TAILCALL,/*	A B C	return R(A)(R(A+1), ... ,R(A+B-1))		*/
OP_RETURN,/*	A B	return R(A), ... ,R(A+B-2)	(see note)	*/

OP_FORLOOP,/*	A sBx	R(A)+=R(A+2);
			if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }*/
OP_FORPREP,/*	A sBx	R(A)-=R(A+2); pc+=sBx				*/

OP_TFORLOOP,/*	A C	R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); 
                        if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++	*/ 
OP_SETLIST,/*	A B C	R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B	*/

OP_CLOSE,/*	A 	close all variables in the stack up to (>=) R(A)*/
OP_CLOSURE,/*	A Bx	R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))	*/

OP_VARARG/*	A B	R(A), R(A+1), ..., R(A+B-1) = vararg		*/
} A_OpCode;

typedef enum {
    A_TT_INVALID,
    A_TT_INT,
    A_TT_FLOAT,
    A_TT_STRING,
    A_TT_COMMA,       /* , */
    A_TT_OPEN_BRACE,  /* { */
    A_TT_CLOSE_BRACE, /* } */
    A_TT_NEWLINE,
    A_TT_CONST,
    A_TT_PARAM,     /* params count */
    A_TT_REGCOUNT,  /* regs count */
    A_TT_FUNC,
    A_TT_INSTR,     /* instruction */
    A_TT_IDENT,
    A_TT_EOT,
} A_TokenType;

typedef struct {
    A_TokenType t;
    union {
        int n;
        double f;
        char *s;
    } u;
} A_Token;

typedef struct {
    A_OpCode t;
    short a;
    union {
        struct {short b; short c;} bc;
        int bx;
    } u;
} A_Instr;

typedef struct {
    char name[MAX_NAME_LEN];
    int param;
    int regcount;
    list *consts;
    list *instrs;
} A_Func;

typedef struct {
    const char *srcfile;
    char *src;
    int curline;
    int curidx;

    list *funcs;
    int curfunc;    /* 0: in global scope */

    A_Token curtok;
} A_State;

A_State* A_newstate(const char *srcfile);
void A_freestate(A_State *as);

A_TokenType A_nexttok(A_State *as);

void A_parse(A_State *as);
void A_createbin(const A_State *as, const char *outfile);

void A_ptok(const A_Token *tok);


#endif
