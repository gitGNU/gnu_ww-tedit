/*

File: wlimits.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 17th March, 1999 (moved out from l1opt.h)
Descrition:
  Applications specific limits and definitions

*/

#ifndef WLIMITS_H
#define WLIMITS_H

#define FILE_DELTA  200         /* Index array grow up size */
#define UNDOINDEX_DELTA  50     /* UndoIndex array grow up size */
#define MAX_KEY_SEQ  3          /* Max keys allowed to be in a single key sequence */
#define MAX_KEYS  255           /* Max number of keys allowed in a key set */
#define MAX_WIN_WIDTH  255      /* Max output window width (2 * buffer in stack!) */
#define MAX_FILE_MSG_LEN  160   /* Max len of a message to be displayed at st. */
#define MAX_SEARCH_STR  160     /* Max search string len (something line 2 lines) */
#define MAX_SECTION_NAME_LEN  25  /* INI file section name lenght */
#define MAX_KEY_NAME_LEN  25    /* INI file key name lenght */
#define MAX_VAL_LEN  256        /* INI file key value length */
#define MAX_MRU_FILES  64       /* Max files in a MRU list */
#define MAX_HISTORY_ITEMS  45   /* Max lines in a history list */
#define MAX_CALCULATOR_HISTORY_ITEMS  8  /* Max lines in a history list for calculator */
#define MAX_FINDINFILES_HISTORY_ITEMS 15  /* Max lines in history list for "Find in Files" prompt */
#define MAX_RECOVERY_TIME_VAL  999  /* In seconds */
#define MAX_RIGHT_MARGIN_VAL  1024  /* Change to whatever, no limit! */
#define MAX_TAB_SIZE  25  /* Could be much larger, for example 2048 :-) */
#define MAX_EXT_LEN  85  /* TDocType, (may be compounded) */
#define MAX_FTYPE_NAME_LEN  25  /* TDocType */
#define MAX_DOCS  20  /* Max number of document types */
#define MAX_CALC_BUF 255  /* Max size of the input line for the calculator */
#define MAX_NODE_LEN 45  /* Max size of a title of a hypertext page */
#define MAX_CACHED_PAGES 10  /* How much pages to cache */
#define MAX_CACHED_FILES 4  /* How much info file indexes to cache */
#define MAX_TEXTBUF 1024  /* Search() prepares here for multiple-line search */
#define MAX_CLIP_HIST 5  /* How much clipboards to keep in history */
#define MAX_CLIP_HIST_WIN_WIDTH 25  /* The width of the selection window */
#define MAX_CONTAINERS 24  /* Number of simultaneously displayed containers */

#ifdef UNIX
#define CASE_SENSITIVE_FILENAMES 1  /* BOOLEAN */
#else
#define CASE_SENSITIVE_FILENAMES 0  /* BOOLEAN */
#endif
#define SAFETY_POOL (32 * 1024)
#define MAX_KEY_SEQUENCE  8  /* for l_kbd.c */
#define KEY_TIMEOUT  100000  /* 100ms time-out inbetween 2 characters */

#define READ_BINARY_FILE "rb"
#define WRITE_BINARY_FILE "wb"
/* for unix READ_BINARY_FILE "r"; WRITE_BINARY_FILE "w" */

#if CASE_SENSITIVE_FILENAMES
#define filestrcmp  strcmp
#else
#define filestrcmp  stricmp
#endif

#ifdef WIN32
#define USE_ASCII_BOXES 1  /* boolean: don't use + and - to draw boxes */
#define PATH_SLASH_CHAR  '\\'
#define DEFAULT_EOL_SIZE 2
#define DEFAULT_EOL  "\r\n"
#define DEFAULT_EOL_TYPE  CRLFtype
#endif

#ifdef MSDOS
#define USE_ASCII_BOXES 0  /* boolean: don't use + and - to draw boxes */
#define PATH_SLASH_CHAR  '\\'
#define DEFAULT_EOL_SIZE 2
#define DEFAULT_EOL  "\r\n"
#define DEFAULT_EOL_TYPE  CRLFtype
#endif

#ifdef UNIX
#define USE_ASCII_BOXES 1  /* boolean: use + and - to draw boxes */
#define PATH_SLASH_CHAR  '/'
#define DEFAULT_EOL_SIZE 1
#define DEFAULT_EOL  "\n"
#define DEFAULT_EOL_TYPE  LFtype
#endif

#endif  /* ifndef WLIMITS_H */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002
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

