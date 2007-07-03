PROJECT(ww)

# WIN32: use dbghelp library if installed in src/dbghelp
IF (EXISTS src/dbghelp/dbghelp.h)
  SET(dbghelp_include src/dbghelp)
  SET(dbghelp_lib     ${CMAKE_CURRENT_SOURCE_DIR}/src/dbghelp/dbghelp.lib)
  SET(dbghelp_def     "/D USE_DBGHELPLIB=1")
ELSE (EXISTS src/dbghelp/dbghelp.h)
  SET(dbghelp_include)
  SET(dbghelp_lib)
  SET(dbghelp_def "/D USE_DBGHELPLIB=0")
ENDIF (EXISTS src/dbghelp/dbghelp.h)

SET(disp_lib_src src/disp/win_g_disp.c)
SET(disp_lib_include src/disp)
SET_SOURCE_FILES_PROPERTIES(${disp_lib_src}
                            PROPERTIES
                            COMPILE_FLAGS "-D D_ASSERT=_assert_sc")

SET(perl_re_lib_src src/perl_re/get.c
                    src/perl_re/maketables.c
                    src/perl_re/pcre.c
                    src/perl_re/study.c
                    src/py_syntax/py_synh.c
                    src/py_syntax/py_syntax.c)
SET(perl_re_lib_include src/perl_re)
SET_SOURCE_FILES_PROPERTIES(${perl_re_lib_src}
                            PROPERTIES
                            COMPILE_FLAGS "-D STATIC")

SET(ww_main_src src/assertsc.c
                src/block.c
                src/block2.c
                src/blockcmd.c
                src/bookm.c
                src/bookmcmd.c
                src/calccmd.c
                src/cmd.c
                src/contain.c
                src/ctxhelp.c
                src/debug.c
                src/defs.c
                src/diag.c
                src/dirent.c
                src/doctype.c
                src/edinterf.c
                src/edit.c
                src/editcmd.c
                src/enterln.c
                src/file.c
                src/file2.c
                src/filecmd.c
                src/filemenu.c
                src/filenav.c
                src/findf.c
                src/fnavcmd.c
                src/fview.c
                src/heapg.c
                src/helpcmd.c
                src/history.c
                src/hypertvw.c
                src/infordr.c
                src/ini.c
                src/ini2.c
                src/keynames.c
                src/keyset.c
                src/ksetcmd.c
                src/l1def.c
                src/l1opt.c
                src/l2disp.c
                src/main2.c
                src/memory.c
                src/menu.c
                src/menudat.c
                src/mru.c
                src/nav.c
                src/navcmd.c
                src/options.c
                src/pageheap.c
                src/palette.c
                src/parslogs.c
                src/path.c
                src/precomp.c
                src/search.c
                src/searchf.c
                src/searcmd.c
                src/smalledt.c
                src/tblocks.c
                src/txtf_rend.c
                src/umenu.c
                src/undo.c
                src/undocmd.c
                src/winclip.c
                src/wline.c
                src/wrkspace.c
                src/ww.c
                src/fpcalc/CALC_TAB.C
                src/fpcalc/CALCFUNC.C
                src/fpcalc/LEXFCALC.C
                src/c_syntax/c_syntax.c
                src/c_syntax/funcs.c
                src/c_syntax/synh.c)

SET_SOURCE_FILES_PROPERTIES(${ww_main_src}
                            PROPERTIES COMPILE_FLAGS
                            "/D HEAP_DBG=1 /D _NON_TEXT ${dbghelp_def}")

SET(ww_src ${disp_lib_src} ${perl_re_lib_src} ${ww_main_src})

INCLUDE_DIRECTORIES(${perl_re_lib_include}
                    ${disp_lib_include}
                    ${dbghelp_include}
                    src/c_syntax
                    src/py_syntax)

ADD_EXECUTABLE(ww WIN32 ${ww_src})

TARGET_LINK_LIBRARIES(ww ${dbghelp_lib})

SET_TARGET_PROPERTIES(ww PROPERTIES
                      LINK_FLAGS_DEBUG "/incremental:no"
                      LINK_FLAGS_RELEASE "/incremental:no")

