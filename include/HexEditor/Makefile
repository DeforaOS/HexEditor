PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
INCLUDEDIR= $(PREFIX)/include


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)/Desktop/HexEditor
	$(INSTALL) -m 0644 plugin.h $(DESTDIR)$(INCLUDEDIR)/Desktop/HexEditor/plugin.h

uninstall:
	$(RM) -- $(DESTDIR)$(INCLUDEDIR)/Desktop/HexEditor/plugin.h

.PHONY: all clean distclean install uninstall
