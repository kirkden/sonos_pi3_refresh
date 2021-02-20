DESTDIR = /mnt/mmcblk0p2/tce/optional
SRCDIR = /mnt/mmcblk0p2/tce/sonos_pi3/sonos_pi3_crossover/src
SRCDIR = /mnt/OneDrive_ckirkby/Source/Raspi/sonos_pi3/sonos_pi3_crossover/src
PLUGDIR = $(SRCDIR)/plugins

all: ladspa-sdk rt-plugins caps-plugins dsp-plugin

#
# ladspa need to be installed first
#
ladspa-sdk:
	(cd $(SRCDIR)/ladspa_sdk_1.15/src;make)

rt-plugins:  
	(cd $(SRCDIR)/rt-plugins-0.0.6/build;cmake ..)
	(cd $(SRCDIR)/rt-plugins-0.0.6/build;make)

caps-plugins:
	(cd $(SRCDIR)/caps-0.9.26;python3.6 ./configure.py)
	(cd $(SRCDIR)/caps-0.9.26;make)

dsp-plugin:
	(cd $(SRCDIR)/dsp;./configure)
	(cd $(SRCDIR)/dsp;make -f GNUmakefile)


.PHONY: tcz
tgz:
	mkdir -p $(PLUGDIR)/usr/bin
	mkdir -p $(PLUGDIR)/usr/include
	mkdir -p $(PLUGDIR)/usr/lib
	mkdir -p $(PLUGDIR)/usr/lib/ladspa
	mkdir -p $(PLUGDIR)/usr/doc
	mkdir -p $(PLUGDIR)/usr/share/ladspa
	
	install -m755 rt-plugins-0.0.6/build/src/*.so $(PLUGDIR)/usr/lib/ladspa/

	install -m755 ladspa_sdk_1.15/bin/* $(PLUGDIR)/usr/bin/
	install -m755 ladspa_sdk_1.15/src/ladspa.h $(PLUGDIR)/usr/include/
	install -m755 ladspa_sdk_1.15/plugins/*  $(PLUGDIR)/usr/lib/ladspa/
	install -m644 ladspa_sdk_1.15/doc/* $(PLUGDIR)/usr/doc/

	install -m755 dsp/dsp $(PLUGDIR)/usr/bin/
	install -m755 dsp/ladspa_dsp.so $(PLUGDIR)/usr/lib/ladspa/
	install -m644 dsp/dsp.1 $(PLUGDIR)/usr/doc/

	install -m755 caps-0.9.26/caps.so $(PLUGDIR)/usr/lib/ladspa/
	install -m644 caps-0.9.26/caps.rdf $(PLUGDIR)/usr/share/ladspa/

	mksquashfs $(PLUGDIR)/ sonos-pi3-plugins.tcz -noappend

.PHONY: install
install:	
	install -m644 sonos-pi3-plugins.tcz $(DESTDIR)/
	grep -qxF 'sonos-pi3-plugins.tcz' /mnt/mmcblk0p2/tce/onboot.lst || echo 'sonos-pi3-plugins.tcz' >> /mnt/mmcblk0p2/tce/onboot.lst
	pcp bu
	echo "Added sonos-pi3-plugins.tcz to /mnt/mmcblk0p2/tce/onboot.lst"

.PHONY: clean
clean:
	rm -f $(SRCDIR)/sonos-pi3-plugins.tcz 
	rm -rf $(PLUGDIR)
	rm -rf $(SRCDIR)/rt-plugins-0.0.6/build/*
	(cd $(SRCDIR)/caps-0.9.26;make -f Makefile clean)
	(cd $(SRCDIR)/ladspa_sdk_1.15/src;make clean)
	(cd $(SRCDIR)/dsp;make -f GNUmakefile clean)
