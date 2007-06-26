ifndef DESTDIR
DEST=/usr/local/bin
else
DEST=$(DESTDIR)/usr/bin
endif

all: console gui

console:
	make -f makefile.build _DEBUG=1

gui:
	make -f makefile.build _DEBUG=1 _NON_TEXT=1

clean: clean-console clean-gui

clean-console:
	make -f makefile.build _DEBUG=1 clean
	rm -rf dbg_con

clean-gui:
	make -f makefile.build _DEBUG=1 _NON_TEXT=1 clean
	rm -rf dbg_g

install: all
	install -d $(DEST)
	install extra/ww $(DEST)/
	install dbg_g/ww $(DEST)/ww_g
	install dbg_con/ww $(DEST)/ww_con

