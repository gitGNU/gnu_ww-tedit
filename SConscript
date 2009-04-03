Import('env', 'yacc', 'lex', 'msvc')

true = 1
false = 0

#
# Sources
#
env.Append( CPPPATH=['src/perl_re','src/fpcalc','src/c_syntax','src/py_syntax'] )

modules = []

if env['WIN32']:
    modules += env.Object( Split("""
          src/ww.c
          src/winclip.c
    """))
    # dirent.c: only needed when compiling with msvc
    if msvc:
        modules += env.Object( Split("""
          src/dirent.c
        """))

    if env['_NON_TEXT']:
        modules += env.Object( Split("""
	        src/gui_kbd.c
	        src/gui_scr.c
        """))
    else:
        modules += env.Object( Split("""
	        src/w_kbd.c
	        src/w_scr.c
        """))
else: # Posix
    modules += env.Object( Split("""
                src/wl.c
    """) )
    if env['_NON_TEXT']:
        modules += env.Object( Split("""
	        src/x_kbd.c
	        src/x_scr.c
	        src/xclip.c
        """))
    else:
        modules += env.Object( Split("""
	        src/l_kbd.c
	        src/l_scr.c
        """))

# Files from src
modules += env.Object( Split("""
	src/assertsc.c
	src/block2.c
	src/block.c
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
	src/doctype.c
	src/edit.c
	src/edinterf.c
	src/editcmd.c
	src/enterln.c
	src/file2.c
	src/file.c
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
	src/ini2.c
	src/ini.c
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
	src/parslogs.c
	src/pageheap.c
	src/palette.c
	src/path.c
	src/precomp.c
	src/search.c
	src/searchf.c
	src/searcmd.c
	src/smalledt.c
	src/strlwr.c
	src/tblocks.c
	src/umenu.c
	src/undo.c
	src/undocmd.c
	src/wline.c
	src/wrkspace.c
"""))

# Files from src/perl_re
modules += env.Object( CCFLAGS="${CCFLAGS} -DSTATIC", target=Split("""
	src/perl_re/get.c
	src/perl_re/maketables.c
	src/perl_re/pcre.c
	src/perl_re/study.c
"""))

# src/fpcalc
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=[
    'src/fpcalc/calcfunc.c',
    yacc( 'src/fpcalc/calc_tab.c', 'src/fpcalc/calc.y', YACCFLAGS='--name-prefix=fcalc_ -d' )[0],
    lex( 'src/fpcalc/lexfcalc.c', 'src/fpcalc/scan.l', LEXFLAGS='-Pfcalc_' )[0]
])


# C syntax highlighting
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=[
	'src/c_syntax/c_syntax.c',
	'src/c_syntax/funcs.c',
    lex( 'src/c_syntax/synh.c', 'src/c_syntax/synh.l' )[0]
])

# Python syntax highlighting
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=[
    'src/py_syntax/py_syntax.c',
    lex( 'src/py_syntax/py_synh.c', 'src/py_syntax/py_synh.l', LEXFLAGS='-Ppy' )[0]
])

prog = env.Program( 'ww', modules )
env.Alias( 'prog', prog )

# Documentation
man = env.Xml2Man( 'doc/ww.1', env.Txt2Xml( 'doc/ww.1' ) )
env.Alias( 'man', man )

# All
all = env.Alias( 'all', [prog,man] )
Default( all )
#
# Prepare are our installation paths:
# We need to use absolute paths because the path may start with "#" (for )
#
env['BINDIR']=env.Dir('$DESTDIR$PREFIX/bin').abspath
env['SYSCONFDIR']=env.Dir('$DESTDIR/etc').abspath
env['MANDIR']=env.Dir('$DESTDIR$PREFIX/share/man').abspath
env['MAN1DIR']='$MANDIR/man1'

install_bin = env.Command( 'install_bin', prog,  [
  'mkdir -p $BINDIR',
  'install -t $BINDIR $SOURCE',
])
env.AlwaysBuild(install_bin)
env.Alias( 'install_bin', install_bin )


install_man = env.Command( 'install_man', man, [
    'install -d -m755 $MAN1DIR',
    'install -m644 $SOURCE $MAN1DIR'
])
env.AlwaysBuild( install_man )
env.Alias( 'install_man', install_man )

env.Alias( 'install', [install_bin, install_man] )
