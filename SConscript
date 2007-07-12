# Command line arguments
opts = Options()
opts.AddOptions(
  BoolOption('_DEBUG', 'debug build', 1),
  BoolOption('_NON_TEXT', 'uses GUI to emulate console functions', 0),
  PathOption('PREFIX', 'Installation directory (prepend with # if local)', '/usr/local', PathOption.PathIsDirCreate)
)

# Build environment
env = Environment( options = opts )
Help(opts.GenerateHelpText(env))

debug = env['_DEBUG']
non_text = env['_NON_TEXT']

# Program

# Files from src
modules = env.Object( Split("""
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
	src/wl.c
	src/wline.c
	src/wrkspace.c
"""))

# Files from src/fpcalc
modules += env.Object( Split("""
	src/fpcalc/calcfunc.c
        src/fpcalc/calc_tab.c
        src/fpcalc/lexfcalc.c
"""))

# Files from src/perl_re
modules += env.Object( Split("""
	src/perl_re/get.c
	src/perl_re/maketables.c
	src/perl_re/pcre.c
	src/perl_re/study.c
"""))

modules += env.Object( Split("""
	src/c_syntax/c_syntax.c
        src/c_syntax/synh.c
	src/c_syntax/funcs.c
"""))

modules += env.Object( Split("""
	src/py_syntax/py_syntax.c
        src/py_syntax/py_synh.c
"""))

# Deifferent config for the GUI or text console build
if non_text:
    modules += env.Object( Split("""
	    src/x_kbd.c
	    src/x_scr.c
	    src/xclip.c
    """))
    env.Append( CCFLAGS='-D_NON_TEXT' )
    env.Append( CPPPATH=['/usr/X11R6/include', '/usr/include/freetype2'] )
    # use freetype-config --libs and xft-config --libs to get lib names
    env.Append( LIBS=['Xft','Xrender','X11'] )
    env.Append( LIBS=['fontconfig','pthread','expat','freetype','z'] )
else:
    modules += env.Object( Split("""
	    src/l_kbd.c
	    src/l_scr.c
    """))
    env.Append( CCFLAGS='-DDARK_BPAL' )
    env.Append( LIBS=['curses'] )


env.Append( CCFLAGS='-Wall -DYY_NO_UNPUT' )
env.Append( CPPPATH=['src/perl_re','src/fpcalc','src/c_syntax','src/py_syntax'] )
env.Append( LIBS=['m'] )

if debug:
  env.Append( CCFLAGS='-D_DEBUG -g -DHEAP_DBG=2' )

prog = env.Program( 'ww', modules )

# Here are our installation paths:
idir_prefix = '$PREFIX'
idir_lib    = '$PREFIX/lib'
idir_bin    = '$PREFIX/bin'
idir_inc    = '$PREFIX/include'
idir_data   = '$PREFIX/share'

env.Install( idir_bin, prog )
env.Alias( 'install', idir_prefix )

