%{
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "calcpriv.h"
#include "calc_tab.h"

#define ScannerError          fcalc_Error 

// The escapes table. Converts from '\x' to a character code.
//
#define FIRST_ESCAPE 97
#define NUM_ESCAPES  22
static char EscapeTable[NUM_ESCAPES] =
{
  '\a','\b',0,0,0,'\f',0,0,0,0,0,0,0,'\n',0,0,0,'\r',0,'\t',0,'\v'
};

#define GET_ESCAPE(c)   EscapeTable[(c) - FIRST_ESCAPE]

// Reserved word table
//
typedef struct
{
  const char * text;
  int code;
} TRESWORD;

#include "calcops.inc"

//--------------------------------------------------------------------------
// Name         ScannerClassifyIdent
//
// Returns 0 if this is not calc operator
// Otherwise returns the operator token code.
//--------------------------------------------------------------------------
static int ScannerClassifyIdent ( const char * szIdent, int len )
{
  if (len >= RES_C_WORD_MIN_LEN && len <= RES_C_WORD_MAX_LEN)
  {
    int hash;

    // we have certain assumptions about C reserved words that decrease
    // table size

    ASSERT( szIdent[0] >= '0' && szIdent[0] <= 'z' );
    ASSERT( szIdent[len - 1] >= '0' &&
            szIdent[len - 1] <= 'z' );

    // calculate perfect hash value

    hash = HashCharValue[ szIdent[0] - '0' ] +
           HashCharValue[ szIdent[len - 1] - '0' ] +
           ((szIdent[1] - '_' + 1) & 7);

    // note: len + 1 is not very big

    if (hash >= 0 && hash < _countof( ResWordsTable ) &&
        memcmp( ResWordsTable[hash].text, szIdent, len + 1 ) == 0)
    {
      return ResWordsTable[hash].code;
    }
  }

  return 0;
};

//--------------------------------------------------------------------------
// Name         ident
//
// An identifier has been recognized.
//--------------------------------------------------------------------------
static int ident ( char * ident, int len )
{
  int op;

  strlwr( ident ); // we are case insensitive

  return (op = ScannerClassifyIdent( ident, len )) != 0 ? op : IDENT;
};

//--------------------------------------------------------------------------
// Name         number
//
// An integer number has been recognized.
//--------------------------------------------------------------------------
static int number ( const char * text, int radix )
{
  ULONG val;

  errno = 0;
  val = strtoul( text, NULL, radix );
  if (errno != 0)
    ScannerError( strerror( errno ) );

  yylval.val.isFloat = FALSE;
  yylval.val.value.i.v = val;
  yylval.val.value.i.isSigned = FALSE;

  return NUMBER;
};

//--------------------------------------------------------------------------
// Name         fnumber
//
// A floating point number has been recognized.
//--------------------------------------------------------------------------
static int fnumber ( const char * text )
{
  double val;

  errno = 0;
  val = strtod( text, NULL );
  if (errno != 0)
    ScannerError( strerror( errno ) );

  yylval.val.isFloat = TRUE;
  yylval.val.value.f = val;

  return NUMBER;
};

//--------------------------------------------------------------------------
// Name         char_const
//
// A character constant has been recognized.
//--------------------------------------------------------------------------
static int char_const ( const char * charBuf, int len )
{
  ULONG val;

  if (len == 0)
  {
    static const char dummyCharConst = ' ';

    ScannerError( "Empty character constant" );

    charBuf = &dummyCharConst;
    len = 1;
  }

  if (len > sizeof(ULONG))
  {
    ScannerError( "Too long character constant" );
    len = 1;
  }

  val = 0;

  do
    val = (val << CHAR_BIT) | *((unsigned char *)charBuf);
  while (++charBuf, --len);

  yylval.val.isFloat = FALSE;
  yylval.val.value.i.v = val;
  yylval.val.value.i.isSigned = TRUE;

  return NUMBER;
};

%}

%option noyywrap
%option nodefault

IDENT    [a-zA-Z_][a-zA-Z_0-9]*

XDIGIT   [0-9a-fA-F]

/* a fractional number */
FRACT    ([0-9]*\.[0-9]+)|([0-9]+\.)

/* a fractional or integer number */
FRACT_OR_INT {FRACT}|[0-9]+

/* exponent */
EXP      [eE][+-]?[0-9]+

%x CHAR_STATE

%%

						char charBuf[8];
            int  charLen;
            #define ADD_CHAR( ch )                                 \
                if (charLen == sizeof(charBuf))                    \
                  ScannerError( "Too long character constant" );   \
                else                                               \
                  charBuf[charLen++] = ch;

"<<"        return SHL;
">>"        return SHR;
"<"         return '<';
">"         return '>';

{FRACT_OR_INT}{EXP}	return fnumber( yytext );
{FRACT}            	return fnumber( yytext );

0[0-7]*[uU]      		return number( yytext, 8 );
[1-9][0-9]*[uU]  		return number( yytext, 10 );
0x{XDIGIT}+[uU]? 		return number( yytext+2, 16 );
0[0-7]*         		number( yytext, 8 ); yylval.val.value.i.isSigned = TRUE; return NUMBER;
[1-9][0-9]*     		number( yytext, 10 ); yylval.val.value.i.isSigned = TRUE; return NUMBER;

#[01]+           		return number( yytext+1, 2 );
${XDIGIT}+       		return number( yytext+1, 16 );

[01]+b           		return number( yytext, 2 );
[0-9]{XDIGIT}*h	 		return number( yytext, 16 );
[0-7]+[qo]       		return number( yytext, 8 );


[ \t\n]+  					/* ignore whitespace */

{IDENT}     				return ident( yytext, yyleng );

\".*\"              return STRING;
\".*                ScannerError("Unterminated string constant");

\'          				charLen = 0; BEGIN( CHAR_STATE );

.										return *yytext;

<CHAR_STATE>{
        \'          BEGIN(INITIAL); return char_const( charBuf, charLen );

        \\[\'\"\\\?]    ADD_CHAR( yytext[1] );
        \\[abfnrtv]     ADD_CHAR( GET_ESCAPE(yytext[1]) );

        \\[0-7]{1,3}    {
                          int ch = strtol( yytext + 1, NULL, 8 );

                          if (ch > 255 || ch < 0)
                            ScannerError( "%d is invalid for a character", ch );
                          else
                            ADD_CHAR( ch );
                        }

        \\x{XDIGIT}{1,2}  ADD_CHAR( strtol( yytext + 2, NULL, 16 ) );

        \\.             ScannerError("Unrecognized charater escape '%s'", yytext );

        <<EOF>>         ScannerError("Unterminated character constant"); BEGIN(INITIAL);

        \n              ScannerError("Unterminated character constant"); BEGIN(INITIAL);

        .               ADD_CHAR( yytext[0] );
}

%%

// Interface provided for CALC.Y in order to access yy_scan_string()
// and yy_delete_buffer() whose declarations are very hard to export
// because they use types declared and defined only in LEXYY.C
//
void * fcalc_ScanString ( const char * str )
{
  return (void *)yy_scan_string( str );
};

void fcalc_EndString ( void * buf )
{
  yy_delete_buffer( (YY_BUFFER_STATE)buf );
};

#ifdef TEST_LEXYY
YYSTYPE yylval;

int main ( void )
{
  int c;
  while (c = yylex())
  {
    if (c == NUMBER)
    {
      if (yylval.val.isFloat)
        printf( "fnum: %f\n", yylval.val.value.f );
      else
      {
        if (yylval.val.value.i.isSigned)
	        printf( "num: %ld\n", yylval.val.value.i.v );
        else
          printf( "num: %lu\n", yylval.val.value.i.v );
      }
    }
    else
      printf( "%d\n", c );
  }
  return 0;
};
#endif
