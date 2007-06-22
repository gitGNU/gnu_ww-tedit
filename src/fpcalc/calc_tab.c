
/*  A Bison parser, made from calc.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse fcalc_parse
#define yylex fcalc_lex
#define yyerror fcalc_error
#define yylval fcalc_lval
#define yychar fcalc_char
#define yydebug fcalc_debug
#define yynerrs fcalc_nerrs
#define	UNSIGNED	258
#define	INT	259
#define	FLOAT	260
#define	DOUBLE	261
#define	NUMBER	262
#define	IDENT	263
#define	STRING	264
#define	SHL	265
#define	SHR	266

#line 1 "calc.y"

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


#line 228 "calc.y"
typedef union
{
  int code;
  Val val;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		63
#define	YYFLAG		-32768
#define	YYNTBASE	24

#define YYTRANSLATE(x) ((unsigned)(x) <= 266 ? yytranslate[x] : 37)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,    19,    12,     2,    21,
    23,    17,    15,    22,    16,     2,    18,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    11,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    10,     2,    20,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    13,    14
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     4,     7,    10,    12,    14,    18,    22,    26,
    30,    34,    38,    42,    46,    50,    54,    56,    58,    63,
    65,    67,    70,    72,    74,    76,    79,    82,    85,    87,
    91,    93,    95,    99,   100,   106,   108,   112,   114
};

static const short yyrhs[] = {    27,
    25,     0,     0,    22,    26,     0,    22,     9,     0,     8,
     0,     7,     0,    27,    15,    27,     0,    27,    16,    27,
     0,    27,    17,    27,     0,    27,    18,    27,     0,    27,
    19,    27,     0,    27,    10,    27,     0,    27,    12,    27,
     0,    27,    11,    27,     0,    27,    13,    27,     0,    27,
    14,    27,     0,    28,     0,    30,     0,    21,    29,    23,
    28,     0,     4,     0,     3,     0,     3,     4,     0,     5,
     0,     6,     0,    31,     0,    20,    28,     0,    16,    28,
     0,    15,    28,     0,     7,     0,    21,    27,    23,     0,
    32,     0,    34,     0,    34,    21,    23,     0,     0,    34,
    21,    33,    35,    23,     0,     8,     0,    35,    22,    36,
     0,    36,     0,    27,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   252,   259,   260,   261,   274,   291,   301,   302,   303,   304,
   318,   332,   333,   334,   335,   345,   356,   360,   361,   369,
   370,   371,   372,   373,   377,   378,   387,   394,   398,   399,
   400,   404,   405,   406,   408,   422,   430,   431,   435
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","UNSIGNED",
"INT","FLOAT","DOUBLE","NUMBER","IDENT","STRING","'|'","'^'","'&'","SHL","SHR",
"'+'","'-'","'*'","'/'","'%'","'~'","'('","','","')'","calc","radix","radix_letter",
"expr","cast_expr","type","unary","primary","function_call","@1","function",
"args","arg", NULL
};
#endif

static const short yyr1[] = {     0,
    24,    25,    25,    25,    26,    26,    27,    27,    27,    27,
    27,    27,    27,    27,    27,    27,    27,    28,    28,    29,
    29,    29,    29,    29,    30,    30,    30,    30,    31,    31,
    31,    32,    32,    33,    32,    34,    35,    35,    36
};

static const short yyr2[] = {     0,
     2,     0,     2,     2,     1,     1,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     1,     1,     4,     1,
     1,     2,     1,     1,     1,     2,     2,     2,     1,     3,
     1,     1,     3,     0,     5,     1,     3,     1,     1
};

static const short yydefact[] = {     0,
    29,    36,     0,     0,     0,     0,     2,    17,    18,    25,
    31,    32,    28,    27,    26,    21,    20,    23,    24,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     1,    34,    22,    30,     0,    12,    14,    13,
    15,    16,     7,     8,     9,    10,    11,     6,     5,     4,
     3,    33,     0,    19,    39,     0,    38,     0,    35,    37,
     0,     0,     0
};

static const short yydefgoto[] = {    61,
    33,    51,    55,     8,    21,     9,    10,    11,    53,    12,
    56,    57
};

static const short yypact[] = {     2,
-32768,-32768,     2,     2,     2,     0,    49,-32768,-32768,-32768,
-32768,    -7,-32768,-32768,-32768,    65,-32768,-32768,-32768,    35,
    47,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,    48,-32768,    82,-32768,-32768,     2,    61,    69,    76,
    81,    81,    84,    84,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,     2,-32768,    14,   -11,-32768,     2,-32768,-32768,
   104,   106,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,    13,    -3,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    50
};


#define	YYLAST		108


static const short yytable[] = {    13,
    14,    15,    16,    17,    18,    19,     1,     2,     1,     2,
    58,    59,     7,    34,     3,     4,     3,     4,    20,     5,
     6,     5,     6,    22,    23,    24,    25,    26,    27,    28,
    29,    30,    31,    54,    38,    39,    40,    41,    42,    43,
    44,    45,    46,    47,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    48,    49,    50,    36,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    35,    37,
    32,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    24,    25,    26,    27,    28,    29,    30,    31,    25,    26,
    27,    28,    29,    30,    31,    27,    28,    29,    30,    31,
    29,    30,    31,    62,    52,    63,     0,    60
};

static const short yycheck[] = {     3,
     4,     5,     3,     4,     5,     6,     7,     8,     7,     8,
    22,    23,     0,    21,    15,    16,    15,    16,     6,    20,
    21,    20,    21,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    37,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,     7,     8,     9,    23,    10,    11,
    12,    13,    14,    15,    16,    17,    18,    19,     4,    23,
    22,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    12,    13,    14,    15,    16,    17,    18,    19,    13,    14,
    15,    16,    17,    18,    19,    15,    16,    17,    18,    19,
    17,    18,    19,     0,    23,     0,    -1,    58
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Ceco:
   - modified to print debug output on YYSTDERR instead of stderr so it can be redirected
   - added check if alloca() returns NULL when resizing the stack
   - mapped __yy_memcpy() to memcpy()
   - changed the definitions to ANSI style
   - Fixed the alloca() includes to support BC and VisualC
*/
#include <string.h>  // for memcpy

#ifndef alloca
  #ifdef __GNUC__
    #define alloca __builtin_alloca
  #else /* not GNU C.  */
    #if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
      #include <alloca.h>
    #else /* not sparc */
      #if defined (MSDOS) || defined (__TURBOC__) || defined(_MSC_VER)
        #include <malloc.h>
        #if defined(_MSC_VER) && !defined(alloca)
          #define alloca _alloca
        #endif   /* Visual C */
      #else /* not MSDOS, not Visual C */
        #if defined(_AIX)
          #include <malloc.h>
          #pragma alloca
        #else /* not MSDOS, or _AIX */
          #ifdef __hpux
            #ifdef __cplusplus
              extern "C" {
                void *alloca (unsigned int);
              };
            #else /* not __cplusplus */
              void *alloca ();
            #endif /* not __cplusplus */
          #endif /* __hpux */
        #endif /* not _AIX */
      #endif /* not MSDOS, not Visual C */
    #endif /* not sparc.  */
  #endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#ifndef YYSTDERR
#  define YYSTDERR stderr
#endif
#define YYNOMORESTACK   do { yyerror( "parser error: stack overflow" ); return 2; } while (0)

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#define __yy_memcpy(FROM,TO,COUNT)	memcpy(TO,FROM,COUNT)

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM
#else
#define YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void
#endif

int
yyparse(YYPARSE_PARAM_DECL)
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(YYSTDERR, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      if (!yyss)
        YYNOMORESTACK;
      __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      if (!yyvs)
        YYNOMORESTACK;
      __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      if (!yyls)
        YYNOMORESTACK;
      __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(YYSTDERR, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(YYSTDERR, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(YYSTDERR, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(YYSTDERR, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (YYSTDERR, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (YYSTDERR, yychar, yylval);
#endif
	  fprintf (YYSTDERR, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(YYSTDERR, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (YYSTDERR, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (YYSTDERR, "%s ", yytname[yyrhs[i]]);
      fprintf (YYSTDERR, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 253 "calc.y"
{
        FormatValue( output, &yyvsp[-1].val, yyvsp[0].code, szRadixFmt );
      ;
    break;}
case 2:
#line 259 "calc.y"
{ yyval.code = 10; ;
    break;}
case 3:
#line 260 "calc.y"
{ yyval.code = yyvsp[0].code; ;
    break;}
case 4:
#line 262 "calc.y"
{
        if (yyleng >= _countof(szRadixFmt))
          { yyerror("Too long format string"); YYABORT; };

        memcpy( szRadixFmt, yytext+1, yyleng-2 ); // preserve the fmt string
        szRadixFmt[yyleng-2] = 0;

        yyval.code = RADIX_FP_FORMAT;
      ;
    break;}
case 5:
#line 275 "calc.y"
{
        yyval.code = 0;

        if (yyleng == 1)
          switch (yytext[0]) // note: idents are lowercase
          {
            case 'd': yyval.code = 10; break;
            case 'x': yyval.code = 16; break;
            case 'o': yyval.code = 8; break;
            case 'b': yyval.code = 2; break;
            case 'c': yyval.code = RADIX_CHAR; break;
          }

        if (yyval.code == 0)
          { yyerror( "Radix must be d, x, o, b or c" ); YYABORT; };
      ;
    break;}
case 6:
#line 292 "calc.y"
{
  		  if (yyvsp[0].val.isFloat || yyvsp[0].val.value.i.v > 26+10)
          { yyerror( "Invalid radix number" ); YYABORT; }
        else
          yyval.code = yyvsp[0].val.value.i.v;
      ;
    break;}
case 7:
#line 301 "calc.y"
{ BINARY_PLUS( yyval.val, yyvsp[-2].val, +, yyvsp[0].val ); ;
    break;}
case 8:
#line 302 "calc.y"
{ BINARY_PLUS( yyval.val, yyvsp[-2].val, -, yyvsp[0].val ); ;
    break;}
case 9:
#line 303 "calc.y"
{ BINARY_PLUS( yyval.val, yyvsp[-2].val, *, yyvsp[0].val ); ;
    break;}
case 10:
#line 304 "calc.y"
{
                          ArithmeticConversion( &yyvsp[-2].val, &yyvsp[0].val );
                          yyval.val = yyvsp[-2].val;
                          if (yyval.val.isFloat)
                            yyval.val.value.f /= yyvsp[0].val.value.f;
                          else
                          {
                            CHECK_DIV0( yyvsp[0].val.value.i.v );
                            if (yyval.val.value.i.isSigned)
                              yyval.val.value.i.v = (long)yyval.val.value.i.v / (long)yyvsp[0].val.value.i.v;
                            else
                              yyval.val.value.i.v = (ULONG)yyval.val.value.i.v / (ULONG)yyvsp[0].val.value.i.v;
                          }
                        ;
    break;}
case 11:
#line 318 "calc.y"
{
                          ArithmeticConversion( &yyvsp[-2].val, &yyvsp[0].val );
                          yyval.val = yyvsp[-2].val;
                          if (yyval.val.isFloat)
                            yyval.val.value.f = fmod( yyval.val.value.f, yyvsp[0].val.value.f );
                          else
                          {
                            CHECK_DIV0( yyvsp[0].val.value.i.v );
                            if (yyval.val.value.i.isSigned)
                              yyval.val.value.i.v = (long)yyval.val.value.i.v % (long)yyvsp[0].val.value.i.v;
                            else
                              yyval.val.value.i.v = (ULONG)yyval.val.value.i.v % (ULONG)yyvsp[0].val.value.i.v;
                          }
                        ;
    break;}
case 12:
#line 332 "calc.y"
{ BINARY_OR( yyval.val, yyvsp[-2].val, |, yyvsp[0].val, "Binary OR" ); ;
    break;}
case 13:
#line 333 "calc.y"
{ BINARY_OR( yyval.val, yyvsp[-2].val, &, yyvsp[0].val, "Binary AND" ); ;
    break;}
case 14:
#line 334 "calc.y"
{ BINARY_OR( yyval.val, yyvsp[-2].val, ^, yyvsp[0].val, "Binary XOR" ); ;
    break;}
case 15:
#line 335 "calc.y"
{
                          if (yyvsp[-2].val.isFloat || yyvsp[0].val.isFloat)
                            { yyerror( "Binary shift requires integer operands" ); YYABORT; }
                          else
                          {
                            yyval.val = yyvsp[-2].val;
                            yyval.val.value.i.v <<= yyvsp[0].val.value.i.v;
                          }
                        ;
    break;}
case 16:
#line 345 "calc.y"
{
                          if (yyvsp[-2].val.isFloat || yyvsp[0].val.isFloat)
                            { yyerror( "Binary shift requires integer operands" ); YYABORT; }
                          else
                          {
                            yyval.val = yyvsp[-2].val;
                            yyval.val.value.i.v = yyval.val.value.i.isSigned ?
                              (long)yyval.val.value.i.v >> yyvsp[0].val.value.i.v :
                              (ULONG)yyval.val.value.i.v >> yyvsp[0].val.value.i.v;
                          }
                        ;
    break;}
case 19:
#line 362 "calc.y"
{
          yyval.val = yyvsp[0].val;
          fcalc_Convert( &yyval.val, yyvsp[-2].code );
        ;
    break;}
case 20:
#line 369 "calc.y"
{ yyval.code = INT; ;
    break;}
case 21:
#line 370 "calc.y"
{ yyval.code = UNSIGNED; ;
    break;}
case 22:
#line 371 "calc.y"
{ yyval.code = UNSIGNED; ;
    break;}
case 23:
#line 372 "calc.y"
{ yyval.code = DOUBLE; ;
    break;}
case 24:
#line 373 "calc.y"
{ yyval.code = DOUBLE; ;
    break;}
case 26:
#line 378 "calc.y"
{
                          if (yyvsp[0].val.isFloat)
                            { yyerror( "Binary NOT requires an integer operand" ); YYABORT; }
                          else
                          {
                            yyval.val = yyvsp[0].val;
                            yyval.val.value.i.v = ~yyval.val.value.i.v;
                          }
  											;
    break;}
case 27:
#line 387 "calc.y"
{
  												yyval.val = yyvsp[0].val;
                          if (yyval.val.isFloat)
                            yyval.val.value.f = -yyval.val.value.f;
                          else
                            yyval.val.value.i.v = -(long)yyval.val.value.i.v;
  											;
    break;}
case 28:
#line 394 "calc.y"
{ yyval.val = yyvsp[0].val; ;
    break;}
case 30:
#line 399 "calc.y"
{ yyval.val = yyvsp[-1].val; ;
    break;}
case 32:
#line 404 "calc.y"
{ fcalc_CallFunc( &yyval.val, yyvsp[0].code, NULL, 0 ); ;
    break;}
case 33:
#line 405 "calc.y"
{ fcalc_CallFunc( &yyval.val, yyvsp[-2].code, NULL, 0 ); ;
    break;}
case 34:
#line 407 "calc.y"
{ yyval.code = NumArgs; ;
    break;}
case 35:
#line 409 "calc.y"
{
        int firstArg, argCount;

        firstArg = yyvsp[-2].code;
        argCount = NumArgs - firstArg;

        fcalc_CallFunc( &yyval.val, yyvsp[-4].code, &Args[firstArg], argCount );

        NumArgs = firstArg;  // pop the arguments
      ;
    break;}
case 36:
#line 423 "calc.y"
{
        if ((yyval.code = fcalc_FindFunc( yytext )) == -1)
          fcalc_Error( "Undefined function '%s'", yytext );
      ;
    break;}
case 39:
#line 435 "calc.y"
{ PushArg( &yyvsp[0].val ); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 450 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (YYSTDERR, "state stack now");
      while (ssp1 != yyssp)
	fprintf (YYSTDERR, " %d", *++ssp1);
      fprintf (YYSTDERR, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(YYSTDERR, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (YYSTDERR, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (YYSTDERR, " %d", *++ssp1);
      fprintf (YYSTDERR, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(YYSTDERR, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 440 "calc.y"



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
  while ((*dest++ = *src++));
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

