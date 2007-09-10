/*

File: funcs.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th December, 2003
Descrition:
  C/C++ function names scan routine

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "c_syntax2.h"

#define MAX_TEXT_SCAN_BUF 160

typedef struct GetCharContext
{
  TSynHInterf stApplyInterf;

  int nStartLine;
  int nStartPos;
  int nNumLines;
  int nEndLine;
  TLinesNavInterf *pNavInterf;
  TEditInterf *pEditInterf;

  char CharTypes[MAX_TEXT_SCAN_BUF];
  int nRegionLine;
  int nRegionStart;
  int nLine;
  int nPos;

  char CurChar;
  char CurType;

  char *psCurLine;
  int nCurLineLen;
  int nCurLineStatus;
  int nCurLineBracketLevel;
  int nCurLineBracketFlag;
  int nCurLineZeroFlag;
  int nPrevLineBracketLevel;

  int nNextLine;
  int nNextPos;
} TGetCharContext;

/*
---get_char_apply_type_proc
Character navigation functions use this as an interface to store
the individual characters syntax types supplied by the syntax highlighting
procedure apply_c_colors().
note: nRegionEnd is inclusive end of range!
*/
static void apply_type_proc(int attr, int nRegionStart, int nRegionEnd,
    struct SynHInterf *pContext)
{
  int nWrtEdge;
  int nWinWidth;
  char *pOutputBuf;
  int nStartPos;
  int nEndPos;
  int i;

  nWrtEdge = pContext->nWrtEdge;
  nWinWidth = pContext->nWinWidth;
  pOutputBuf = pContext->pOutputBuf;

  nStartPos = nRegionStart  - nWrtEdge;
  nEndPos = nRegionEnd - nWrtEdge;

  if (nStartPos < 0 && nEndPos < 0)
    return;

  if (nStartPos < 0)
    nStartPos = 0;

  for (i = nStartPos; i <= nEndPos && i < nWinWidth; ++i)
    pOutputBuf[i] = attr;
}

/*
---get_cur_line
*/
static void get_cur_line(TGetCharContext *pstTextContext)
{
  pstTextContext->psCurLine = pstTextContext->pNavInterf->pfnGetLine(
    pstTextContext->nLine,
    pstTextContext->pNavInterf);
  pstTextContext->nCurLineLen = pstTextContext->pNavInterf->pfnGetLineLen(
    pstTextContext->nLine,
    pstTextContext->pNavInterf);
}

/*
---get_char_region_syntax_types
Extracts the syntax types for a region of characters (MAX_TEXT_SCAN_BUF)
Region start is pstTextContext->nRegionStart/nReagionLine
*/
static void get_char_region_syntax_types(TGetCharContext *pstTextContext)
{
  int prev_ln_status;
  unsigned int ln_status;

  prev_ln_status = pstTextContext->pNavInterf->pfnGetLineStatus(
    pstTextContext->nLine - 1,
    pstTextContext->pNavInterf);

  /*
  Specify the region in the TApplyInterf structure
  */
  pstTextContext->stApplyInterf.nWrtEdge = pstTextContext->nRegionStart;
  pstTextContext->stApplyInterf.nWinWidth = MAX_TEXT_SCAN_BUF;
  pstTextContext->stApplyInterf.pOutputBuf = pstTextContext->CharTypes;
  pstTextContext->stApplyInterf.pfnPutAttr = apply_type_proc;

  apply_c_colors(pstTextContext->psCurLine, pstTextContext->nCurLineLen,
    prev_ln_status, &pstTextContext->stApplyInterf);

  ln_status = pstTextContext->pNavInterf->pfnGetLineStatus(
      pstTextContext->nLine,
      pstTextContext->pNavInterf);

  pstTextContext->nCurLineStatus = ln_status & 0xffff;
  pstTextContext->nCurLineBracketLevel = (ln_status >> 16) & 0x1fff;

  pstTextContext->nCurLineBracketFlag = 0;
  if ((ln_status & 0x40000000) != 0)
   pstTextContext->nCurLineBracketFlag = 1;
  pstTextContext->nCurLineZeroFlag = 0;
  if ((ln_status & 0x20000000) != 0)
   pstTextContext->nCurLineZeroFlag = 1;

  pstTextContext->nPrevLineBracketLevel = (prev_ln_status >> 16) & 0x1fff;
}

/*
---cur_pos_is_inside_syntax_region
Returns
  0 -- cur pos is inside our known syntax region window
  -1 -- cur pos is before the start of the know syntax region window
  +1 -- cur pos is after the end of the known syntax region window
*/
static int cur_pos_is_inside_syntax_region(TGetCharContext *pstTextContext)
{
  if (pstTextContext->nRegionStart == -1)
    return 1;  /* first time call */
  if (pstTextContext->nLine < pstTextContext->nRegionLine)
    return -1;
  if (pstTextContext->nLine > pstTextContext->nRegionLine)
    return 1;
  if (pstTextContext->nPos < pstTextContext->nRegionStart)
    return -1;
  if (pstTextContext->nPos >= pstTextContext->nRegionStart + MAX_TEXT_SCAN_BUF)
    return 1;
  return 0;
}

/*
---get_char_syntax_type
Returns the syntax type of the character at nLine/nPos.
Calls apply_c_colors() to extract new region of colors when needed.
*/
static char get_char_syntax_type(TGetCharContext *pstTextContext)
{
  int nRegionStart;
  int index;

  nRegionStart = -1;
  switch (cur_pos_is_inside_syntax_region(pstTextContext))
  {
    case 0:  /* cur_pos is still inside the buffer */
      break;
    case -1:  /* cur_pos is before the start of the buffer */
      nRegionStart = pstTextContext->nPos - MAX_TEXT_SCAN_BUF + 1;
      if (nRegionStart < 0)
        nRegionStart = 0;
      break;
    case 1:  /* cur_pos is after the end of the buffer */
      nRegionStart = pstTextContext->nPos;
      break;
  }
  if (nRegionStart != -1)
  {
    pstTextContext->nRegionStart = nRegionStart;
    pstTextContext->nRegionLine = pstTextContext->nLine;
    get_char_region_syntax_types(pstTextContext);
  }

  if (pstTextContext->nPos >= pstTextContext->nCurLineLen)
    return 0;

  index = pstTextContext->nPos - pstTextContext->nRegionStart;
  return pstTextContext->CharTypes[index];
}

/*
---setup
This sets the context (TGetCharContext*) for
the character navigation functions get_char_forward()/get_char_reverse().
*/
static void setup(int nStartLine, int nStartPos,
  int nNumLines, int nEndLine,
  TGetCharContext *pstTextContext, TLinesNavInterf *pNavInterf)
{
  pstTextContext->pNavInterf = pNavInterf;
  pstTextContext->nStartLine = nStartLine;
  pstTextContext->nStartPos = nStartPos;
  pstTextContext->nNumLines = nNumLines;
  pstTextContext->nEndLine = nEndLine;
  pstTextContext->nNextLine = nStartLine;
  pstTextContext->nNextPos = nStartPos;
  pstTextContext->nLine = -1;
  pstTextContext->nPos = -1;
  pstTextContext->nRegionStart = -1;
  pstTextContext->nRegionLine = -1;
  pstTextContext->psCurLine = NULL;
  pstTextContext->nCurLineLen = -1;
  pstTextContext->pEditInterf = NULL;
}

/*
---get_line
Moves current position to NextPos/NextLine.
Extracts psCurLine (and attributes), CurType, CurChar.
*/
static void get_line(TGetCharContext *pstTextContext)
{
  pstTextContext->nPos = pstTextContext->nNextPos;
  if (pstTextContext->nLine != pstTextContext->nNextLine)
  {
    pstTextContext->nLine = pstTextContext->nNextLine;
    get_cur_line(pstTextContext);
  }
}

/*
---get_char
Gets character at NextPos/NextLine
0 - no more characters on this line
*/
static int get_char(TGetCharContext *pstTextContext)
{
  get_line(pstTextContext);
  pstTextContext->CurType = get_char_syntax_type(pstTextContext);
  if (pstTextContext->nPos >= pstTextContext->nCurLineLen)
    return 0;
  pstTextContext->CurChar = pstTextContext->psCurLine[pstTextContext->nPos];
  return 1;
}

/*
---goto_next_line
0 -- no more lines
*/
static int goto_next_line(TGetCharContext *pstTextContext)
{
  pstTextContext->nNextPos = 0;
  if (pstTextContext->nNextLine + 1 ==
    pstTextContext->nStartLine + pstTextContext->nNumLines)
    return 0;
  ++pstTextContext->nNextLine;
  return 1;
}

/*
---goto_next_char
Advances NextPos/NextLine
0 -- no more characters
*/
static int goto_next_char(TGetCharContext *pstTextContext)
{
  if (pstTextContext->nPos >= pstTextContext->nCurLineLen - 1)
    return 0;
  ++pstTextContext->nNextPos;
  return 1;
}

/*
---goto_prev_line
0 -- no more lines
*/
static int goto_prev_line(TGetCharContext *pstTextContext)
{
  pstTextContext->nNextPos = 0;
  if (pstTextContext->nNextLine == pstTextContext->nStartLine)
    return 0;
  --pstTextContext->nNextLine;
  return 1;
}

/*
---goto_prev_char
Moves NextPos/NextLine a character leftward on the line
0 -- no more characters
*/
static int goto_prev_char(TGetCharContext *pstTextContext)
{
  if (pstTextContext->nPos == 0)
    return 0;
  --pstTextContext->nNextPos;
  return 1;
}

/*
---set_pos
*/
static int set_pos(TGetCharContext *pstTextContext, int nLine, int nPos)
{
  pstTextContext->nNextLine = nLine;
  pstTextContext->nNextPos = nPos;
  if (pstTextContext->nNextLine > pstTextContext->nEndLine)
    return 0;
  return 1;
}

/*
---set_pos_end_of_line
*/
static int set_pos_end_of_line(TGetCharContext *pstTextContext)
{
  if (pstTextContext->nCurLineLen == 0)
    return 0;
  pstTextContext->nNextPos = pstTextContext->nCurLineLen - 1;
  return 1;
}

/*
---find_open_parenthesis
*/
static int find_open_parenthesis(TGetCharContext *pstTextContext)
{
  int num;

  num = 0;
_l1:
  if (!goto_prev_char(pstTextContext))
  {
    if (!goto_prev_line(pstTextContext))
      return 0;
    get_line(pstTextContext);  /* just to extract line-length */
    if (!set_pos_end_of_line(pstTextContext))  /* line empty? */
      goto _l1;
  }
  get_char(pstTextContext);
  if (pstTextContext->CurType == COLOR_OPERATOR)
  {
    if (pstTextContext->CurChar == ')')
      ++num;
    if (pstTextContext->CurChar == '(')
    {
      if (num == 0)
        return 1;
      --num;
    }
  }
  goto _l1;
}

/*
---find_closing_parenthesis
*/
static int find_closing_parenthesis(TGetCharContext *pstTextContext)
{
  int num;
  int b_quit;
  int b_result;

  num = 0;

  b_quit = 0;
  b_result = 0;
  while (!b_quit)
  {
    /* move a char forward */
    if (!goto_next_char(pstTextContext))
    {
      /* or a line forward */
      do  /* or a line forward */
      {
        if (!goto_next_line(pstTextContext))
          goto _exit;
        get_char(pstTextContext);
      }
      while (!pstTextContext->nCurLineLen == 0);  /* skip over empty lines */
    }

    /* get char and status */
    get_char(pstTextContext);
    if (pstTextContext->CurType == COLOR_OPERATOR)
    {
      if (pstTextContext->CurChar == '(')
        ++num;
      if (pstTextContext->CurChar == ')')
      {
        if (num == 0)
        {
          b_quit = 1;
          b_result = 1;
        }
        else
          --num;
      }
    }
  }

_exit:
  return b_result;
}

/*
---find_line_with_bracket_forward
Search forward for a line that contains at least a single '{' or '}'
*/
static int find_line_with_bracket_forward(TGetCharContext *pstTextContext)
{
  do
  {
    if (!goto_next_line(pstTextContext))
      return 0;
    get_char(pstTextContext);  /* get length and status */
  }
  while (!pstTextContext->nCurLineBracketFlag);
  return 1;
}

/*
---find_closing_bracket
On entry: current position is on top of '{'.
On exit: current position is on the corresponding '}'
Returns 0: no matching closing bracket found
Returns 1: operation succeeded
*/
static int find_closing_bracket(TGetCharContext *pstTextContext)
{
  int num;
  int b_quit;
  int result;

  num = 0;

  b_quit = 0;
  while (!b_quit)
  {
    /* move a char forward */
    if (!goto_next_char(pstTextContext))
    {
      /* or a line forward */
      if (!find_line_with_bracket_forward(pstTextContext))
      {
        result = 0;
        break;
      }
    }

    /* get char and status */
    get_char(pstTextContext);
    if (pstTextContext->CurType == COLOR_OPERATOR)
    {
      if (pstTextContext->CurChar == '{')
        ++num;
      if (pstTextContext->CurChar == '}')
      {
        if (num == 0)
          return 1;
        --num;
      }
    }
  }

  return result;
}

/*
---find_line_with_bracket_backward
Search backward for a line that contains at least a single '{' or '}'
*/
static int find_line_with_bracket_backward(TGetCharContext *pstTextContext)
{
  do
  {
    if (!goto_prev_line(pstTextContext))
      return 0;
    get_char(pstTextContext);  /* get length and status */
  }
  while (!pstTextContext->nCurLineBracketFlag);
  pstTextContext->nNextPos = pstTextContext->nCurLineLen - 1;
  return 1;
}

/*
---find_opening_bracket
On entry: current position is on top of '}'.
On exit: current position is on the corresponding '{'
Returns 0: no matching closing bracket found
Returns 1: operation succeeded
*/
static int find_opening_bracket(TGetCharContext *pstTextContext)
{
  int num;
  int b_quit;
  int result;

  num = 0;

  b_quit = 0;
  while (!b_quit)
  {
    /* move a char barcward */
    if (!goto_prev_char(pstTextContext))
    {
      /* or a line backward */
      if (!find_line_with_bracket_backward(pstTextContext))
      {
        result = 0;
        break;
      }
    }

    /* get char and status */
    get_char(pstTextContext);
    if (pstTextContext->CurType == COLOR_OPERATOR)
    {
      if (pstTextContext->CurChar == '}')
        ++num;
      if (pstTextContext->CurChar == '{')
      {
        if (num == 0)
        {
          result = 1;
          b_quit = 1;
        }
        else
          --num;
      }
    }
  }

  return result;
}

/*
---calc_bracket_level
Calculates the bracket level at (after) the current cursor position.
The fact that we keep bracket level at the end of every line
in the file makes this a very fast operation.
*/
static int calc_bracket_level(TGetCharContext *pstTextContext)
{
  int nBracketLevel;
  int nTargetPos;

  nTargetPos = pstTextContext->nPos;

  /* get prev line bracket level */
  get_char(pstTextContext);
  nBracketLevel = pstTextContext->nPrevLineBracketLevel;

  /* move to the beginning of this line */
  pstTextContext->nNextPos = 0;

_l0:
  get_char(pstTextContext);
  if (pstTextContext->CurType == COLOR_OPERATOR)
  {
    if (pstTextContext->CurChar == '{')
      ++nBracketLevel;
    if (pstTextContext->CurChar == '}')
      --nBracketLevel;
  }
  if (pstTextContext->nPos == nTargetPos)
    return nBracketLevel;
  if (!goto_next_char(pstTextContext))
    return -1;
  goto _l0;
}

/*
---find_block_start
We are about insert a '}' or we are on top of a '}'.
This function searches for the beginning of the block. Notes the position
of the opening '{' and continues the scan for the block operator -- if,
while, do, for, or a it could be a function name.
*/
static int find_block_start(TGetCharContext *pstTextContext,
  TSynHRegion bracket_regions[], int max_entries)
{
  /* [1] is the current '}' */
  bracket_regions[1].nLine = pstTextContext->nLine;
  bracket_regions[1].nPos = pstTextContext->nPos;
  bracket_regions[1].nLen = 1;
  /* find the remote '{' */
  if (!find_opening_bracket(pstTextContext))
    return 0;
  bracket_regions[0].nLine = pstTextContext->nLine;
  bracket_regions[0].nPos = pstTextContext->nPos;
  bracket_regions[0].nLen = 1;
  return 1;
}

/*
---strrev2
Not present on unix platforms
*/
static void strrev2(char *buf)
{
  char *p;
  char *p2;
  int l;
  char temp;

  l = strlen(buf);
  p2 = &buf[l - 1];
  p = buf;

  while (p < p2)
  {
    temp = *p;
    *p = *p2;
    *p2 = temp;
    ++p;
    --p2;
  }
}

/*
---is_function_name
*/
static int is_function_name(char c)
{
  if (isalpha(c))
    return 1;
  if (isdigit(c))
    return 1;
  if (c == '_')
    return 1;
  return 0;
}

/*
---is_blank
*/
static int is_blank(char c)
{
  if (c == ' ' || c == '\t')
    return 1;
  return 0;
}

/*
---skip_blanks_comments_backward
Skips over blanks and comments until first non-blank
*/
static int skip_blanks_comments_backward(TGetCharContext *pstTextContext)
{
  do
  {
    /* move a char barcward */
    if (!goto_prev_char(pstTextContext))
    {
      do  /* or a line backward */
      {
        if (!goto_prev_line(pstTextContext))
          return 0;
        get_char(pstTextContext);
      }
      while (!set_pos_end_of_line(pstTextContext));
    }

    /* get char and status */
    get_char(pstTextContext);
  }
  while (pstTextContext->CurType == COLOR_COMMENT ||
         is_blank(pstTextContext->CurChar));
  return 1;
}

/*
---skip_blanks_comments_forward
Skips over blanks and comments until first non-blank
*/
static int skip_blanks_comments_forward(TGetCharContext *pstTextContext)
{
  int b_result;

  b_result = 0;
  do
  {
    /* move a char forward */
    if (!goto_next_char(pstTextContext))
    {
      do  /* or a line forward */
      {
        if (!goto_next_line(pstTextContext))
          goto _exit;
        get_char(pstTextContext);
      }
      while (!pstTextContext->nCurLineLen == 0);  /* skip over empty lines */
    }

    /* get char and status */
    get_char(pstTextContext);
  }
  while (pstTextContext->CurType == COLOR_COMMENT ||
         is_blank(pstTextContext->CurChar));
  b_result = 1;
_exit:
  return b_result;
}

/*
---skip_to_reserved_word_backward
Skips over blanks and comments until first non-blank
*/
static int skip_to_reserved_word_backward(TGetCharContext *pstTextContext)
{
  do
  {
    /* move a char barcward */
    if (!goto_prev_char(pstTextContext))
    {
      do  /* or a line backward */
      {
        if (!goto_prev_line(pstTextContext))
          return 0;
        get_char(pstTextContext);
      }
      while (!set_pos_end_of_line(pstTextContext));
    }

    /* get char and status */
    get_char(pstTextContext);
  }
  while (pstTextContext->CurType != COLOR_RESWORD);

  return 1;
}

/*
---function_name_scan
nStartLine -- begin the function scan from here
nNumLines -- how many lines at the outside (usially until the end of the file)
nEndLine -- what is the line of the last function name of interest
*/
int function_name_scan(int nStartLine, int nStartPos,
  int nNumLines, int nEndLine,
  TFunctionName FuncNames[], int nMaxEntries,
  TLinesNavInterf *pNavInterf)
{
  TGetCharContext stTextContext;
  int nNumFunctions;
  int nFuncNameEndPos;
  int b;
  int b_opening_bracket_found;
  int save_ln;
  int save_pos;

  if (nNumLines == 0)
    return 0;

  nNumFunctions = 0;
  setup(nStartLine, nStartPos, nNumLines, nEndLine, &stTextContext, pNavInterf);

  set_pos(&stTextContext, nStartLine, nStartPos);

  /*
  Search for an opening '{'
  */
_find_function_block:
  b_opening_bracket_found = 0;
  while (!b_opening_bracket_found)
  {
    get_char(&stTextContext);  /* get char and status */
    while (!stTextContext.nCurLineBracketFlag)  /* no brackets on this line? */
      if (!find_line_with_bracket_forward(&stTextContext))
        return nNumFunctions;
    while (1)  /* walk this line and examine all '{' characters */
    {
      get_char(&stTextContext);
      if ( stTextContext.CurType == COLOR_OPERATOR &&
           stTextContext.CurChar == '{' )
      {
        if (calc_bracket_level(&stTextContext) == 1)
        {
          b_opening_bracket_found = 1;
          break;  /* bracket transition from level 0 to 1 ! */
        }
      }
      if (!goto_next_char(&stTextContext))
      {
        if (!goto_next_line(&stTextContext))
          return nNumFunctions;
        break;
      }
    }
  }

  /*
  We are at '{' that opens a function block
  */
  save_pos = stTextContext.nPos;
  save_ln = stTextContext.nLine;

  /*
  First seek for ')' backward
  And then jump to the start of parameters list -- '('
  */
  if (!skip_blanks_comments_backward(&stTextContext))
  {
_restore_and_skip_to_end_of_block:
    set_pos(&stTextContext, save_ln, save_pos);
    get_char(&stTextContext);
    if (!find_closing_bracket(&stTextContext))
      return nNumFunctions;
    goto _find_function_block;
  }

  if (stTextContext.CurType == COLOR_OPERATOR &&
      stTextContext.CurChar == ')')
  {
    /* Jump over the the () pair */
    if (!find_open_parenthesis(&stTextContext))
      goto _restore_and_skip_to_end_of_block;

    /* move a char backward beyond '(' */
    if (!goto_prev_char(&stTextContext))
      if (!goto_prev_line(&stTextContext))
        goto _restore_and_skip_to_end_of_block;
  }
  else
    goto _restore_and_skip_to_end_of_block;

  nFuncNameEndPos = stTextContext.nPos - 1;

  /*
  We are at the start of the parameter list of the function '('
  Walk backward to find the last character of the function
  */
  while (1)
  {
    b = goto_prev_char(&stTextContext);
    if (!b) /* no more characters, we hit start of the line */
      break;
    get_char(&stTextContext);
    if (! ( stTextContext.CurType == COLOR_TEXT &&
           is_function_name(stTextContext.CurChar) )
       )
      break;  /* end of function name (going backward) */
  }

  /*
  Register the function name in FuncNames[]
  */
  if (stTextContext.nLine > stTextContext.nEndLine)
    return nNumFunctions;  /* it is out of range of interest */
  memset(&FuncNames[nNumFunctions], 0, sizeof(TFunctionName));
  FuncNames[nNumFunctions].nLine = stTextContext.nLine;
  if (b)
  {
    FuncNames[nNumFunctions].nNamePos = stTextContext.nPos + 1;
    FuncNames[nNumFunctions].nNameLen =
      nFuncNameEndPos - stTextContext.nPos;
  }
  else  /* include the 0 position */
  {
    FuncNames[nNumFunctions].nNamePos = stTextContext.nPos;
    FuncNames[nNumFunctions].nNameLen =
      nFuncNameEndPos - stTextContext.nPos + 1;
  }

  /*
  Find the end of the function block
  */
  stTextContext.nNextPos = save_pos;
  stTextContext.nNextLine = save_ln;
  find_closing_bracket(&stTextContext);
  FuncNames[nNumFunctions].nFuncEndLine = stTextContext.nLine;
  FuncNames[nNumFunctions].nFuncEndPos = stTextContext.nPos;
  ++nNumFunctions;
  if (nNumFunctions == nMaxEntries)
    return nNumFunctions;
  goto _find_function_block;
}

/*
---extract_reserved_word_backward
Moves backward across the blanks and the comments
and extracts the next word
*/
static int extract_reserved_word_backward(TGetCharContext *pstTextContext,
  int start_line, int start_pos, char *buf, int buf_size,
  int *word_line, int *word_pos)
{
  int result;
  int p;
  int b_line_begin;

  result = 0;
  set_pos(pstTextContext, start_line, start_pos);
  get_char(pstTextContext);

  if (!skip_blanks_comments_backward(pstTextContext))
    goto _exit;

  p = 0;
  b_line_begin = 0;
  do
  {
    if (! ( pstTextContext->CurType == COLOR_TEXT
            || pstTextContext->CurType == COLOR_RESWORD )
       )
       break;
    buf[p] = pstTextContext->CurChar;
    ++p;
    if (p > buf_size)
      goto _exit;
    if (!goto_prev_char(pstTextContext))
    {
      b_line_begin = 1;
      break;
    }
    get_char(pstTextContext);
  }
  while (is_function_name(pstTextContext->CurChar));

  buf[p] = '\0';
  strrev2(buf);
  result = 1;

  *word_line = pstTextContext->nLine;
  *word_pos = pstTextContext->nPos + 1;
  if (b_line_begin)
    *word_pos = pstTextContext->nPos;

_exit:
  return result;
}

/*
---extract_reserved_word
Moves forward across the blanks and the comments
and extracts the next word
*/
static int extract_reserved_word(TGetCharContext *pstTextContext,
  int start_line, int start_pos, char *buf, int buf_size,
  int *word_line, int *word_pos)
{
  int result;
  int p;

  buf[0] = '\0';
  result = 0;
  set_pos(pstTextContext, start_line, start_pos);
  get_char(pstTextContext);

  do
  {
    /* move a char forward */
    if (!goto_next_char(pstTextContext))
    {
      do  /* or a line forward */
      {
        if (!goto_next_line(pstTextContext))
          goto _exit;
        get_char(pstTextContext);
      }
      while (pstTextContext->nCurLineLen == 0);
    }

    /* get char and status */
    get_char(pstTextContext);
  }
  while (pstTextContext->CurType == COLOR_COMMENT ||
         is_blank(pstTextContext->CurChar));

  *word_line = pstTextContext->nLine;
  *word_pos = pstTextContext->nPos;
  p = 0;
  do
  {
    buf[p] = pstTextContext->CurChar;
    ++p;
    if (p > buf_size)
      goto _exit;
    if (!goto_next_char(pstTextContext))
      break;
    get_char(pstTextContext);
  }
  while (is_function_name(pstTextContext->CurChar));

  buf[p] = '\0';
  result = 1;

_exit:
  return result;
}

/*
---swap_region_entries
*/
static void swap_region_entries(TSynHRegion *entry1, TSynHRegion *entry2)
{
  int nTempPos;
  int nTempLine;
  int nTempLen;

  nTempPos = entry1->nPos;
  nTempLine = entry1->nLine;
  nTempLen = entry1->nLen;
  entry1->nPos = entry2->nPos;
  entry1->nLine = entry2->nLine;
  entry1->nLen = entry2->nLen;
  entry2->nPos = nTempPos;
  entry2->nLine = nTempLine;
  entry2->nLen = nTempLen;
}

/*
---extract_brackets_tooltip_lines
Extracts the lines that compose a tooltip when a {} block is highlighted.
The tooltip comprises the part of the {} that is not visible on the screen.
*/
static int extract_brackets_tooltip_lines(TGetCharContext *pstTextContext,
  int block_line, int block_pos, int block_end_line, int block_end_pos,
  int res_word_line, int res_word_pos, int res_word_len,
  int res_word_num_lines,  /* do {} while (condition more than one line) */
  char *res_word,  /* if reserved word is overrided, "else/if" for example */
  TBracketBlockTooltip *pBlockTooltip, TEditInterf *pEditInterf)
{
  int size;
  int top_line;
  int num_visible_lines;
  int cur_line;
  char *pline;
  int line_len;
  int copy_len;
  int b_result;

  b_result = 0;
  pBlockTooltip->NumLines = 0;

  pEditInterf->pfnGetCurPagePos(&top_line, NULL, &num_visible_lines, pEditInterf);

  /*
  Two cases:
  1. reserved word is before the block, example "for () {}"
  2. reserved word is after the block, example "do {} while () "
  */
  if (res_word_line <= block_line)  /* case1 */
  {
    if (res_word_line >= top_line)  /* entirely visible? */
      goto _exit;  /* no tooltip */
    cur_line = res_word_line;
    pBlockTooltip->bIsTop = 1;
  }
  else  /* case2 */
  {
    if (res_word_line + res_word_num_lines <
      top_line + num_visible_lines)
      goto _exit;
    cur_line = block_end_line;
    pBlockTooltip->bIsTop = 0;
  }

  size = 0;
  while (size < pBlockTooltip->BufSize - 1)
  {
    /*
    Get line text and lenght
    */
    pline = pEditInterf->stNavInterf.pfnGetLine(cur_line,
      &pEditInterf->stNavInterf);
    line_len = pEditInterf->stNavInterf.pfnGetLineLen(cur_line,
      &pEditInterf->stNavInterf);

    /*
    Transfer text in the tooltip buf
    */
    copy_len = line_len;
    if (size + copy_len + 2 > pBlockTooltip->BufSize)
      copy_len = pBlockTooltip->BufSize - size - 2;
    memcpy(&pBlockTooltip->pDestBuf[size], pline, copy_len);
    size += copy_len;
    pBlockTooltip->pDestBuf[size] = '\0';
    ++pBlockTooltip->NumLines;

    /*
    Move to the next line
    */
    ++cur_line;
    if (cur_line == res_word_line + res_word_num_lines + 1)
      break;
    pBlockTooltip->pDestBuf[size] = '\n';
    size++;
  }
  pBlockTooltip->pDestBuf[size] = '\0';
  b_result = 1;

_exit:
  return b_result;
}

/*
---find_block_operator
For the block {} already stored in bracket_regions extracts the operators.
Those are if/else/while/do-while/function-name. The operator that defines
the condition is always stored in bracket_regions[2].
*/
static int find_block_operator(TGetCharContext *pstTextContext,
  TSynHRegion bracket_regions[], int max_entries,
  TBracketBlockTooltip *pBlockTooltip, TEditInterf *pEditInterf)
{
  int block_line;
  int block_pos;
  int block_end_line;
  int block_end_pos;
  int result;
  #define MAX_WORD_SIZE 80
  char word[MAX_WORD_SIZE];
  int word_line;
  int word_pos;
  int save_ln;
  int save_pos;
  int res_word_line;
  int res_word_pos;
  int res_word_num_lines;
  int res_word_len;

  /*
  bracket_regions[] holds block positions in no
  particular order. we have to sort by line number to
  find which one is block-begin and block-end
  */
  if (bracket_regions[0].nLine < bracket_regions[1].nLine)
  {
    block_line = bracket_regions[0].nLine;
    block_pos = bracket_regions[0].nPos;
    block_end_line = bracket_regions[1].nLine;
    block_end_pos = bracket_regions[1].nPos;
  }
  else
    if (bracket_regions[1].nLine < bracket_regions[0].nLine)
    {
      block_line = bracket_regions[1].nLine;
      block_pos = bracket_regions[1].nPos;
      block_end_line = bracket_regions[0].nLine;
      block_end_pos = bracket_regions[0].nPos;
    }
    else
      if (bracket_regions[0].nPos < bracket_regions[1].nPos)
      {
        block_line = bracket_regions[0].nLine;
        block_pos = bracket_regions[0].nPos;
        block_end_line = bracket_regions[1].nLine;
        block_end_pos = bracket_regions[1].nPos;
      }
      else
      {
        block_line = bracket_regions[1].nLine;
        block_pos = bracket_regions[1].nPos;
        block_end_line = bracket_regions[0].nLine;
        block_end_pos = bracket_regions[0].nPos;
      }

  /*
  Walk backward for the first non-comment character
  */
  set_pos(pstTextContext, block_line, block_pos);
  get_char(pstTextContext);
  result = 0;  /* asssume failure */

  /*
  Check for ')' find the pair
  Restore pos otherwise
  */
  save_ln = pstTextContext->nLine;
  save_pos = pstTextContext->nPos;

  if (!skip_blanks_comments_backward(pstTextContext))
    goto _exit;

  if (pstTextContext->CurType == COLOR_OPERATOR &&
      pstTextContext->CurChar == ')')
  {
    /* Jump over the the () pair */
    if (!find_open_parenthesis(pstTextContext))
      goto _exit;

    /* move a char backward beyond '(' */
    if (!goto_prev_char(pstTextContext))
      if (!goto_prev_line(pstTextContext))
        goto _exit;
  }
  else
  {
    /* restore */
    set_pos(pstTextContext, save_ln, save_pos);
    get_char(pstTextContext);  /* only to apply the new pos */
  }

  /* Extract the word */
  extract_reserved_word_backward(pstTextContext,
    pstTextContext->nLine, pstTextContext->nPos,
    word, MAX_WORD_SIZE,
    &bracket_regions[2].nLine, &bracket_regions[2].nPos);
  res_word_len = strlen(word);
  bracket_regions[2].nLen = res_word_len;

  pBlockTooltip->pDestBuf[0] = '\0';  /* assume no tooltip */
  pBlockTooltip->NumLines = 0;

  if (res_word_len != 0)  /* was there block header word detected? */
  {
    res_word_line = bracket_regions[2].nLine;
    res_word_pos = bracket_regions[2].nPos;
    res_word_num_lines = block_line - res_word_line;
    extract_brackets_tooltip_lines(pstTextContext,
      block_line, block_pos, block_end_line, block_end_pos,
      res_word_line, res_word_pos, res_word_len,
      res_word_num_lines, NULL,
      pBlockTooltip, pEditInterf);
  }

  if (strcmp(word, "do") == 0)
  {
    if (!extract_reserved_word(pstTextContext, block_end_line, block_end_pos,
      word, MAX_WORD_SIZE, &word_line, &word_pos))
      goto _exit;
    if (strcmp(word, "while") != 0)
      goto _exit;
    bracket_regions[3].nLine = word_line;
    bracket_regions[3].nPos = word_pos;
    bracket_regions[3].nLen = 5;

    /*
    Find how many lines is the condition in () spread over
    */
    res_word_num_lines = 0;
    res_word_line = word_line;
    res_word_pos = word_pos;
    if (skip_blanks_comments_forward(pstTextContext))
    {
      if (pstTextContext->CurType == COLOR_OPERATOR &&
          pstTextContext->CurChar == '(')
      {
        /* Jump over the the () pair */
        if (find_closing_parenthesis(pstTextContext))
          res_word_num_lines = pstTextContext->nLine - res_word_line;
        extract_brackets_tooltip_lines(pstTextContext,
          block_line, block_pos, block_end_line, block_end_pos,
          res_word_line, res_word_pos, 5,
          res_word_num_lines, NULL,
          pBlockTooltip, pEditInterf);
      }
    }

    /* [2] is always the line that may generate tooltip TODO: */
    swap_region_entries(&bracket_regions[2], &bracket_regions[3]);
  }
  else
    if (strcmp(word, "else") == 0)
    {
      /*
      Check for } then look for the opening {
      and then for the () pair and the "if" keyword
      */
      if (!skip_blanks_comments_backward(pstTextContext))
        goto _exit;
      if (pstTextContext->CurType == COLOR_OPERATOR &&
          pstTextContext->CurChar == '}')
      {
        /* Jump over the the {} pair */
        if (!find_opening_bracket(pstTextContext))
          goto _exit;
        if (!skip_blanks_comments_backward(pstTextContext))
          goto _exit;

        if (pstTextContext->CurType == COLOR_OPERATOR &&
            pstTextContext->CurChar == ')')
        {
          /* Jump over the the () pair */
          if (!find_open_parenthesis(pstTextContext))
            goto _exit;

          /* move a char backward beyond '(' */
          if (!goto_prev_char(pstTextContext))
            if (!goto_prev_line(pstTextContext))
              goto _exit;
        }
        else
          goto _exit;

        /* Extract the word */
        extract_reserved_word_backward(pstTextContext,
          pstTextContext->nLine, pstTextContext->nPos,
          word, MAX_WORD_SIZE,
          &bracket_regions[3].nLine, &bracket_regions[3].nPos);
        bracket_regions[3].nLen = strlen(word);
      }
      else  /* there is no {} between the 'if' and the 'else' */
      {
        /*
        Simply search backward for the first reserved word "if"
        */
        do
        {
          if (!skip_to_reserved_word_backward(pstTextContext))
            goto _exit;
          /* Extract the word */
          extract_reserved_word_backward(pstTextContext,
            pstTextContext->nLine, pstTextContext->nPos + 1,
            word, MAX_WORD_SIZE,
            &bracket_regions[3].nLine, &bracket_regions[3].nPos);
        }
        while (strcmp(word, "if") != 0);
        bracket_regions[3].nLen = strlen(word);
      }
    }

  result = 1;

_exit:
  return result;
}

/*
---c_lang_is_over_bracket
*/
int c_lang_is_over_bracket(TSynHRegion BracketRegions[], int nMaxEntries,
  TBracketBlockTooltip *pBlockTooltip,
  TEditInterf *pEditInterf)
{
  int nCol;
  int nRow;
  int nPos;
  int nNumLines;
  TGetCharContext stTextContext;
  int nTempPos;
  int nTempLine;

  memset(BracketRegions, 0, nMaxEntries * sizeof(TSynHRegion));
  pBlockTooltip->NumLines = 0;

  nNumLines = pEditInterf->pfnGetNumLines(pEditInterf);
  setup(0, 0, nNumLines, nNumLines - 1,
    &stTextContext, &pEditInterf->stNavInterf);

  pEditInterf->pfnGetCurPos(&nCol, &nRow, &nPos, pEditInterf);
  if (!set_pos(&stTextContext, nRow, nPos))
    return 0;

  get_char(&stTextContext);
  pBlockTooltip->bCursorIsOverOpenBracket = 0;
  if (stTextContext.CurType == COLOR_OPERATOR)
  {
    if (stTextContext.CurChar == '}')
    {
      if (!find_block_start(&stTextContext, BracketRegions, nMaxEntries))
        goto _exit;
    }
    else
      if (stTextContext.CurChar == '{')
      {
        pBlockTooltip->bCursorIsOverOpenBracket = 1;
        if (!find_closing_bracket(&stTextContext))
          goto _exit;
        if (!find_block_start(&stTextContext, BracketRegions, nMaxEntries))
          goto _exit;
        nTempPos = BracketRegions[0].nPos;
        nTempLine = BracketRegions[0].nLine;
        BracketRegions[0].nPos = BracketRegions[1].nPos;
        BracketRegions[0].nLine = BracketRegions[1].nLine;
        BracketRegions[1].nPos = nTempPos;
        BracketRegions[1].nLine = nTempLine;
      }
      else
        goto _exit;

    if (!find_block_operator(&stTextContext,
      BracketRegions, nMaxEntries,
      pBlockTooltip, pEditInterf))
    {
      goto _exit;
    }
  }

_exit:
  if (BracketRegions[0].nLen > 0)
    return 1;
  return 0;
}

#define MAX_CALC_INDENT 12

/*
on exit:
returns indent of line, -1 line is empty (len=0 or only spaces)
stTextContext.nPos/CurChar point to first non-blank of the line
*/
static int find_line_indent(TGetCharContext *pstTextContext, int line)
{
  TEditInterf *pEditInterf;
  int indent;

  pEditInterf = pstTextContext->pEditInterf;

  if (!set_pos(pstTextContext, line, 0))  /* From pos 0 of cur line */
    return -1;
  get_char(pstTextContext);

  if (pstTextContext->nCurLineLen == 0)
    return -1;

  if (!is_blank(pstTextContext->CurChar))
    return 0;
  else
  {
    while (is_blank(pstTextContext->CurChar))
    {
      indent = pstTextContext->nPos;
      /* Adjust for any tab characters */
      indent = pEditInterf->pfnGetTabPos(indent, line, pEditInterf);
      if (!goto_next_char(pstTextContext))
        return -1;
      get_char(pstTextContext);
    }
    return indent + 1;
  }
}

/*
searches for the first indent below a line
indent must be !=0, and different than current line indent
*/
static int find_indent_downward(TGetCharContext *pstTextContext)
{
  int indent;
  int line;

  line = pstTextContext->nLine + 1;

  for (;; ++line)
  {
    indent = find_line_indent(pstTextContext, line);

    if (indent == -1)
      continue;

    return indent;
  }

  return -1;
}

/*
Searches backward to find the beginning of a block with '{' at
bracket 0
*/
static int find_prev_block_level0(TGetCharContext *pstTextContext)
{
  int r;
  int save_line;
  int line;

  save_line = pstTextContext->nLine;

  /* find end of previous block */
  line = pstTextContext->nLine - 1;
  do
  {
    if (line < 0)
    {
      r = 0;
      goto _exit;
    }
    if (!set_pos(pstTextContext, line--, 0))
    {
      r = 0;
      goto _exit;
    }
    get_char(pstTextContext);
  }
  while (pstTextContext->nPrevLineBracketLevel == 0);

  if (find_opening_bracket(pstTextContext))
    r = 1;
  else
    r = 0;

_exit:
  if (!r)
    if (set_pos(pstTextContext, save_line, 0))
      get_char(pstTextContext);

  return r;
}

/*
Searches forward to find the beginning of a block with '{' at
bracket 0
*/
static int find_next_block_level0(TGetCharContext *pstTextContext)
{
  int r;
  int save_line;
  int line;

  save_line = pstTextContext->nLine;

  /* find beginning of next block */
  line = pstTextContext->nLine + 1;
  do
  {
    if (!set_pos(pstTextContext, line++, 0))
    {
      r = 0;
      goto _exit;
    }
    get_char(pstTextContext);
  }
  while (pstTextContext->nPrevLineBracketLevel == 0);
  r = 1;

_exit:
  if (!r)
    if (set_pos(pstTextContext, save_line, 0))
      get_char(pstTextContext);

  return r;
}

/*
---c_lang_calc_indent
This function calculates the desired indentation at a line position.
Returns -1 if no suitable indent is calculated.
It will only work if the cursor is at pos 0 of a line, this shows an
intention to paste and benefit from an auto-indent operation!
*/
int c_lang_calc_indent(TEditInterf *pEditInterf)
{
  int nCol;
  int nRow;
  int nPos;
  int nNumLines;
  TGetCharContext stTextContext;
  int nIndent;
  int new_indent;
  int block_start_indent;
  int open_bracket_line;

  nIndent = -1;  /* Assume no good indent */

  /* setup the navigation range -- entire file */
  nNumLines = pEditInterf->pfnGetNumLines(pEditInterf);
  setup(0, 0, nNumLines, nNumLines - 1,
    &stTextContext, &pEditInterf->stNavInterf);
  stTextContext.pEditInterf = pEditInterf;

  pEditInterf->pfnGetCurPos(&nCol, &nRow, &nPos, pEditInterf);
  if (nPos != 0)  /* User doesn't intend autoindent? */
    return -1;

  nIndent = find_line_indent(&stTextContext, nRow);

  /* if we are at a line with some text, paste at the same indent */
  if (nIndent != -1 && stTextContext.CurChar != '}')
    return nIndent;

  /* Detect the indent of the current block */
  if (!find_opening_bracket(&stTextContext))
    return -1;

  open_bracket_line = stTextContext.nLine;
  new_indent = find_indent_downward(&stTextContext);
  if (   new_indent != -1
      && stTextContext.CurChar == '}'  /* check for empty {} */
      && open_bracket_line == stTextContext.nLine - 1)
  {
    if (!set_pos(&stTextContext, open_bracket_line - 1, 0))  /* From pos 0 of cur line */
      return -1;
    get_char(&stTextContext);
    block_start_indent = new_indent;
    if (!find_prev_block_level0(&stTextContext))
    {
      if (!set_pos(&stTextContext, open_bracket_line, 0))
        return -1;
      get_char(&stTextContext);
      if (!find_closing_bracket(&stTextContext))
        return -1;
      if (!find_next_block_level0(&stTextContext))
        return -1;
    }
    new_indent = find_indent_downward(&stTextContext);
    if (new_indent != -1)
      new_indent += block_start_indent;
  }

  return new_indent;
}

/*
---c_lang_examine_key
This function monitors for auto-format characters like { or } or
similar
The function will produce the appropriate highligh regions.
*/
int c_lang_examine_key(char ascii_char, TEditInterf *pEditInterf)
{
  int nCol;
  int nRow;
  int nPos;
  int nNumLines;
  TGetCharContext stTextContext;
  TSynHRegion bracket_regions[6];
  TBracketBlockTooltip stTooltip;
  char sDummyTooltipBuf[256];
  int b_bracket_on_empty_ln;

  if (ascii_char == '}')
  {
    stTooltip.pDestBuf = sDummyTooltipBuf;
    stTooltip.BufSize = sizeof(sDummyTooltipBuf);
    memset(bracket_regions, 0, sizeof(bracket_regions));

    nNumLines = pEditInterf->pfnGetNumLines(pEditInterf);
    setup(0, 0, nNumLines, nNumLines - 1,
      &stTextContext, &pEditInterf->stNavInterf);

    pEditInterf->pfnGetCurPos(&nCol, &nRow, &nPos, pEditInterf);
    stTextContext.nNextPos = 0;
    stTextContext.nNextLine = nRow;

    get_char(&stTextContext);

    /*
    First, verify that the line is empty (or blanks only)
    */
    while (1)
    {
      /* move a char forward */
      if (!goto_next_char(&stTextContext))
        break;
      get_char(&stTextContext);
      if (!is_blank(stTextContext.CurChar))
        goto _exit;
    }

    /*
    Find if there is opening pair -- '{' and the block operator
    of the pair (if, for, while, etc.)
    */
    if (!find_block_start(&stTextContext, bracket_regions, 6))
      goto _exit;
    /* Find if the bracket is the first non-blank character */
    b_bracket_on_empty_ln = 1;
    if (!set_pos(&stTextContext, bracket_regions[0].nLine, bracket_regions[0].nPos))
      goto _exit;
    get_char(&stTextContext);
    while (1)
    {
      /* move a char forward */
      if (!goto_prev_char(&stTextContext))
        break;
      get_char(&stTextContext);
      if (!is_blank(stTextContext.CurChar))
      {
        b_bracket_on_empty_ln = 0;
        break;
      }
    }
    if (b_bracket_on_empty_ln)
    {
      /* use opening bracket pos */
      nCol = pEditInterf->pfnGetTabPos(bracket_regions[0].nPos,
        bracket_regions[0].nLine, pEditInterf);
    }
    else  /* bracket is not the first non-blank */
    {
      find_block_operator(&stTextContext, bracket_regions, 6,
        &stTooltip, pEditInterf);

      if (bracket_regions[2].nPos > 0)  /* block operator detected? */
        nCol = pEditInterf->pfnGetTabPos(bracket_regions[2].nPos,
          bracket_regions[2].nLine, pEditInterf);  /* use operator pos */
      else  /* use opening bracket pos */
        nCol = pEditInterf->pfnGetTabPos(bracket_regions[0].nPos,
          bracket_regions[0].nLine, pEditInterf);
    }
    pEditInterf->pfnGotoColRow(nCol, nRow, pEditInterf);
_exit:
    return 1;  /* Yes, try to highligh areas (bracket areas) */
  }

  return 0;  /* No, there are no areas to highligh */
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
