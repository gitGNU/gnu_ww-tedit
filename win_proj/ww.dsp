# Microsoft Developer Studio Project File - Name="ww" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ww - Win32 GUIDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ww.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ww.mak" CFG="ww - Win32 GUIDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ww - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ww - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ww - Win32 GUIDebug" (based on "Win32 (x86) Console Application")
!MESSAGE "ww - Win32 GUIRelease" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ww - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /Yu"global.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386

!ELSEIF  "$(CFG)" == "ww - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /D HEAP_DBG=1 /FR /Yu"global.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../src/dbghelp/dbghelp.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "ww - Win32 GUIDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "GUIDebug"
# PROP BASE Intermediate_Dir "GUIDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "GUIDebug"
# PROP Intermediate_Dir "GUIDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /D HEAP_DBG=1 /FR /Yu"global.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /D "_NON_TEXT" /D HEAP_DBG=1 /FR /Yu"global.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../src/dbghelp/dbghelp.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /map
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../src/dbghelp/dbghelp.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ww - Win32 GUIRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "GUIRelease"
# PROP BASE Intermediate_Dir "GUIRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "GUIRelease"
# PROP Intermediate_Dir "GUIRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /Yu"global.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USE_WINDOWS" /D "_NON_TEXT" /Yu"global.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ww - Win32 Release"
# Name "ww - Win32 Debug"
# Name "ww - Win32 GUIDebug"
# Name "ww - Win32 GUIRelease"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "fpcalc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\fpcalc\CALC_TAB.C
# ADD CPP /W2
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\fpcalc\CALCFUNC.C
# ADD CPP /W2
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\fpcalc\LEXFCALC.C
# ADD CPP /W2
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "perl_re"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\perl_re\get.c
# ADD CPP /D "STATIC"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\perl_re\maketables.c
# ADD CPP /D "STATIC"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\perl_re\pcre.c
# ADD CPP /D "STATIC"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\perl_re\study.c
# ADD CPP /D "STATIC"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "c_syntax"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\c_syntax\c_syntax.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\c_syntax\funcs.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\c_syntax\synh.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "py_syntax"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\py_syntax\py_syntax.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\py_syntax\py_synh.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\assertsc.c
# ADD CPP /I "../src/dbghelp"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\block.c
# End Source File
# Begin Source File

SOURCE=..\src\block2.c
# End Source File
# Begin Source File

SOURCE=..\src\blockcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\bookm.c
# End Source File
# Begin Source File

SOURCE=..\src\bookmcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\calccmd.c
# End Source File
# Begin Source File

SOURCE=..\src\cmd.c
# End Source File
# Begin Source File

SOURCE=..\src\contain.c
# End Source File
# Begin Source File

SOURCE=..\src\ctxhelp.c
# End Source File
# Begin Source File

SOURCE=..\src\debug.c
# End Source File
# Begin Source File

SOURCE=..\src\defs.c
# End Source File
# Begin Source File

SOURCE=..\src\diag.c
# End Source File
# Begin Source File

SOURCE=..\src\dirent.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\doctype.c
# End Source File
# Begin Source File

SOURCE=..\src\edinterf.c
# End Source File
# Begin Source File

SOURCE=..\src\edit.c
# End Source File
# Begin Source File

SOURCE=..\src\editcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\enterln.c
# End Source File
# Begin Source File

SOURCE=..\src\file.c
# End Source File
# Begin Source File

SOURCE=..\src\file2.c
# End Source File
# Begin Source File

SOURCE=..\src\filecmd.c
# End Source File
# Begin Source File

SOURCE=..\src\filemenu.c
# End Source File
# Begin Source File

SOURCE=..\src\filenav.c
# End Source File
# Begin Source File

SOURCE=..\src\findf.c
# End Source File
# Begin Source File

SOURCE=..\src\fnavcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\fview.c
# End Source File
# Begin Source File

SOURCE=..\src\gui_kbd.c
# End Source File
# Begin Source File

SOURCE=..\src\gui_scr.c
# End Source File
# Begin Source File

SOURCE=..\src\heapg.c
# ADD CPP /I "../src/dbghelp"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\helpcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\history.c
# End Source File
# Begin Source File

SOURCE=..\src\hypertvw.c
# End Source File
# Begin Source File

SOURCE=..\src\infordr.c
# End Source File
# Begin Source File

SOURCE=..\src\ini.c
# End Source File
# Begin Source File

SOURCE=..\src\ini2.c
# End Source File
# Begin Source File

SOURCE=..\src\keynames.c
# End Source File
# Begin Source File

SOURCE=..\src\keyset.c
# End Source File
# Begin Source File

SOURCE=..\src\ksetcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\l1def.c
# End Source File
# Begin Source File

SOURCE=..\src\l1opt.c
# End Source File
# Begin Source File

SOURCE=..\src\l2disp.c
# End Source File
# Begin Source File

SOURCE=..\src\main2.c
# ADD CPP /I "../src/c_syntax"
# ADD CPP /I "../src/py_syntax"
# End Source File
# Begin Source File

SOURCE=..\src\memory.c
# End Source File
# Begin Source File

SOURCE=..\src\menu.c
# End Source File
# Begin Source File

SOURCE=..\src\menudat.c
# End Source File
# Begin Source File

SOURCE=..\src\mru.c
# End Source File
# Begin Source File

SOURCE=..\src\nav.c
# End Source File
# Begin Source File

SOURCE=..\src\navcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\options.c
# End Source File
# Begin Source File

SOURCE=..\src\pageheap.c
# End Source File
# Begin Source File

SOURCE=..\src\palette.c
# End Source File
# Begin Source File

SOURCE=..\src\parslogs.c
# End Source File
# Begin Source File

SOURCE=..\src\path.c
# End Source File
# Begin Source File

SOURCE=..\src\precomp.c
# ADD CPP /Yc"global.h"
# End Source File
# Begin Source File

SOURCE=..\src\search.c
# ADD CPP /I "../src/perl_re"
# End Source File
# Begin Source File

SOURCE=..\src\searchf.c
# End Source File
# Begin Source File

SOURCE=..\src\searcmd.c
# End Source File
# Begin Source File

SOURCE=..\src\smalledt.c
# End Source File
# Begin Source File

SOURCE=..\src\tblocks.c
# End Source File
# Begin Source File

SOURCE=..\src\umenu.c
# End Source File
# Begin Source File

SOURCE=..\src\undo.c
# End Source File
# Begin Source File

SOURCE=..\src\undocmd.c
# End Source File
# Begin Source File

SOURCE=..\src\w_kbd.c
# End Source File
# Begin Source File

SOURCE=..\src\w_scr.c
# End Source File
# Begin Source File

SOURCE=..\src\winclip.c
# End Source File
# Begin Source File

SOURCE=..\src\wline.c
# End Source File
# Begin Source File

SOURCE=..\src\wrkspace.c
# End Source File
# Begin Source File

SOURCE=..\src\ww.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
