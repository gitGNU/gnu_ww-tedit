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
opts = Variables('options.cache')
opts.AddVariables(
  BoolVariable('_DEBUG', 'debug build', true),
  BoolVariable('_NON_TEXT', 'uses GUI to emulate console functions', false),
  BoolVariable('WIN32', 'Win32 target', env['WIN32']),
  ('DESTDIR', 'Optional staging directory (prepend with # if local)', ''),
  ('PREFIX', 'Installation directory', '/usr/local'),
)

opts.Update(env)
Help(opts.GenerateHelpText(env))

if opts.UnknownVariables():
    print "*** Unknown variables:", opts.UnknownVariables().keys()
    Exit(1)

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
        env.Tool('crossmingw' )

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

outdir='build'

# Different config for the GUI or text console build
if env['WIN32']:
    env.Append( CCFLAGS='-DWIN32 -DUSE_WINDOWS')
    env.Append( LIBS=['gdi32', 'user32'] )
    if env['_NON_TEXT']:
        outdir = 'build/w32gui'
        env.Append( CCFLAGS='-D_NON_TEXT' )
        if not msvc:
                env.Append( LINKFLAGS='-mwindows' )
        else:
                env.Append( LINKFLAGS='-subsystem:windows' )
    else:
        outdir='build/w32con'
        env.Append( CCFLAGS='-DDARK_BPAL -D_CONSOLE' )
        if msvc:
                env.Append( LINKFLAGS='-subsystem:console' )
else: # Posix
  if env['_NON_TEXT']:
      outdir='build/x11'
      env.Append( CCFLAGS='-D_NON_TEXT' )
      env.ParseConfig( "xft-config --libs --cflags" )
  else:
      outdir='build/curses'
      env.Append( CCFLAGS='-DDARK_BPAL' )
      env.ParseConfig( "ncurses5-config --libs --cflags" )

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

Export( 'env', 'yacc', 'lex', 'msvc' )

SConscript('SConscript', variant_dir=outdir, duplicate=0)

