#
# note: ps2pdf produces better results

APPNAME := notes
ADDMODS := str.o nc-readstr.o nc-core.o nc-keyb.o nc-view.o nc-list.o notes.o list.o
INSTALL := /usr/local/bin
MANDIR1 := /usr/local/share/man/man1
MANDIR5 := /usr/local/share/man/man5
CFLAGS  := -Os -Wall -Wformat=0 -D_GNU_SOURCE
LDLIBS  := -lncurses
#M2RFLAGS := -z
M2RFLAGS := 

all: $(APPNAME)

clean:
	-rm -f *.o $(APPNAME) $(APPNAME).man $(APPNAME).1.gz $(APPNAME)rc.man $(APPNAME)rc.5.gz > /dev/null

$(APPMODS): %.o: %.c

$(APPNAME): $(ADDMODS)

$(APPNAME).man: $(APPNAME).md
	md2roff $(M2RFLAGS) $(APPNAME).md > $(APPNAME).man

$(APPNAME).1.gz: $(APPNAME).md
	md2roff $(M2RFLAGS) $(APPNAME).md > $(APPNAME).1
	gzip -f $(APPNAME).1

$(APPNAME)rc.man: $(APPNAME)rc.md
	md2roff $(M2RFLAGS) $(APPNAME)rc.md > $(APPNAME)rc.man

$(APPNAME)rc.5.gz: $(APPNAME)rc.md
	md2roff $(M2RFLAGS) $(APPNAME)rc.md > $(APPNAME)rc.5
	gzip -f $(APPNAME)rc.5

html: $(APPNAME).man
	md2roff -q $(APPNAME).md > $(APPNAME).man
	groff $(APPNAME).man -Thtml -man > $(APPNAME).html

pdf: $(APPNAME).md
	md2roff -q $(APPNAME).md > $(APPNAME).man
	groff $(APPNAME).man -Tpdf -man -dPDF.EXPORT=1 -dLABEL.REFS=1 -P -e > $(APPNAME).pdf

install: $(APPNAME) $(APPNAME).1.gz $(APPNAME)rc.5.gz
	sudo install -m 755 -o root -g root -s $(APPNAME) $(INSTALL)
	sudo install -m 644 -o root -g root $(APPNAME).1.gz $(MANDIR1)/
	sudo install -m 644 -o root -g root $(APPNAME)rc.5.gz $(MANDIR5)/

uninstall:
	sudo rm $(INSTALL)/$(APPNAME) $(MANDIR1)/$(APPNAME).1.gz $(MANDIR5)/$(APPNAME)rc.5.gz

# utilities
nc-colors: nc-colors.c
	gcc nc-colors.c -o nc-colors -lncurses

nc-getch: nc-getch.c
	gcc nc-getch.c -o nc-getch -lncurses

