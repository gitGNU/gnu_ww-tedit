BISON  = bison
FLEX   = flex
CFLAGS = -DTEST_CALC  

!ifdef YYDEBUG
CFLAGS = $(CFLAGS) -DYYDEBUG=1
!endif

!ifdef DEBUG
CFLAGS = $(CFLAGS) -D_DEBUG -Zi -MDd
!else
CFLAGS = $(CFLAGS) -DNDEBUG -MD
!endif

calc.exe: calc_tab.obj lexfcalc.obj calcfunc.obj
  $(CC) $(CFLAGS) $** -Fe$@

calc_tab.c calc_tab.h: calc.y
  $(BISON) --name-prefix=fcalc_ -d calc.y

lexfcalc.c: scan.l
  $(FLEX) -Pfcalc_ scan.l

scan.l: calc_tab.h

