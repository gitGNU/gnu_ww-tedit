/*

File: xclip.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th Jun, 2005
Descrition:
  Functions that store and get text from the X Window clipboard.

*/

#ifndef XCLIP_H
#define XCLIP_H

struct disp_ctx;

void xclipbrd_request(struct disp_ctx *pdisp_ctx, int b_get_primary);
char *xclipbrd_extract(struct disp_ctx *pdisp_ctx);
void xclipbrd_free(char *sel_data);
void xclipbrd_announce(struct disp_ctx *pdisp_ctx, int b_primary);
void xclipbrd_send(struct disp_ctx *pdisp_ctx,
  int requestor, int requestor_property, int time, int target_type,
  const char *pdata, int b_primary);

#endif

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2005
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

