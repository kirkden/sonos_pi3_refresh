VERSION = 0.9.26

PREFIX = /usr
DESTDIR = 

CC = g++

OPTS = -O3 -ffast-math -funroll-loops -Wall -fPIC -DPIC 
#OPTS = -fPIC -DPIC -g -DDEBUG 

_LDFLAGS = -shared 
STRIP = strip

-include defines.make

CFLAGS += $(OPTS) $(_CFLAGS)
LDFLAGS += $(_LDFLAGS) $(CFLAGS)

PLUG = caps

SOURCES = $(wildcard *.cc) $(wildcard dsp/*.cc)
OBJECTS	= $(SOURCES:.cc=.o) 
HEADERS = $(wildcard *.h) $(wildcard dsp/*.h) $(wildcard util/*.h) $(wildcard dsp/tonestack/*.h)

DEST = $(PREFIX)/lib/ladspa
RDFDEST = $(PREFIX)/share/ladspa/rdf

# targets following -------------------------------------------------------------

all: depend $(PLUG).so tags

run: all
	@#python bin/enhance_dp_wsop.py
	@#python -i bin/rack.py White AutoFilter cream.Audio.Meter Pan
	@#python -i bin/rack.py White AutoFilter Pan
	@#python -i bin/rack.py Click Plate
	@#python -i bin/rack.py PhaserII 
	@#~/cream/gdb-python html/graph.py Eq4p,a.f=100,a.gain=30.png
	@#~/cream/gdb-python bin/eqtest.py
	@#~/cream/gdb-python bin/fractalstest.py
	@#python bin/sinsweep.py
	@#python -i ~/reve/bin/noisegate.py
	@#rack.py Noisegate AmpVI Plate
	@#python ~/reve/bin/hum.py
	@#python bin/cabtest.py
	@#python bin/spicetest.py
	@#python bin/test.py
	@#cd ~/reve && python bin/cabmake.py
	@python ~/bin/rack.py

rdf: $(PLUG).rdf
$(PLUG).rdf: all tools/make-rdf.py
	python tools/make-rdf.py > $(PLUG).rdf

$(PLUG).so: $(OBJECTS)
	$(CC) $(ARCH) $(LDFLAGS) -o $@ $(OBJECTS)

.cc.s: 
	$(CC) $(ARCH) $(CFLAGS) -S $<

.cc.o: depend 
	$(CC) $(ARCH) $(CFLAGS) -o $@ -c $<

tags: $(SOURCES) $(HEADERS)
	@-if [ -x /usr/bin/ctags ]; then ctags $(SOURCES) $(HEADERS) >/dev/null 2>&1 ; fi

install: all
	@$(STRIP) $(PLUG).so > /dev/null
	install -d $(DESTDIR)$(DEST)
	install -m 644 $(PLUG).so $(DESTDIR)$(DEST)
	install -d $(DESTDIR)$(RDFDEST)
	install -m 644 $(PLUG).rdf $(DESTDIR)$(RDFDEST)

fake-install: all
	-rm $(DESTDIR)$(DEST)/$(PLUG).so
	ln -s `pwd`/$(PLUG).so $(DESTDIR)$(DEST)/$(PLUG).so
	-rm $(DESTDIR)$(RDFDEST)/$(PLUG).rdf
	ln -s `pwd`/$(PLUG).rdf $(DESTDIR)$(RDFDEST)/$(PLUG).rdf

rdf-install:
	install -d $(DESTDIR)$(RDFDEST)
	install -m 644 $(PLUG).rdf $(DESTDIR)$(RDFDEST)

uninstall:
	-rm $(DESTDIR)$(DEST)/$(PLUG).so
	-rm $(DESTDIR)$(RDFDEST)/$(PLUG).rdf

clean:
	rm -f $(OBJECTS) $(PLUG).so *.s depend

version.h:
	@VERSION=$(VERSION) python tools/make-version.h.py

dist: all $(PLUG).rdf version.h
	tools/make-dist.py caps $(VERSION) $(CFLAGS)

depend: $(SOURCES) $(HEADERS)
	$(CC) -MM $(CFLAGS) $(DEFINES) $(SOURCES) > depend

-include depend
