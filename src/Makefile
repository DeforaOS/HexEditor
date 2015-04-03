SUBDIRS	= plugins
TARGETS	= $(OBJDIR)hexeditor
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && \
		if [ -n "$(OBJDIR)" ]; then \
		([ -d "$(OBJDIR)$$i" ] || $(MKDIR) -- "$(OBJDIR)$$i") && \
		$(MAKE) OBJDIR="$(OBJDIR)$$i/"; \
		else $(MAKE); fi) || exit; done

hexeditor_OBJS = $(OBJDIR)hexeditor.o $(OBJDIR)window.o $(OBJDIR)main.o
hexeditor_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
hexeditor_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)hexeditor: $(hexeditor_OBJS)
	$(CC) -o $(OBJDIR)hexeditor $(hexeditor_OBJS) $(hexeditor_LDFLAGS)

$(OBJDIR)hexeditor.o: hexeditor.c hexeditor.h ../config.h
	$(CC) $(hexeditor_CFLAGS) -o $(OBJDIR)hexeditor.o -c hexeditor.c

$(OBJDIR)window.o: window.c hexeditor.h window.h
	$(CC) $(hexeditor_CFLAGS) -o $(OBJDIR)window.o -c window.c

$(OBJDIR)main.o: main.c window.h
	$(CC) $(hexeditor_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(hexeditor_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(hexeditor_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)hexeditor $(DESTDIR)$(BINDIR)/hexeditor

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/hexeditor

.PHONY: all subdirs clean distclean install uninstall
