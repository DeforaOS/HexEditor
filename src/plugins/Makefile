TARGETS	= template.so
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags gtk+-2.0 libSystem`
CFLAGS	= -Wall -g -O2 -fPIC
LDFLAGSF= `pkg-config --libs gtk+-2.0 libSystem`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

template_OBJS = template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

template.so: $(template_OBJS)
	$(CCSHARED) -o template.so $(template_OBJS) $(template_LDFLAGS)

template.o: template.c
	$(CC) $(template_CFLAGS) -c template.c

clean:
	$(RM) -- $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
