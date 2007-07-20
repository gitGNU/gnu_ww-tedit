true = 1
false = 0

# This is the build environment
env = Environment()

#Uncomment this to see what the environment contains
#print env.Dump();Exit(0)

print 'Platform: %s' % env['PLATFORM']

# .txt => .xml asciidoc builder
env['BUILDERS']['Txt2Xml'] = \
    Builder( action = 'asciidoc -b docbook -d manpage -o $TARGET $SOURCE',
             suffix = '.xml',
             src_suffix = '.txt' )

# .xml  => manpage asciidoc builder
env['BUILDERS']['Xml2Man'] = \
    Builder(action = 'xmlto -m doc/callouts.xsl man -o ${TARGET.dir} $SOURCE',
            src_suffix = '.xml' )


# yacc and lex builders
#
# Define yacc and lex builders which defer to the builtin CFile yacc/lex
# builder if yacc/lex has been detected on the platform, and otherwise
# use pregenerated files (with '.gen' extension)

import os.path

def yacc_present ( target, source, **dict ):
    return env.CFile( target, source, **dict )

def yacc_missing ( target, source, **dict ):
    header = os.path.splitext( target )[0] + '.h'
    # Note that each builder, including Command, returns a list
    return \
        env.Command( target, target+'.gen', Copy('$TARGET','$SOURCE')) + \
        env.Command( header, header+'.gen', Copy('$TARGET','$SOURCE'))

def lex_present ( target, source, **dict ):
    return env.CFile( target, source, **dict )

def lex_missing ( target, source, **dict ):
    return env.Command( target, target+".gen", Copy('$TARGET','$SOURCE') )

if "yacc" in env['TOOLS']:
    yacc = yacc_present
else:
    print "yacc/bison not detected. Using pregenerated files"
    yacc = yacc_missing

if "lex" in env['TOOLS']:
    lex = lex_present
else:
    print "lex/flex not detected. Using pregenerated files"
    lex = lex_missing


# Prepare default option values
if env['PLATFORM'] == 'win32':
    env['WIN32'] = true
else:
    env['WIN32'] = false

# Command line arguments
opts = Options('options.cache')
opts.AddOptions(
  BoolOption('_DEBUG', 'debug build', true),
  BoolOption('_NON_TEXT', 'uses GUI to emulate console functions', false),
  BoolOption('WIN32', 'Win32 target', env['WIN32']),
  ('DESTDIR', 'Optional staging directory (prepend with # if local)', ''),
  ('PREFIX', 'Installation directory', '/usr/local'),
)

opts.Update(env)
Help(opts.GenerateHelpText(env))

#
# Configuration
#

# Force Win32 target under Win32
if env['PLATFORM'] == 'win32':
    env['WIN32'] = true
elif env['PLATFORM'] == 'cygwin':
    if env['WIN32']: # Win32 cross-compilation under cygwin ?
        env.Append( CCFLAGS='-mno-cygwin' )
        env.Append( LINKFLAGS='-mno-cygwin' )
else: # Posix platform
    if env['WIN32']: # Win32 cross-compilation ? Use the ming32 cross-compilation tool
        env.Tool('crossmingw', ['../scons-tools'] )

# Remember if we are running under msvc
msvc = "cl" == env['CC']


# Check if the compiler supports Win32-style EH. If so, updates the compiler
# defines with -DHAVE_WIN32EH
def CheckEH ( context ):
    context.Message('Checking for Win32-style EH... ')
    result = context.TryCompile( extension=".c", text="""
        void ehfunc ( void )
        {
          __try { (void)0; } __except(0) { (void)0; }
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

# Libraries
if not msvc:
    env.Append( LIBS=['m'] )

# Compile flags
if not msvc:
    env.Append( CCFLAGS='-Wall' )
else:
    env.Append( CCFLAGS='-W3' )

if env['_DEBUG']:
    env.Append( CCFLAGS='-D_DEBUG -DHEAP_DBG=2' )
    if not msvc:
        env.Append( CCFLAGS='-g' )
    else:
        env.Append( CCFLAGS='-GZ -Zi -MDd' )
else:
    env.Append( CCFLAGS='-DNDEBUG -O1' )
    if msvc:
        env.Append( CCFLAGS='-MD' )

# Different config for the GUI or text console build
if env['WIN32']:
    env.Append( CCFLAGS='-DWIN32 -DUSE_WINDOWS')
    env.Append( LIBS=['gdi32', 'user32'] )
    if env['_NON_TEXT']:
        env.Append( CCFLAGS='-D_NON_TEXT' )
        if not msvc:
	        env.Append( LINKFLAGS='-mwindows' )
        else:
	        env.Append( LINKFLAGS='-subsystem:windows' )
    else:
        env.Append( CCFLAGS='-DDARK_BPAL -D_CONSOLE' )
        if msvc:
	        env.Append( LINKFLAGS='-subsystem:console' )
else: # Posix
  if env['_NON_TEXT']:
      env.Append( CCFLAGS='-D_NON_TEXT' )
      env.Append( CPPPATH=['/usr/X11R6/include', '/usr/include/freetype2'] )
      # use freetype-config --libs and xft-config --libs to get lib names
      env.Append( LIBPATH=['/usr/X11R6/lib'] )
      env.Append( LIBS=['Xft','Xrender','X11'] )
      env.Append( LIBS=['fontconfig','pthread','expat','freetype','z'] )
  else:
      env.Append( CCFLAGS='-DDARK_BPAL' )
      env.Append( LIBS=['curses'] )

# Configuration, but only if we are not cleaning
if not env.GetOption('clean'):
    conf = Configure(env, custom_tests={'CheckEH':CheckEH} )

    # Check for dbghelp
    conf.env.Append( CPPPATH='' ) # Make sure CPPPATH is non-empty
    save_cpppath = conf.env['CPPPATH']
    conf.env.Append( CPPPATH='#dbghelp' )
    if conf.CheckLibWithHeader( 'dbghelp/dbghelp', ['windows.h','dbghelp.h'], 'c', autoadd=true ):
        conf.env.Append( CCFLAGS='-DHAVE_LIBDBGHELP' )
    else:
        conf.env.Replace( CPPPATH=save_cpppath )

    if conf.CheckCHeader( 'crtdbg.h' ):
        conf.env.Append( CCFLAGS='-DHAVE_CRTDBG_H' )

    conf.CheckEH()
    env = conf.Finish()

# Cache the updated options
opts.Save('options.cache', env)

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

