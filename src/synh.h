/*

File: synh.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 2nd November, 1998
Descrition:
  Syntax highlighting interface descriptions.
  This file is an independent (to no other header files)
  interface for syntax highlighting functions to set color attributes into
  the output buffer of a text line.

*/

#ifndef SYNH_H
#define SYNH_H

typedef enum Color_t
{
  COLOR_EOF,
  COLOR_TEXT,
  COLOR_BLOCK,
  COLOR_NUMBER,
  COLOR_COMMENT,
  COLOR_RESWORD,
  COLOR_REGISTER,
  COLOR_INSTRUCTION,
  COLOR_STRING,
  COLOR_PP,
  COLOR_OPERATOR,
  COLOR_SFR,
  COLOR_BPAIR,
  COLOR_TOOLTIP,
  COLOR_BLOCKCURSOR,
  COLOR_BOOKMARK
} Color_t;

/*
Interface used by the syntax highlighting function to apply colors
*/
typedef struct SynHInterf
{
  void (*pfnPutAttr)(int attr, int nRegionStart, int nRegionEnd,
    struct SynHInterf *pContext);

  /*
  Fields used by PutAttr()
  */
  void *pLine;
  int nWrtEdge;
  int nWinWidth;
  void *pOutputBuf;
} TSynHInterf;

/*
Function name location in a file
*/
typedef struct FunctionName
{
  int nLine;
  int nNamePos;
  int nNameLen;
  int nFuncEndLine;
  int nFuncEndPos;
} TFunctionName;

typedef struct SynHRegion
{
  int nLine;
  int nPos;
  int nLen;
} TSynHRegion;

typedef struct BracketBlockTooltip
{
  char *pDestBuf;
  int BufSize;
  int bIsTop;
  int NumLines;
  int bCursorIsOverOpenBracket;
} TBracketBlockTooltip;

/*
Interface used by the syntax highlighting function to scan lines of
the file. This is used for the function name highlighting.
*/
typedef struct LinesNavInterf
{
  char *(*pfnGetLine)(int Line, struct LinesNavInterf *pContext);
  int (*pfnGetLineLen)(int Line, struct LinesNavInterf *pContext);
  unsigned long (*pfnGetLineStatus)(int Line, struct LinesNavInterf *pContext);

  /*
  Fields set by the supplier side of the interface
  */
  void *pFile;
} TLinesNavInterf;

/*
Interface supplied by TFileView down to wline() to be called back
in order to apply special extra colors on top of the already applied
syntax colors.
*/
typedef struct ExtraColorInterf
{
  /* wline() calls pfnApplyExtraColors() which on its hand uses the
  supplied TSynHIntef to alter the attributes of a line */
  void (*pfnApplyExtraColors)(const void *pFile, int bNewPage, int nLine,
    int nTopLine, int nWinHeight,
    TSynHInterf *pSynhInterf, struct ExtraColorInterf *pCtx);

  void *pExtraCtx;  /* this is supplied when called from TFileView.HndlEvent() */
  void *pReserved1;
  void *pReserved2;
  int  nReserved1;
} TExtraColorInterf;

/*
Editor methods
*/
typedef struct EditInterf
{
  void (*pfnGetCurPos)(int *pnCol, int *pnRow, int *pnPos, struct EditInterf *pInterf);
  void (*pfnGotoColRow)(int nCol, int nRow, struct EditInterf *pInterf);
  void (*pfnGotoPosRow)(int nPos, int nRow, struct EditInterf *pInterf);
  int (*pfnGetTabPos)(int nPos, int nRow, struct EditInterf *pInterf);
  void (*pfnGetCurPagePos)(int *pnTopLine, int *pnWrtEdge, int *pnNumVisibleLines, struct EditInterf *pInterf);
  int (*pfnGetNumLines)(struct EditInterf *pInterf);
  void (*pfnGetText)(int nLine, int nPos, int nLen, char *pDest, struct EditInterf *pInterf);
  void *pFile;

  TLinesNavInterf stNavInterf;
} TEditInterf;

/*
TODO: typedef all document related call-back function
(copy from c_syntax/c_syntax2.h)
*/
typedef int (*TExamineKeyProc)(char ascii_char, TEditInterf *pEditInterf);
typedef int (*TIsOverBracket)(TSynHRegion BracketRegions[], int nMaxEntries,
  TBracketBlockTooltip *pBlockTooltip,
  TEditInterf *pEditInterf);
typedef int (*TCalcIndentProc)(TEditInterf *pEditInterf);

#endif /* ifndef SYNH_H */

