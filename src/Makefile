TARGETS	= hexeditor
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

hexeditor_OBJS = hexeditor.o window.o main.o
hexeditor_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
hexeditor_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

hexeditor: $(hexeditor_OBJS)
	$(CC) -o hexeditor $(hexeditor_OBJS) $(hexeditor_LDFLAGS)

hexeditor.o: hexeditor.c hexeditor.h ../config.h
	$(CC) $(hexeditor_CFLAGS) -c hexeditor.c

window.o: window.c hexeditor.h window.h
	$(CC) $(hexeditor_CFLAGS) -c window.c

main.o: main.c window.h
	$(CC) $(hexeditor_CFLAGS) -c main.c

clean:
	$(RM) -- $(hexeditor_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 hexeditor $(DESTDIR)$(BINDIR)/hexeditor

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/hexeditor

.PHONY: all clean distclean install uninstall
