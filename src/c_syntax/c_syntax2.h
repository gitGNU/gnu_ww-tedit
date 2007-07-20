/*

File: c_syntax2.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 19th October, 2003
Descrition:
  C/C++ syntax highlighting internal API

*/

#ifndef C_SYNTAX2_H
#define C_SYNTAX2_H

#include "../synh.h"

extern int apply_c_colors(char *line, int len,
  int prevln_status, TSynHInterf *pApplyInterf);
extern int function_name_scan(int nStartLine, int nStartPos,
  int nNumLines, int nEndLine,
  TFunctionName FuncNames[], int nMaxEntries,
  TLinesNavInterf *pNavInterf);
extern int c_lang_is_over_bracket(TSynHRegion BracketRegions[], int nMaxEntries,
  TBracketBlockTooltip *pBlockTooltip,
  TEditInterf *pEditInterf);
extern int c_lang_calc_indent(TEditInterf *pEditInterf);
extern int c_lang_examine_key(char ascii_char, TEditInterf *pEditInterf);

#endif  /* ifndef C_SYNTAX2_H */

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
