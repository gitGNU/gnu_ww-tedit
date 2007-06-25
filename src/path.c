/*

File: path.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Filename and filepath manipulation routines.
  #ifdef separated releases for MSDOS, WINDOWS and UNIX.

*/

#include "global.h"
#include "maxpath.h"
#include "wlimits.h"
#include "findf.h"
#include "l1def.h"
#include "path.h"

#include <time.h>

#ifdef MSDOS
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <fcntl.h>
#include <libc/dosio.h>
#include <unistd.h>  /* for getcwd */
#endif

#ifdef MSDOS
/* ************************************************************************
   Function: _turnback_slashes
   Description:
     turns all '/' to '\\'
*/
void _turnback_slashes(char *file)
{
  char *p;

  for (p = file; *p; p++)
    if (*p == '/')
      *p = '\\';
}
#endif

void GetCurDir(char *buff)
{
  ASSERT(buff != NULL);

  #ifdef WIN32
  if (GetCurrentDirectory(_MAX_PATH, buff) == 0)
    buff[0] = '\0';
  #else
  if (getcwd(buff, _MAX_PATH) == NULL)
    buff[0] = '\0';
  #ifdef MSDOS
  _turnback_slashes(buff);
  #endif
  #endif
}

#ifdef MSDOS
static unsigned use_lfn;

static char *
__get_current_directory(char *out, int drive_number)
{
  __dpmi_regs r;
  char tmpbuf[FILENAME_MAX];

  memset(&r, 0, sizeof(r));
  if(use_lfn)
    r.x.ax = 0x7147;
  else
    r.h.ah = 0x47;
  r.h.dl = drive_number + 1;
  r.x.si = __tb_offset;
  r.x.ds = __tb_segment;
  __dpmi_int(0x21, &r);

  if (r.x.flags & 1)
  {
    errno = r.x.ax;
    return out;
  }
  else
  {
    dosmemget(__tb, sizeof(tmpbuf), tmpbuf);
    strcpy(out+1,tmpbuf);

    /* Root path, don't insert "/", it'll be added later */
    if (*(out + 1) != '\0')
      *out = '/';
    else
      *out = '\0';
    return out + strlen(out);
  }
}

__inline__ static int
is_slash(int c)
{
  return c == '/' || c == '\\';
}

__inline__ static int
is_term(int c)
{
  return c == '/' || c == '\\' || c == '\0';
}

/*

   Takes as input an arbitrary path.  Fixes up the path by:
   1. Removing consecutive slashes
   2. Removing trailing slashes
   3. Making the path absolute if it wasn't already
   4. Removing "." in the path
   5. Removing ".." entries in the path (and the directory above them)
   6. Adding a drive specification if one wasn't there
   7. Converting all slashes to '/'

   Q: What is the difference with the original DJGPP version?
   A: Added support for network paths in format \\workstation\device\...

 */
void
_fixpath(const char *in, char *out)
{
  int		drive_number;
  const char	*ip = in;
  char		*op = out;
  int		preserve_case = _preserve_fncase();
  char		*name_start;
  BOOLEAN bNetPath = FALSE;

  use_lfn = _USE_LFN;
  drive_number = 0;  /*	A: */

  if ((*ip == '\\' && *(ip + 1) == '\\')
    ||
      (*ip == '/' && *(ip + 1) == '/'))  /* Network path (starts with \\ or //) */
  {
    *out++ = *ip;
    *out++ = *(ip + 1);  /* Skip adding drive */
    op = out;
    bNetPath = TRUE;
  }
  else
  {
    /* Add drive specification to output string */
    if (((*ip >= 'a' && *ip <= 'z') ||
	 (*ip >= 'A' && *ip <= 'Z'))
	&& (*(ip + 1) == ':'))
    {
      if (*ip >= 'a' && *ip <= 'z')
      {
	drive_number = *ip - 'a';
	*op++ = *ip++;
      }
      else
      {
	drive_number = *ip - 'A';
	if (*ip <= 'Z')
	  *op++ = drive_number + 'a';
	else
	  *op++ = *ip;
	++ip;
      }
      *op++ = *ip++;
    }
    else
    {
      __dpmi_regs r;
      r.h.ah = 0x19;
      __dpmi_int(0x21, &r);
      drive_number = r.h.al;
      *op++ = drive_number + (drive_number < 26 ? 'a' : 'A');
      *op++ = ':';
    }
  }

  /* Convert relative path to absolute */
  if (!is_slash(*ip))
    op = __get_current_directory(op, drive_number);

  /* Step through the input path */
  while (*ip)
  {
    /* Skip input slashes */
    if (is_slash(*ip))
    {
      ip++;
      continue;
    }

    /* Skip "." and output nothing */
    if (*ip == '.' && is_term(*(ip + 1)))
    {
      ip++;
      continue;
    }

    /* Skip ".." and remove previous output directory */
    if (*ip == '.' && *(ip + 1) == '.' && is_term(*(ip + 2)))
    {
      ip += 2;
      /* Don't back up over drive spec */
      if (op > out + 2)
	/* This requires "/" to follow drive spec */
	while (!is_slash(*--op));
      continue;
    }

    /* Copy path component from in to out */
    if (!bNetPath)
      *op++ = '/';
    bNetPath = FALSE;
    while (!is_term(*ip)) *op++ = *ip++;
  }

  /* If root directory, insert trailing slash */
  if (op == out + 2) *op++ = '/';

  /* Null terminate the output */
  *op = '\0';

  /* switch FOO\BAR to foo/bar, downcase where appropriate */
  for (op = out + 3, name_start = op - 1; *name_start; op++)
  {
    char long_name[FILENAME_MAX], short_name[13];

    if (*op == '\\')
      *op = '/';
    if (!preserve_case && (*op == '/' || *op == '\0'))
    {
      memcpy(long_name, name_start+1, op - name_start - 1);
      long_name[op - name_start - 1] = '\0';
      if (!strcmp(_lfn_gen_short_fname(long_name, short_name), long_name))
      {
	while (++name_start < op)
	  if (*name_start >= 'A' && *name_start <= 'Z')
	    *name_start += 'a' - 'A';
      }
      else
	name_start = op;
    }
    else if (*op == '\0')
      break;
  }
}
#endif

#ifdef UNIX
/* 
Here now the very usefull function _fixpath() from DJGPP's
libc 'fixpath.c'
I have modified it to be used on unix systems (like linux).

Peter: Modified version is from turbo vision sources.
*/

/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

__inline__ static int
is_slash(int c)
{
  return c == '/';
}

__inline__ static int
is_term(int c)
{
  return c == '/' || c == '\0';
}

/* Takes as input an arbitrary path.  Fixes up the path by:
   1. Removing consecutive slashes
   2. Removing trailing slashes
   3. Making the path absolute if it wasn't already
   4. Removing "." in the path
   5. Removing ".." entries in the path (and the directory above them)
 */
void
_fixpath(const char *in, char *out)
{
  const char	*ip = in;
  char		*op = out;

  /* Convert relative path to absolute */
  if (!is_slash(*ip))
  {
    /* getcurdir(0,op); */
    GetCurDir(op);
    op += strlen(op);
  }

  /* Step through the input path */
  while (*ip)
  {
    /* Skip input slashes */
    if (is_slash(*ip))
    {
      ip++;
      continue;
    }

    /* Skip "." and output nothing */
    if (*ip == '.' && is_term(*(ip + 1)))
    {
      ip++;
      continue;
    }

    /* Skip ".." and remove previous output directory */
    if (*ip == '.' && *(ip + 1) == '.' && is_term(*(ip + 2)))
    {
      ip += 2;
      /* Don't back up over root '/' */
      if (op > out )
      /* This requires "/" to follow drive spec */
	while (!is_slash(*--op));
      continue;
    }

    /* Copy path component from in to out */
    *op++ = '/';
    while (!is_term(*ip)) *op++ = *ip++;
  }

  /* If root directory, insert trailing slash */
  if (op == out) *op++ = '/';

  /* Null terminate the output */
  *op = '\0';

}

/*
End of modified code from DJGPP's libc 'fixpath.c'
*/
#endif

/* ************************************************************************
   Function: CheckForSpecial
   Description:
     Checks whether in a file name there's .. or . path specifiers.
   Returns:
     TRUE -- the path contains .. or .
     FALSE -- the path is free of .. or . directories.
*/
static BOOLEAN CheckForSpecial(char *file)
{
  char *p;

  ASSERT(file != NULL);
  ASSERT(strlen(file) < _MAX_PATH);

  /* Check for . and .. */
  p = strchr(file, '.');
  if (p == NULL)
    return (FALSE);
  ++p;
  if (*p != '.' && *p != '\\')  /* .. and .\ */
    return (FALSE);
  return (TRUE);
}

/* ************************************************************************
   Function: GetFullPath
   Description:
     Expands the path of file
   TODO:
     Make a cache set of files necessary for expanding.
*/
void GetFullPath(char *file)
{
  char temp[_MAX_PATH];
  #ifdef _WIN32
  char *pFilePart;
  #endif
  #ifdef UNIX
  char *pTrailingSlash;
  BOOLEAN bTrailingSlash;
  #endif

  ASSERT(file != NULL);
  ASSERT(strlen(file) < _MAX_PATH);

  if (file[0] == '\0')
    goto _l1;

  /*
  Make some fast predictions
  */
  if (file[0] == '\\' && file[1] == '\\')
  {
    if (!CheckForSpecial(file))
    {
      /* This is network path and is always full */
      #ifdef MSDOS
      _turnback_slashes(file);
      #endif
      /*TRACE1("*GetFullPath(%s)\n", file);*/
      return;
    }
  }
  if (file[1] == ':' && file[2] == '\\')
  {
    /* The name contains root dir and drive */
    if (!CheckForSpecial(file))
    {
      /* This is network path and is always full */
      #ifdef MSDOS
      _turnback_slashes(file);
      #endif
      /*TRACE1("*GetFullPath(%s)\n", file);*/
      return;
    }
  }

  /*TRACE1("GetFullPath(%s)\n", file);*/

_l1:
  #ifdef MSDOS
  _fixpath(file, temp);
  _turnback_slashes(temp);
  #endif
  #ifdef WIN32
  if (file[0] == '\0')
  {
    file[0] = '.';
    file[1] = '\0';  /* file = "." */
  }
  GetFullPathName(file, _MAX_PATH, temp, &pFilePart);
  #endif
  #ifdef UNIX
  /* Preserve a trailing slash */
  pTrailingSlash = strchr(file, '\0');
  bTrailingSlash = FALSE;
  if (*(pTrailingSlash - 1) == '/')
    bTrailingSlash = TRUE;
  _fixpath(file, temp);
  if (bTrailingSlash)
    AddTrailingSlash(temp);
  #endif
  strcpy(file, temp);
}

/* ************************************************************************
   Function: ResetDestBuf
   Description:
     DEBUG ONLY
*/
static void ResetDestBuf(char *psDestBuf)
{
#ifdef _DEBUG
  memset(psDestBuf, 0x42, _MAX_PATH);  /* will possibly catch an error */
#endif
}

/* ************************************************************************
   Function: ShrinkPath
   Description:
     In dest you receive a path that excludes the current dir path
     Example: if Path = c:\bin\edit\t1.exe, CurDir = c:\bin
     then On exit: Dest = edit\t1.exe (c:\bin\ is omitted from the path)
     If the dest path exceeds the width the path will be cut down
     Example: if Dest = c:\bin\edit\t1.exe, width = 17
     On exit: Dest = c:\bin...\t1.exe
     If width == 0 no check will be made

     bIsFile: Passed to FSplit(), look at FSplit() for details.
   CAUTION:
     psDest should point at an area where at least _MAX_PATH bytes
     should be available.
*/
void ShrinkPath(const char *psPath, char *psDest, int nWidth, BOOLEAN bIsFile)
{
  char fullpath[_MAX_PATH];
  char partial_fullpath[_MAX_PATH];
  char curdir[_MAX_PATH];
  char name[_MAX_PATH];
  char *p;
  char *pLastSlash;
  char *pDest;
  int l;
  int l_full_path;
  int nCutPoint;

  #define SlashChar PATH_SLASH_CHAR

  ASSERT(psPath != NULL);
  ASSERT(psDest != NULL);
  ASSERT(nWidth	>= 0);
  ASSERT(strlen(psPath) < _MAX_PATH);

  ResetDestBuf(psDest);

  /*
  Make fast assumptions.
  */
  if (strchr(psPath, SlashChar) == NULL)
  {
    /* No path? This is exactly the final result! */
    strcpy(psDest, psPath);
    /*TRACE1("*ShrinkPath(%s)\n", psPath);*/  /* Indicate fast execution */
    return;
  }

  /*TRACE1("ShrinkPath(%s)\n", psPath);*/

  psDest[0] = '\0';

  /* To get the real file name */
  FSplit(psPath, fullpath, name, sAllMask, TRUE, bIsFile);
  strcat(fullpath, name);

  GetCurDir(curdir);

  /*
  Check whether the current path matches the start of the full path
  */
  strcpy(partial_fullpath, fullpath);
  l = strlen(curdir);
  l_full_path = strlen(fullpath);
  /* Cut the full path to the length of the current path  */
  /* Compose special partial_fullpath that contains       */
  /* if possible only as much as strlen(curdir) from      */
  /* fullpath. Advance partial_fullpath to the first      */
  /* slashcharacter, this is necessary in cases like      */
  /* curdir = c:\temp and the file we get from c:\tempbla */
  /* then we need to get the whole c:\tempbla instead of  */
  /* only the c:\temp portion into partua_fullpath        */
  p = &partial_fullpath[l_full_path];  /* assume: nothing to cut */
  if (l_full_path > l)
    p = &partial_fullpath[l];
  while (*p)  /* will exit immediately if there is nothing to cut */
  {
    if (*p == SlashChar)
      break;
    ++p;
  }
  *p = '\0'; /* cut here */
  /* now compare the so prepared partial_fullpath */
  if (filestrcmp(partial_fullpath, curdir) == 0)
  {
    pDest = psDest;
    if (l == 3)  /* The difference starts from root */
    {
      /* Check whether that this is a name in root dir 
      (there is only one slash -- of the root dir!) */
      if (strchr(&fullpath[l], SlashChar) != NULL)
        *pDest++ = SlashChar;  /* If not only the root, add the root */
      --l;  /* start copy from [l + 1] to include first char! */
    }
    if ((int)strlen(fullpath) == l)
      *pDest = '\0';  /* Path component is empty */
    else
      strcpy(pDest, &fullpath[l + 1]);  /* Only after curpath component */
  }
  else  /* There's no any coincidence -- copy the whole path */
    strcpy(psDest, fullpath);

  /* Check whether psDest fits in the width */
  if (nWidth == 0)
    return;

  l = strlen(psDest);
  if (l < nWidth)
    return;

  /* Extract the name only */
  pLastSlash = strrchr(psDest, SlashChar);
  p = strrchr(psDest, ':');

  if (p > pLastSlash)
    pLastSlash = p;

  if (p == NULL && pLastSlash == NULL)
  {
    psDest[nWidth] = '\0';  /* Slash not found cut anyway */
    return;
  }

  strcpy(name, pLastSlash);
  l = strlen(name);
  nCutPoint = nWidth - l - 3;
  if (nCutPoint < 0)
    nCutPoint = 0;
  psDest[nCutPoint] = '\0';
  strcat(psDest, "...");
  strcat(psDest, name);
  psDest[nWidth] = '\0';  /* Ensure against very long file names */
}

/* ************************************************************************
   Function: AddTrailingSlash
   Description:
     Adds a slash to a path.
*/
void AddTrailingSlash(char *psPath)
{
  char *p;

  /*TRACE1("AddTrailingSlash(%s)\n", psPath);*/

  ASSERT(psPath != NULL);

  p = strchr(psPath, (char)NULL);
  ASSERT(p != NULL);
  ASSERT(p - psPath < _MAX_PATH);

  if (p - psPath > 0)
  {
    /* Check whether there's a trailing slash */
    if (*(p - 1) == PATH_SLASH_CHAR)
      return;
  }  /* else the path is empty -- make a string with a single slash only */
  /* Add a trailing slash */
  *(p) = PATH_SLASH_CHAR;
  *(p + 1) = '\0';
}

/* ************************************************************************
   Function: HasWild
   Description:
     TRUE for a	file name presenting a wildcard.
*/
BOOLEAN HasWild(char *psFileName)
{
  /*TRACE1("HasWild(%s)\n", psFileName);*/

  if (strchr(psFileName, '*') != NULL)
    return (TRUE);
  if (strchr(psFileName, '?') != NULL)
    return (TRUE);
  return (FALSE);
}

/* ************************************************************************
   Function: countslashes
   Description:
*/
static int countslashes(const char *dirname)
{
  const char *p;
  int n;

  n = 0;
  p = dirname;

  while (*p)
    if (*p++ == PATH_SLASH_CHAR)
      ++n;

  return n;
}

/* ************************************************************************
   Function: FSplit
   Description:
     Splits a name to path and filename. Returns psDefaultExt if
     no file name present in psSPath.
     psDPath will alsways have trailing slash.
     bNoError: split the path ignoring the io errors.
     IO error can occure only when detecting whether the name
     presents directory. when bNoError is set and io error occures
     it is assumed that	psSPath is a file name.
     bIsFile: when set to TRUE, function is directed not to check
     whether this is file or directory and supposes this is always
     a file. Q:Why is this paramater necessary? A: In order to reduce
     the time for this function to return result when operating
     on network paths. If the psSPath content is for shure a
     file then setting bIsFile to TRUE will spare a stat() call that is
     relatively slow on network paths.
   Returns:
     FALSE for io error -- check errno for error code.
*/
BOOLEAN FSplit(const char *psSPath, char *psDPath, char *psDFile, const char *psDefaultExt,
  BOOLEAN bNoError, BOOLEAN bIsFile)
{
  char sFullPath[_MAX_PATH];
  char *pLastSlash;
  char sPath[_MAX_PATH];
  char sFile[_MAX_PATH];
  struct stat statbuf;
  BOOLEAN bNetPath;
  BOOLEAN bRootDirOnly;

  /*TRACE1("FSplit(%s)\n", psSPath);*/

  ASSERT(psSPath != NULL);
  ASSERT(psDPath != NULL);
  ASSERT(psDefaultExt != NULL);

  strcpy(sFullPath, psSPath);
  GetFullPath(sFullPath);
  bNetPath = FALSE;
  if (sFullPath[0] == '\\' && sFullPath[1] == '\\')
    bNetPath = TRUE;

  /* Search for the last '\' (GetFullPath will ensure at leas one '\\') */
  pLastSlash = strrchr(sFullPath, PATH_SLASH_CHAR);
  if (pLastSlash == NULL)
  {
    /* this is a MS-DOS format path (including a drive) */
    pLastSlash = strrchr(sFullPath, '\\');
  }
  /* Check the case when there's a network path and entering root dir */
  ASSERT(pLastSlash != NULL);

  bRootDirOnly = FALSE;
  if (bNetPath)
  {
    if (countslashes(&sFullPath[2]) == 2)  /* only the separation for server_name and the root */
      bRootDirOnly = TRUE;
  }

  /* Split sFullPath into sPath and sFile */
  if (*(pLastSlash + 1)	== '\0')
  {
    /* No file */
    nofile:
    strcpy(sPath, sFullPath);
    strcpy(sFile, psDefaultExt);
    goto exitpoint;
  }

  if (HasWild(sFullPath))
  {
    /* Wildcards indicate there's a file name */
    split:
    strcpy(sPath, sFullPath);
    if (bRootDirOnly) /* and bNetPath */
      sPath[(int)(pLastSlash - sFullPath + 1)] = '\0';  /* Preserve the root dir slash */
    else
      sPath[(int)(pLastSlash - sFullPath)] = '\0';

    strcpy(sFile, pLastSlash + 1);
    goto exitpoint;
  }

  if (!bIsFile)
  {
    if (stat(sFullPath, &statbuf) != 0)
    {
      if (bNoError)
	goto split;
      return (FALSE);
    }

    if ((statbuf.st_mode & S_IFDIR) != 0)
      goto nofile;  /* This is a directory, act with the default ext */

    /* Get the exact name */
    GetFileParameters(sFullPath, pLastSlash + 1, NULL, NULL);
  }

  goto split;

  exitpoint:
  strcpy(psDPath, sPath);
  AddTrailingSlash(psDPath);
  strcpy(psDFile, sFile);
  return (TRUE);
}

/* ************************************************************************
   Function: FileExists
   Description:
   TRUE -- file exists
   FALSE -- file doesn't exists
*/
BOOLEAN FileExists(const char *sFileName)
{
  struct stat statbuf;

  /*TRACE1("FileExists(%s)\n", sFileName);*/

  return (stat(sFileName, &statbuf) == 0);
}

/* ************************************************************************
   Function: GetFileParameters
   Description:
     Extracts all the file specific information.
     If some of the information is not necessary invoke
     the function with the corespondent pointers NULL.
*/
BOOLEAN GetFileParameters(const char *sFileName, char *sExactName,
  TTime *pTime, int *Size)
{
  struct FF_DAT *ffdat;
  struct findfilestruct ff;
  struct tm *time;

  /*TRACE1("GetFileParameters(%s)\n", sFileName);*/

  ffdat = find_open(sFileName, FFIND_FILES);
  if (ffdat == NULL)
    return (FALSE);

  if (find_file(ffdat, &ff) != 0)
  {
    find_close(ffdat);
    return (FALSE);
  }

  if (sExactName != NULL)
  {
    strcpy(sExactName, ff.filename);
  }

  if (pTime != NULL)
  {
    time = localtime(&ff.st.st_mtime);
    pTime->year = time->tm_year;
    pTime->month = time->tm_mon;
    pTime->day = time->tm_mday;
    pTime->hour = time->tm_hour;
    pTime->min = time->tm_min;
    pTime->sec = time->tm_sec & 0xfe;  /* 0xfe to mask the odd seconds */
  }

  if (Size != NULL)
  {
    *Size = ff.st.st_size;
  }

  find_close(ffdat);
  return (TRUE);
}

/* ************************************************************************
   Function: IsValidFileName
   Description:
     Checks whether this is a valid file name or the file has
     invalid path. The function works on non-existing files as well.
*/
BOOLEAN	IsValidFileName(char *sFileName)
{
  struct stat statbuf;
  #ifdef MSDOS
  struct DOSERROR error;
  #endif

  /*TRACE1("IsValidFileName(%s)\n", sFileName);*/
  if (stat(sFileName, &statbuf) != 0)
  {
    /* Error acquiring stat info */
    #ifdef MSDOS
    dosexterr(&error);
    if (error.exterror == 3)  /* Path not found */
      return FALSE;
    #endif
    #ifdef WINDOWS
    if (_doserrno == 3)  /* Path not found */
      return FALSE;
    #endif
    #ifdef UNIX
    /* TODO: Try to find the error code on UNIX machines
    that shows invalid path for a particular filename */
    #endif
  }
  return TRUE;
}

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

