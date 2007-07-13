true = 1
false = 0

# Command line arguments
opts = Options('options.cache')
opts.AddOptions(
  BoolOption('_DEBUG', 'debug build', true),
  BoolOption('_NON_TEXT', 'uses GUI to emulate console functions', false),
  BoolOption('WIN32', 'Win32 target', false),
  PathOption('PREFIX', 'Installation directory (prepend with # if local)', '/usr/local', PathOption.PathIsDirCreate)
)

# Build environment
env = Environment( options = opts )
Help(opts.GenerateHelpText(env))

# Extract theoptions into variables for convenience
_DEBUG = env['_DEBUG']
_NON_TEXT = env['_NON_TEXT']
WIN32 = env['WIN32']
cross = false

#
# Configuration
#

if env['PLATFORM'] == 'win32':
  cross = false
  WIN32 = true
else: # Posix platform
  if WIN32: # Win32 cross-compilation ? Use the ming32 cross-compilation tool
    cross = True
    env.Tool('crossmingw', ['../scons-tools'] )

# Check if the compiler supports Win32-style EH. If so, updates the compiler
# defines with -DHAVE_WIN32EH
def CheckEH ( context ):
  context.Message('Checking for Win32-style EH... ')
  result = context.TryCompile( extension=".c", text="""
    void ehfunc ( void )
    {
      __try { (void)0; } __except { (void)0; }
    }
  """)
  context.Result(result)
  key = "HAVE_WIN32EH"
  if result:
    context.env.Append( CCFLAGS='-D'+key )
  if hasattr(context,'config_h'):
    if result:
      context.config_h += '#define %s\n' % key 
    else:
      context.config_h += '/* #undef %s */\n' % key
  return result

# Configuration, but only if we are not cleaning
if not env.GetOption('clean'): 
  # Different config for the GUI or text console build
  if WIN32:
    env.Append( CCFLAGS='-DWIN32 -DUSE_WINDOWS')
    env.Append( LIBS=['gdi32'] )
    if _NON_TEXT:
	env.Append( CCFLAGS='-D_NON_TEXT -mwindows' )
	env.Append( LIBS=['user32'] )
    else:
	env.Append( CCFLAGS='-DDARK_BPAL -D_CONSOLE' )
  else: # Posix
    if _NON_TEXT:
	env.Append( CCFLAGS='-D_NON_TEXT' )
	env.Append( CPPPATH=['/usr/X11R6/include', '/usr/include/freetype2'] )
	# use freetype-config --libs and xft-config --libs to get lib names
	env.Append( LIBS=['Xft','Xrender','X11'] )
	env.Append( LIBS=['fontconfig','pthread','expat','freetype','z'] )
    else:
	env.Append( CCFLAGS='-DDARK_BPAL' )
	env.Append( LIBS=['curses'] )

  conf = Configure(env, custom_tests={'CheckEH':CheckEH} )
  if conf.CheckLibWithHeader( 'dbghelp', 'dbghelp.h', 'c', autoadd=true ):
    conf.env.Append( CCFLAGS='-DHAVE_LIBDBGHELP' )
  if conf.CheckCHeader( 'crtdbg.h' ):
    conf.env.Append( CCFLAGS='-DHAVE_CRTDBG_H' )
  conf.CheckEH()
  env = conf.Finish()


env.Append( CCFLAGS='-Wall' )
env.Append( CPPPATH=['src/perl_re','src/fpcalc','src/c_syntax','src/py_syntax'] )
env.Append( LIBS=['m'] )

if _DEBUG:
  env.Append( CCFLAGS='-D_DEBUG -g -DHEAP_DBG=2' )
else:
  env.Append( CCFLAGS='-DNDEBUG -O1' )

opts.Save('options.cache', env)

#
# Sources
#
modules = []

if WIN32:
  modules += env.Object( Split("""
  	src/ww.c 
	src/winclip.c
  """))
  if _NON_TEXT:
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
  if _NON_TEXT:
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

# Files from src/fpcalc
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=Split("""
        src/fpcalc/calcfunc.c
        src/fpcalc/calc_tab.c
        src/fpcalc/lexfcalc.c
"""))

# Files from src/perl_re
modules += env.Object( CCFLAGS="${CCFLAGS} -DSTATIC", target=Split("""
	src/perl_re/get.c
	src/perl_re/maketables.c
	src/perl_re/pcre.c
	src/perl_re/study.c
"""))

# C syntax highlighting
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=Split("""
	src/c_syntax/c_syntax.c
        src/c_syntax/synh.c
	src/c_syntax/funcs.c
"""))

# Python syntax highlighting
modules += env.Object( CCFLAGS="${CCFLAGS} -DYY_NO_UNPUT", target=Split("""
        src/py_syntax/py_syntax.c
        src/py_syntax/py_synh.c
"""))

prog = env.Program( 'ww', modules )

# Here are our installation paths:
idir_prefix = '$PREFIX'
idir_lib    = '$PREFIX/lib'
idir_bin    = '$PREFIX/bin'
idir_inc    = '$PREFIX/include'
idir_data   = '$PREFIX/share'

env.Install( idir_bin, prog )
env.Alias( 'install', idir_prefix )

