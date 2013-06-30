TARGETS	= hexeditor.1 hexeditor.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

hexeditor.1: hexeditor.xml
	./docbook.sh -P "$(PREFIX)" -- "hexeditor.1"

hexeditor.html: hexeditor.xml
	./docbook.sh -P "$(PREFIX)" -- "hexeditor.html"

clean:
	$(RM) -- $(hexeditor.1_OBJS) $(hexeditor.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "hexeditor.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "hexeditor.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "hexeditor.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "hexeditor.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "hexeditor.html"

.PHONY: all clean distclean install uninstall
