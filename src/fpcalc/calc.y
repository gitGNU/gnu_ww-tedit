%{
#include <float.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#include "calcpriv.h"   // private interfaces
#include "fpconf.h"     // floating point configuration

//////////////////////////////////////////////////////
// Local data
//

#define MAX_ARGS  32        // max number of function arguments
														// (counting nested function calls)

static Val Args[MAX_ARGS];  // function argument stack
static int NumArgs;         // top of stack

static char * output;       // pointer to where the string result must be
														// stored.

static char szRadixFmt[16]; // a sprintf() format string used to format
														// floating point values

#define RADIX_CHAR       ('c' << 8)
#define RADIX_FP_FORMAT  ('f' << 8)

static jmp_buf ErrJmp;      // a fatal error (the message has been printed)

static void DivisionByZero ( void )
{
  fcalc_Error( "Integer division by zero" );
};

// If DIV0_SIGNAL is supported we don't need to check explicitly for
// integer divisions by zero. The CHECK_DIV0() macro encapsulates
// this.
//
#if DIV0_SIGNAL
  #define CHECK_DIV0( op )
#else
  #define CHECK_DIV0( op )   ((op) ? (void)0 : DivisionByZero())
#endif

//--------------------------------------------------------------------------
// Name         SigFpe
//
// Handle floating point errors (SIG_FPE signal) by calling fcalc_Error.
//--------------------------------------------------------------------------
#ifdef __TURBOC__
// Under Borland's compilers we receive additional information about the
// type of FP exception that occured. We are interested only in determining
// whether it was actually an integer division by zero.
//
static void SigFpe ( int sig, int type )
{
  if (type == FPE_INTDIV0 || type == FPE_INTOVFLOW)
    DivisionByZero();

#else
// For the rest of the compilers we can use the standard function
// declaration
//
static void SigFpe ( int sig )
{
#endif

  // Microsoft requires this after an FP exception. I know that
  // under Visual C the FP exceptions are supposed to be disabled,
  // but better safe than sorry...
  //
  CLEAR_FP_STATE();

  fcalc_Error( "Floating point error" );
};

//--------------------------------------------------------------------------
// Name         matherr
//
// Catch and ignore the floating point errors that occur in
// math functions. (For exanple whan calling sqrt(-1.0)
//--------------------------------------------------------------------------
MATHERR_SIG
{
  return 1;	// Ignore the error
};

//--------------------------------------------------------------------------
// Name         ConvertToFloat
//
// Converts a value to floating point.
//--------------------------------------------------------------------------
static void ConvertToFloat ( Val * op )
{
  double f;

  if (op->isFloat)
    return;

  op->value.f =
    op->value.i.isSigned ? (double)(long)op->value.i.v : (double)(ULONG)op->value.i.v;
  op->isFloat = TRUE;
};

//--------------------------------------------------------------------------
// Name         ArithmeticConversion
//
// Performs a subset of C-s standard arithmetic conversions. The purpose
// is to have both operands of the same type.
//
// In our simplified case (we support only double, unsigned_long and long)
// the rules are (in this order):
//   1. If one operand is double, convert the other one to double
//   2. Both operands are long. If one is unsigned_long convert the other
//      to unsigned_long as well.
//
// To recapitulate: the operand that has the "weaker" type is converted to the
// "stronger" type. The types go from "strongest" to "weakest" as follows:
//    double => unsigned long => signed long
//--------------------------------------------------------------------------
static void ArithmeticConversion ( Val * op1, Val * op2 )
{
  if (op1->isFloat)
    ConvertToFloat( op2 );
  else
  if (op2->isFloat)
    ConvertToFloat( op1 );
  else
  if (op1->value.i.isSigned == FALSE)
    op2->value.i.isSigned = FALSE;
  else
  if (op2->value.i.isSigned == FALSE)
    op1->value.i.isSigned = FALSE;
};

//--------------------------------------------------------------------------
// Name         fcalc_Convert
//
// Convert an value to a desired type. The type is specified as the
// code of the token (DOUBLE, INT or UNSIGNED).
// All errors (range) are ignored.
//--------------------------------------------------------------------------
void fcalc_Convert ( Val * op, int totype )
{
  switch (totype)
  {
    case DOUBLE:
      ConvertToFloat( op );
      break;

    case INT:
      if (op->isFloat)
      {
        op->isFloat = FALSE;
        op->value.i.v = (long)op->value.f;
      }
      op->value.i.isSigned = TRUE;
      break;

    case UNSIGNED:
      if (op->isFloat)
      {
        op->isFloat = FALSE;
        op->value.i.v = (ULONG)op->value.f;
      }
      op->value.i.isSigned = FALSE;
      break;

    default:
      assert( 0 );
  }
};

static void PushArg ( const Val * arg )
{
  if (NumArgs == MAX_ARGS)
    fcalc_Error( "Too many function arguments" );

  Args[NumArgs++] = *arg;
};


// Forward declarations of functions
//
static void yyerror ( char * msg );
static void FormatValue ( char * dest, Val * pVal, int radix, const char * fmt );

#define BINARY_PLUS( res, a, op, b )                    \
      {                                                 \
        ArithmeticConversion( &a, &b );                 \
        res = a;                                        \
        if (res.isFloat)                                \
          res.value.f op##= b.value.f;                  \
        else                                            \
          res.value.i.v op##= b.value.i.v;              \
      }

#define BINARY_OR( res, a, op, b, opname )              \
      {                                                 \
        if (a.isFloat || b.isFloat)                     \
          { yyerror( opname " requires integer operands" ); YYABORT; } \
        else                                            \
        {                                               \
          res = a;                                      \
          res.value.i.v op##= b.value.i.v;              \
          res.value.i.isSigned &= b.value.i.isSigned;   \
        }                                               \
      }

// Some stupid compilers do not define this although they do support
// the necessaary ANSI C features (like const and new-style function
// definitions). BISON's output in turn relies on __STDC__ to determine
// whether to actually use ANSI C-style.
//
#ifndef __STDC__
  #define __STDC__ 1
#endif

%}

%union
{
  int code;
  Val val;
}

%token<code> UNSIGNED INT FLOAT DOUBLE
%token<val>  NUMBER IDENT STRING

%left<code>  '|'
%left<code>  '^'
%left<code>  '&'
%left<code>  SHL SHR
%left<code>  '+' '-'
%left<code>  '*'  '/'  '%'
%right<code> '~'
%left<code>  '('

%type<val>   expr cast_expr unary primary function_call
%type<code>  radix type function radix_letter

%%

calc
  : expr radix
      {
        FormatValue( output, &$1, $2, szRadixFmt );
      }
  ;

radix
  : /* empty */               { $$ = 10; }
  | ',' radix_letter          { $$ = $2; }
  | ',' STRING
      {
        if (yyleng >= _countof(szRadixFmt))
          { yyerror("Too long format string"); YYABORT; };

        memcpy( szRadixFmt, yytext+1, yyleng-2 ); // preserve the fmt string
        szRadixFmt[yyleng-2] = 0;

        $$ = RADIX_FP_FORMAT;
      }
  ;

radix_letter
  : IDENT
      {
        $$ = 0;

        if (yyleng == 1)
          switch (yytext[0]) // note: idents are lowercase
          {
            case 'd': $$ = 10; break;
            case 'x': $$ = 16; break;
            case 'o': $$ = 8; break;
            case 'b': $$ = 2; break;
            case 'c': $$ = RADIX_CHAR; break;
          }

        if ($$ == 0)
          { yyerror( "Radix must be d, x, o, b or c" ); YYABORT; };
      }
  | NUMBER
      {
  		  if ($1.isFloat || $1.value.i.v > 26+10)
          { yyerror( "Invalid radix number" ); YYABORT; }
        else
          $$ = $1.value.i.v;
      }
  ;

expr
  : expr '+' expr       { BINARY_PLUS( $$, $1, +, $3 ); }
  | expr '-' expr       { BINARY_PLUS( $$, $1, -, $3 ); }
  | expr '*' expr       { BINARY_PLUS( $$, $1, *, $3 ); }
  | expr '/' expr       {
                          ArithmeticConversion( &$1, &$3 );
                          $$ = $1;
                          if ($$.isFloat)
                            $$.value.f /= $3.value.f;
                          else
                          {
                            CHECK_DIV0( $3.value.i.v );
                            if ($$.value.i.isSigned)
                              $$.value.i.v = (long)$$.value.i.v / (long)$3.value.i.v;
                            else
                              $$.value.i.v = (ULONG)$$.value.i.v / (ULONG)$3.value.i.v;
                          }
                        }
  | expr '%' expr       {
                          ArithmeticConversion( &$1, &$3 );
                          $$ = $1;
                          if ($$.isFloat)
                            $$.value.f = fmod( $$.value.f, $3.value.f );
                          else
                          {
                            CHECK_DIV0( $3.value.i.v );
                            if ($$.value.i.isSigned)
                              $$.value.i.v = (long)$$.value.i.v % (long)$3.value.i.v;
                            else
                              $$.value.i.v = (ULONG)$$.value.i.v % (ULONG)$3.value.i.v;
                          }
                        }
  | expr '|' expr       { BINARY_OR( $$, $1, |, $3, "Binary OR" ); }
  | expr '&' expr       { BINARY_OR( $$, $1, &, $3, "Binary AND" ); }
  | expr '^' expr       { BINARY_OR( $$, $1, ^, $3, "Binary XOR" ); }
  | expr SHL expr       {
                          if ($1.isFloat || $3.isFloat)
                            { yyerror( "Binary shift requires integer operands" ); YYABORT; }
                          else
                          {
                            $$ = $1;
                            $$.value.i.v <<= $3.value.i.v;
                          }
                        }

  | expr SHR expr       {
                          if ($1.isFloat || $3.isFloat)
                            { yyerror( "Binary shift requires integer operands" ); YYABORT; }
                          else
                          {
                            $$ = $1;
                            $$.value.i.v = $$.value.i.isSigned ?
                              (long)$$.value.i.v >> $3.value.i.v :
                              (ULONG)$$.value.i.v >> $3.value.i.v;
                          }
                        }
  | cast_expr
  ;

cast_expr
  : unary
  | '(' type ')' cast_expr
        {
          $$ = $4;
          fcalc_Convert( &$$, $2 );
        }
  ;

type
  : INT              { $$ = INT; }
  | UNSIGNED         { $$ = UNSIGNED; }
  | UNSIGNED INT     { $$ = UNSIGNED; }
  | FLOAT            { $$ = DOUBLE; }
  | DOUBLE           { $$ = DOUBLE; }
  ;

unary
  : primary
  | '~' cast_expr       {
                          if ($2.isFloat)
                            { yyerror( "Binary NOT requires an integer operand" ); YYABORT; }
                          else
                          {
                            $$ = $2;
                            $$.value.i.v = ~$$.value.i.v;
                          }
  											}
  | '-' cast_expr       {
  												$$ = $2;
                          if ($$.isFloat)
                            $$.value.f = -$$.value.f;
                          else
                            $$.value.i.v = -(long)$$.value.i.v;
  											}
  | '+' cast_expr       { $$ = $2; }
  ;

primary
  : NUMBER
  | '(' expr ')'        { $$ = $2; }
  | function_call
  ;

function_call
  : function            { fcalc_CallFunc( &$$, $1, NULL, 0 ); }
  | function '(' ')'    { fcalc_CallFunc( &$$, $1, NULL, 0 ); }
  | function '('
      { $<code>$ = NumArgs; }  // remember first arguments number
      args ')'
      {
        int firstArg, argCount;

        firstArg = $<code>3;
        argCount = NumArgs - firstArg;

        fcalc_CallFunc( &$$, $1, &Args[firstArg], argCount );

        NumArgs = firstArg;  // pop the arguments
      }
  ;

function
  : IDENT
      {
        if (($$ = fcalc_FindFunc( yytext )) == -1)
          fcalc_Error( "Undefined function '%s'", yytext );
      }
  ;

args
  : args ',' arg
  | arg
  ;

arg
  : expr   { PushArg( &$1 ); }
  ;



%%


//--------------------------------------------------------------------------
// Name         stp
//
// A very convinient string function that in my opinion should be
// present in every C library.
// It is very similar to strcpy() but returns a pointer to the *end*
// of the copied string thus combining strcpy() and strcat().
//--------------------------------------------------------------------------
static char * stp ( char * dest, char * src )
{
  while (*dest++ = *src++);
  return dest-1;
};

static char * ul2a ( unsigned long num, char * res, unsigned radix )
{
  unsigned i;
  unsigned char buf[sizeof(long)*8+8]; // Reserve space to convert a long into binary
  int      addRadix = 0;

  static const char digs[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  /*
    for 2, 8, 10, 16 radix: put a prefix
  */
  switch (radix)
  {
    case 10:
      break;
    case 2:
      *res++ = '#';
      break;
    case 8:
      *res++ = '0';
      break;
    case 16:
      *res++ = '0';
      *res++ = 'x';
      break;
    default:
      addRadix = 1;
      break;
  }

  i = 0;
  do
  {
    buf[i++] = digs[num % radix];
  }
  while (num /= radix);

  // For binary output align the resulting string to 8 digits
  if (radix == 2)
  {
    while (i & 7)
      buf[i++] = '0';
  }

  do
    *res++ = buf[--i];
  while (i);

  if (addRadix)
  {
    /* add the radix in format ",radix" */
    *res++ = ',';
//    *res++ = '[';
    res = ul2a( radix, res, 10 );
//    *res++ = ']';
  }

  *res = 0;
  return res;
};

static char * l2a ( long num, char * res, unsigned radix )
{
  if (num < 0)
  {
    *res++ = '-';
    num = -num;
  }
  return ul2a( num, res, radix );
};

// Validates the format string. I wrote this just for fun
//
static BOOLEAN ValidateFormat ( const char * fmt )
{
#define ZER  -1
#define D09  -2
#define E_F  -3

#define MAX_TRANS 3

  static const short states[][MAX_TRANS][2] =
  {
/* 0 */   { { '%', 1 } },
/* 1 */   { { D09, 2 }, { E_F, 5 }, { '.', 6 } },
/* 2 */   { { D09, 2 }, { E_F, 5 }, { '.', 3 } },
/* 3 */   { { D09, 4 }, { E_F, 5 } },
/* 4 */   { { D09, 4 }, { E_F, 5 } },
/* 5 */   { { ZER,-1 } },
/* 6 */   { { D09, 7 } },
/* 7 */   { { D09, 7 }, { E_F, 5 } },
  };

  // target state encoding:
  //   -1 : accept
  // character encoding:
  //   D09  : '0' .. '9'
  //   E_F  : 'E', 'F'
  //   ZER  : '\0'

  int curState, curChar;
  int trans;

  curState = 0;
  do
  {
    // translate curChar
    //
    curChar = *fmt++;
    if (isdigit( curChar ))
      curChar = D09;
    else
    if (curChar == 'e' || curChar == 'f')
      curChar = E_F;
    else
    if (curChar == 0)
      curChar = ZER;

    // Locate a transition
    //
    for ( trans = 0; states[curState][trans][0] != curChar; ++trans )
    {
      if (trans == MAX_TRANS - 1)
        return FALSE;
    };

    curState = states[curState][trans][1];
  }
  while (curState != -1);

  return TRUE;

#undef D09
#undef E_F
#undef MAX_TRANS
};


//
// Radix is the target base and must be within 1-36 except in some
// special cases:
//   RADIX_CHAR : target must be formated as a character
//   RADIX_FP_FORMAT : target must be formated according to _fmt_.
//
static void FormatValue ( char * dest, Val * pVal, int radix, const char * fmt )
{
  if (radix == RADIX_FP_FORMAT)
    fcalc_Convert( pVal, DOUBLE );
  else
  if (radix != 10)
    fcalc_Convert( pVal, UNSIGNED );

  if (pVal->isFloat)
  {
    if (radix == RADIX_FP_FORMAT)
    {
      if (!ValidateFormat( fmt ))
        fcalc_Error( "Invalid format string" );
    }
    else
    {
      ASSERT( radix == 10 );
      fmt = "%f";
    }

    sprintf( dest, fmt, pVal->value.f );
  }
  else
  {
    if (radix == RADIX_CHAR)
    {
      ULONG val;
#define HIGH_SHIFT ((sizeof(ULONG)/sizeof(char) - 1) * 8)
#define HIGH_CHAR  ((ULONG)UCHAR_MAX << HIGH_SHIFT)

      ASSERT( pVal->isFloat == FALSE );
      val = pVal->value.i.v;

      *dest++ = '\'';

      if (val != 0)
      {
	      while ((val & HIGH_CHAR) == 0) // Find the most significant character
          val <<= CHAR_BIT;
      }

      do
      {
        unsigned ch;
        ch = (val >> HIGH_SHIFT) & UCHAR_MAX;
        if (ch >= 32 && ch <= 127)
          *dest++ = ch;
        else
        {
          *dest++ = '\\';
          *dest++ = 'x';
          *dest++ = ch / 16 + '0';
          *dest++ = ch % 16 + '0';
        }
        val <<= CHAR_BIT;
      }
      while (val != 0);

      *dest++ = '\'';
      *dest = 0;

#undef HIGH_CHAR
#undef HIGH_SHIFT
    }
    else
    {
      if (pVal->value.i.isSigned)
      {
        // Only decimal numbers can be displayed as signed
        //
        ASSERT( radix == 10 );
        l2a( pVal->value.i.v, dest, radix );
      }
      else
        ul2a( pVal->value.i.v, dest, radix );
    }
  }
};

//--------------------------------------------------------------------------
// Name         yyerror
//
// Description
//--------------------------------------------------------------------------
static void yyerror ( char * msg )
{
  stp( stp( output, "**Error: " ), msg );
};

//--------------------------------------------------------------------------
// Name         fcalc_Error
//
// Generates the error message and aborts the computation by lomgjmp-ing
// to ErrJmp.
//--------------------------------------------------------------------------
void fcalc_Error ( const char * err, ... )
{
  va_list ap;
  char errMsgBuf[256];

  va_start( ap, err );

  vsprintf( errMsgBuf, err, ap );

  va_end( ap );

  yyerror( errMsgBuf );
  longjmp( ErrJmp, 1 );
};

//--------------------------------------------------------------------------
// Name         GuardCalc
//
// Description
//--------------------------------------------------------------------------
static int GuardCalc ( void )
{
  int volatile res = 1; // failure by default
  void ( * volatile old_sig )( int sig ) = signal( SIGFPE, SigFpe );

  DISABLE_FP_EXCEPTIONS

  if (setjmp( ErrJmp ) == 0)
    res = yyparse();

  RESTORE_FP_EXCEPTIONS

  signal( SIGFPE, old_sig );

  return res;
};

//--------------------------------------------------------------------------
// Name         Calc
//
// Description
//--------------------------------------------------------------------------
int Calc ( char * dest, const char * input )
{
  void * b;
  int res;

  // Initialize the lexer with the string
  //
  b = fcalc_ScanString( input );

  output = dest;
  NumArgs = 0;

  res = GuardCalc();

  // Release the resources used by the lexer
  //
  fcalc_EndString( b );

  return res;
};

#ifdef TEST_CALC
int main ( int argc, char ** argv )
{
  char buf[512], * p;
  char res[512];
  int  arg;

#if YYDEBUG
  yydebug = 1;
#endif

  buf[0] = 0;
  p = buf;
  for ( arg = 1; arg < argc; ++arg)
    p = stp( stp( p, " " ), argv[arg] );

  arg = Calc( res, buf );
  puts( res );

  return arg;
};
#endif

