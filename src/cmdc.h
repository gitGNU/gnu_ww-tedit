/*

File: cmdc.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW text editor
Started: 25th April, 2006
Descrition:
  Editor command functions context definition

*/

#ifndef CMDC_H
#define CMDC_H

struct cmdc
{
  #ifdef _DEBUG
  BYTE magic_byte;
  #define CMDC_MAGIC 0x61
  #endif

  void *wrkspace;
  void *param1;
};

typedef struct cmdc cmdc_t;

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_CMDC(cmdc) ((cmdc) != NULL && (cmdc)->magic_byte == CMDC_MAGIC)
#else
#define VALID_CMDC(cmdc) (1)
#endif

#define _CMDC_PFILE(cmdc) \
  (VALID_CMDC(cmdc) ? (TFile*)(cmdc)->param1 : 0)
#define CMDC_PFILE(cmdc) _CMDC_PFILE((cmdc_t *)cmdc)

#define _CMDC_WRKSPACE(cmdc) \
  (VALID_CMDC(cmdc) ? (wrkspace_data_t *)(cmdc)->wrkspace : 0)
#define CMDC_WRKSPACE(cmdc) _CMDC_WRKSPACE((cmdc_t *)cmdc)

#ifdef _DEBUG
#define CMDC_SET(cmdc, _wrkspace, _param1)\
do                                        \
{                                         \
  memset(&cmdc, 0, sizeof(cmdc));         \
  cmdc.magic_byte = CMDC_MAGIC;           \
  cmdc.wrkspace = _wrkspace;              \
  cmdc.param1 = _param1;                  \
}                                         \
while(0)
#else
#define CMDC_SET(cmdc, _wrkspace, _param1)\
do                                        \
{                                         \
  memset(&cmdc, 0, sizeof(cmdc));         \
  cmdc.wrkspace = _wrkspace;              \
  cmdc.param1 = _param1;                  \
}                                         \
while(0)
#endif

#endif  /* ifndef CMDC_H */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2006
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

