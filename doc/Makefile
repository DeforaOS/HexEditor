TARGETS	= $(OBJDIR)hexeditor.1 $(OBJDIR)hexeditor.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)hexeditor.1: hexeditor.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)hexeditor.1"

$(OBJDIR)hexeditor.html: hexeditor.css.xml hexeditor.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)hexeditor.html"

clean:
	$(RM) -- $(hexeditor.1_OBJS) $(hexeditor.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "hexeditor.1"
	./docbook.sh -c -P "$(PREFIX)" -- "hexeditor.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)hexeditor.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)hexeditor.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "hexeditor.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "hexeditor.html"

.PHONY: all clean distclean install uninstall
