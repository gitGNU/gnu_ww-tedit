/*
  calcpriv.h:
    Defines the internal interfaces of the calculator.
*/

///////////////////////////////////////////////////////
//
// Useful general purpose types definitions and macros
//

typedef unsigned long ULONG;
typedef char BOOLEAN;

#define TRUE 1
#define FALSE 0

#define _countof( x )   (sizeof(x) / sizeof((x)[0]))
#define ASSERT( x )     assert( x )


///////////////////////////////////////////////////////
//
// Internal types and global symbols
//

typedef struct
{
  BOOLEAN isFloat;       
  union
  {
    struct
    {
      ULONG   v;
      BOOLEAN isSigned;
    } i;
    double f;
  } value;
} Val;


// from LEXYY.C
//

// ===================== name mappings for the lexer
#ifndef yylex
  #define yylex fcalc_lex
#endif
#ifndef yytext
  #define yytext fcalc_text
#endif
#ifndef yyleng
  #define yyleng fcalc_leng
#endif

extern char * yytext;
extern int    yyleng;

extern int yylex ( void );

extern void * fcalc_ScanString ( const char * );
extern void fcalc_EndString ( void * );

// From CALC.Y
//
#ifndef yylval
  #define yylval fcalc_lval
#endif

void fcalc_Convert ( Val * op, int totype );
void fcalc_Error ( const char * err, ... );

// From CALCOPS.C
// 
int  fcalc_ClassifyIdent ( const char * szIdent, int len );

// From CALCFUNC.C
//
int  fcalc_FindFunc ( const char * szName );
void fcalc_CallFunc ( Val * res, int funcIndex, Val * args, int argCount );


