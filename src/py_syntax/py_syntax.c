/*

File: py_syntax.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th December, 2003
Descrition:
  Python syntax highlighting.
  Registers the highlighting procedure with the appropriate document type.

*/

#include "../global.h"
#include "../doctype.h"
#include "../synh.h"
#include "py_syntax.h"
#include "py_syntax2.h"

/* Document type description */
static TDocType PythonDocDesc =
{
  "*.py",  // char sExt[MAX_EXT_LEN]
  0,       // int nType
  8,       // int nTabSize
  1,       // BOOLEAN bUseTabs
  1,       // BOOLEAN bOptimalFill
  1,       // BOOLEAN bAutoIndent
  1,       // BOOLEAN bBackspaceUnindent
  0,       // BOOLEAN bCursorThroughTabs
  0,       // BOOLEAN bWordWrap
};

void PyLangRegister(void)
{
  TDocType *pPyDoc;
  int nCType;  /* C/C++ as registered in the system */

  nCType = FindSyntaxType("Python");
  if (nCType < 0)
    nCType = AddSyntaxType("Python");
  ASSERT(nCType >= 0);
  SetSyntaxProc(nCType, apply_py_colors);

  pPyDoc = DetectADocument("test.py");
  if (pPyDoc == NULL && DocTypesEmptyEntries() > 0)
  {
    pPyDoc = &PythonDocDesc;
    pPyDoc->nType = nCType;
    DocTypesInsertEntry(pPyDoc, -1);
  }
  else  /* *.py migth have been entered manually, now set the syntax type */
    pPyDoc->nType = nCType;
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2004
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
