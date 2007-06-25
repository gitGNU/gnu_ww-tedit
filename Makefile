# Makefile

# Layout:
# -- The source files are in src, src/fpcalc, src/perl_re,
# we put these paths in $(VPATH);
# -- We need to build the .c->.h dependency files (.d files)
# first, we put the .d files in the output directory $(OUTPATH).
# We include all the *.d files as rules to suplement the *.o->*.c
# dependency chain; If a *.d file is regenerated the make will
# remake the whole Makefile which guarantees us consistency;
# -- We compile *.c to *.o in the $(OUTPATH) directory as well;

# How to maintain and extend this Makefile
# -- if new source directories are added you need to extend 
# the VPATH definition;
# -- if new source files are added you need to extend the MODULES
# definition;
# -- if new source file type are added you need to add an implicit
# rules at the end of the Makefile;

# Rules:
# -- generate all intermediate files into the $(OUTPATH) directory

# Defines:
#  _DEBUG=1 -- debug build
#  _NON_TEXT=1 -- uses GUI to emulate console functions
#  _STATIC=1 -- link staticly all external libraries

.SILENT:

VPATH:=src:src/fpcalc:src/perl_re:src/c_syntax:src/py_syntax

# For UNIX platforms gcc is not the default compiler
CC=gcc

ifdef _DEBUG
OUTPATH:=dbg
else
OUTPATH:=rel
endif

ifdef _STATIC
OUTPATH:=$(OUTPATH)_static
endif

ifdef _NON_TEXT
OUTPATH:=$(OUTPATH)_g
else
OUTPATH:=$(OUTPATH)_con
endif

# This is the main target
all: $(OUTPATH)/ww

# Files from src
MODULES:=block2.o \
	block.o \
	blockcmd.o \
	bookm.o \
	bookmcmd.o \
	calccmd.o \
	cmd.o \
	contain.o \
	ctxhelp.o \
	debug.o \
	defs.o \
	diag.o \
	doctype.o \
	edit.o \
	edinterf.o \
	editcmd.o \
	enterln.o \
	file2.o \
	file.o \
	filecmd.o \
	filemenu.o \
	filenav.o \
	findf.o \
	fnavcmd.o \
	fview.o \
	heapg.o \
	helpcmd.o \
	history.o \
	hypertvw.o \
	infordr.o \
	ini2.o \
	ini.o \
	keynames.o \
	keyset.o \
	ksetcmd.o \
	l1def.o \
	l1opt.o \
	l2disp.o \
	main2.o \
	memory.o \
	menu.o \
	menudat.o \
	mru.o \
	nav.o \
	navcmd.o \
	options.o \
	parslogs.o \
	pageheap.o \
	palette.o \
	path.o \
	precomp.o \
	search.o \
	searchf.o \
	searcmd.o \
	smalledt.o \
	strlwr.o \
	tblocks.o \
	umenu.o \
	undo.o \
	undocmd.o \
	wl.o \
	wline.o \
	wrkspace.o
    
# Files from src/fpcalc
MODULES += calcfunc.o \
        calc_tab.o \
        lexfcalc.o

# Files from src/perl_re    
MODULES += get.o \
	maketables.o \
	pcre.o \
	study.o

MODULES += c_syntax.o \
        synh.o \
	funcs.o

MODULES += py_syntax.o \
        py_synh.o

# *Files* for the GUI or text console build
ifdef _NON_TEXT
MODULES += x_kbd.o \
	x_scr.o \
	xclip.o
else
MODULES += l_kbd.o \
	l_scr.o
endif

# Options for compilation
CFLAGS:=-Wall -Isrc/perl_re -Isrc/fpcalc -Isrc/c_syntax -Isrc/py_syntax -DYY_NO_UNPUT

ifdef _STATIC
CFLAGS += -static
endif

ifdef _DEBUG
CFLAGS += -D_DEBUG -g -DHEAP_DBG=2
endif

# *Defines* for the GUI or text console build
ifdef _NON_TEXT
CFLAGS += -D_NON_TEXT -I/usr/X11R6/include -I/usr/include/freetype2
else
CFLAGS += -DDARK_BPAL
endif

# Uncomment the line below to debug the link process
#LFLAGS += -Xlinker --verbose

# *Libraries* for the GUI or text console build
ifdef _NON_TEXT
# use freetype-config --libs and xft-config --libs to get lib names
LFLAGS += -L/usr/lib -L/usr/X11R6/lib -lXft -lXrender -lX11
LFLAGS += -lfontconfig -lpthread -lexpat -lfreetype -lz
else
LFLAGS += -lcurses
endif

ifdef CYGWIN_ROOT
LFLAGS += -Xlinker --enable-auto-import
endif

# OBJ contains all the output files names prepended with $(OUTPATH)
# to effectively specifiy output directory for each of the obj files
OBJ:=$(patsubst %.o,$(OUTPATH)/%.o,$(MODULES))

# "ww" binary depends on all the obj modules that we expect
# to find built in the $(OUTPATH) directory
$(OUTPATH)/ww: $(OUTPATH) $(OBJ)
	@echo [ link $(OUTPATH)/ww ]
	$(CC) $(CFLAGS) $(OBJ) $(LFLAGS) -lm -o $(OUTPATH)/ww

$(OUTPATH):
	echo here

# every obj module in the $(OUTPATH) directory depends
# on a .c file. The correspondent .c file will be searched
# in the directories specified in $(VPATH)
# $@ stands for the full obj module file name + extention (target file)
# $< stands for the c module file name + extention (source file)
$(OUTPATH)/%.o: %.c
	@echo [ $< ]
	$(CC) $(CFLAGS) -c -o $@ $<

# DEPS are .d files for each of the .c files that show
# the set of the header files on which this .c file depends 
DEPS:=$(patsubst %.o,%.d,$(OBJ))

# for each *.c file we need to generate a *.d file and this file
# is then included in this Makefile
# The '-e' flag to the shell makes it exit immediately if the '$(CC)'
# command fails (normally it exits with the status of the last command in
# the pipeline, which will prevent make to notice the exit code of '$(CC)')
# The 'sed' command translates "main.o: main.c defs.h" to
# "$(OUTPATH)/main.o $(OUTPATH)/main.d: main.c defs.h"
# The last shell command in the pipeline removes the .d file if it is empty.
$(OUTPATH)/%.d: %.c
	@echo [ dep $< ]
	mkdir -p $(OUTPATH)
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/$(OUTPATH)\/\1.o $(OUTPATH)\/$*\.d : /g' > $@; \
	[ -s $@ ] || rm -f $@

# Avoid including '.d' files during 'clean' rules, so
# make won't create them only to immediately remove them again
ifneq ($(MAKECMDGOALS),clean)
# the '-' character keeps make from issuing a warning prior the first .d files
# are generated.
-include $(DEPS)
endif

.PHONY: clean
clean:
	-rm -f -v $(DEPS)
	-rm -f -v $(OBJ)
	-rm -f -v $(OUTPATH)/ww

