all:  dsp ladspa_dsp
install:  install_dsp install_ladspa_dsp install_manual
uninstall:  uninstall_dsp uninstall_ladspa_dsp uninstall_manual

PREFIX := /usr
BINDIR := /bin
LIBDIR := /lib
DATADIR := /share
MANDIR := /man
DSP_OBJ += 
DSP_CPP_OBJ += 
DSP_EXTRA_CFLAGS :=  
DSP_EXTRA_LIBS :=  
LADSPA_DSP_OBJ += 
LADSPA_DSP_CPP_OBJ += 
LADSPA_DSP_EXTRA_CFLAGS :=  
LADSPA_DSP_EXTRA_LIBS :=  
