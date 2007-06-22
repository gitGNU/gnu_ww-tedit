%{

/*

File: py_synh.l
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th September, 2003
Descrition:
  Syntax highlighting for C and C++

General design and Flex declarations prepared by Tzvetan Mikov

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../synh.h"
#include "py_syntax2.h"

static int BracketLevel;
static int bHasBracket;
static int bZero;

static unsigned OutLen;

// InLine and InLen define the line being scanned at the moment.
static const char * InLine = NULL;
static int InLen = 0;

static void set_yy_input ( const char * line, int len )
{
  InLine = line;
  InLen = len;
  YY_NEW_FILE; // use to be yyrestart();
}

#define YY_INPUT(buf,result,max_size)                   \
  do {                                                  \
    int len = InLen < max_size ? InLen : max_size;      \
    memcpy( buf, InLine, len );                         \
    InLen -= len;                                       \
    InLine += len;                                      \
    result = len;                                       \
  } while (0)

static TSynHInterf *pSynH;

static void out ( char ch, Color_t attr )
{
  pSynH->pfnPutAttr(attr, OutLen, OutLen, pSynH);
  ++OutLen;
}

static void outcur ( Color_t attr )
{
  int i;

  pSynH->pfnPutAttr(attr, OutLen, OutLen + yyleng - 1, pSynH);
  OutLen += yyleng;
}

%}

%option outfile="py_synh.c"
%option prefix="py"
%option noyywrap
%option nodefault

WS       [ \f\v\t]+
SUFFIX   [uUlL]*

%x LINE_COMMENT CHAR_STATE STR_STATE BIGSTR_STATE

%%

and |
del |
for |
is |
raise |
assert |
elif |
from |
lambda |
return |
break |
else |
global |
not |
try |
class |
except |
if |
or |
while |
continue |
exec |
import |
pass |
yield |
def |
finally |
in |
print          outcur( COLOR_RESWORD );

[A-Za-z_][A-Za-z_0-9]*  outcur( COLOR_TEXT );

[0-9]+{SUFFIX}         outcur( COLOR_NUMBER );
0x[0-9a-fA-F]+{SUFFIX}  outcur( COLOR_NUMBER );

"#"            outcur( COLOR_COMMENT ); BEGIN( LINE_COMMENT );

\"\"\"         outcur( COLOR_STRING ); BEGIN( BIGSTR_STATE );

\'             outcur( COLOR_STRING ); BEGIN( CHAR_STATE );
\"             outcur( COLOR_STRING ); BEGIN( STR_STATE );

[\+\-\;\.\}\{\*\/\%\>\<\=\:\!\(\)\,\]\[\|\&|\~\?] out( *yytext, COLOR_OPERATOR );

\n
.              out( *yytext, COLOR_TEXT );


<LINE_COMMENT>{
        \0              BEGIN(INITIAL);
        \\\0            out( '\\', COLOR_COMMENT ); return 0;
        \n
        .               outcur( COLOR_COMMENT );
}

<CHAR_STATE>{
        \0              BEGIN(INITIAL);
        \'              outcur( COLOR_STRING ); BEGIN( INITIAL );
        \\\'            outcur( COLOR_STRING );
        \\\0            out( '\\', COLOR_STRING ); return 0;
        \n
        .               outcur( COLOR_STRING );
}

<STR_STATE>{
        \0              BEGIN(INITIAL);
        \"              outcur( COLOR_STRING ); BEGIN( INITIAL );
        \\\"            outcur( COLOR_STRING );
        \\\0            out( '\\', COLOR_STRING ); return 0;
        \\.             outcur( COLOR_STRING );
        \n
        .               outcur( COLOR_STRING );
}

<BIGSTR_STATE>{
        \0              return 0;
	\"\"\"          outcur( COLOR_STRING ); BEGIN( INITIAL );
        \n
        .               outcur( COLOR_STRING );
}

%%

/*
---apply_py_colors
Returns status to be stored for this line
Communicates parameters to yy_scan_bytes() via a set of global variables.
prevln_status: lo_word -> yy_start, hi_word & 0x7f -> BracketLevel
Returns;
  yy_start | BracketLevel | bHasBracket | bZero
Important:
  This function always scans ther entire line.
*/
int apply_py_colors(char *line, int len, int prevln_status, TSynHInterf *pApplyInterf)
{
  int my_yy_start;
  unsigned int ln_status;  /* 32-bits */

  pSynH = pApplyInterf;
  OutLen = 0;
  my_yy_start = prevln_status & 0xffff;
  BracketLevel = (prevln_status >> 16) & 0x1fff;
  bHasBracket = 0;
  BEGIN(my_yy_start);
  set_yy_input(line, len + 1);
  yylex();
  if (BracketLevel == 0)
    bZero = 1;

  /* update get_char_region_syntax_types() if the compisition below changes! */
  ln_status = BracketLevel & 0x3fff;  /* bracket level at the e-o-l */
  if (bHasBracket)  /* has at least one { or } */
    ln_status |= 0x4000;
  if (bZero)  /* brackets next level reaches 0 somewhere on this line */
    ln_status |= 0x2000;
  ln_status = (ln_status << 16) | (YY_START);
  return ln_status;
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2003
Petar Marinov

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

