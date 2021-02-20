/*
  interface.cc

	Copyright 2004-14 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	LADSPA descriptor factory, host interface.

*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/
/*
	LADSPA ID ranges 1761 - 1800 and 2581 - 2660 
	(2541 - 2580 donated to artemio@kdemail.net)
*/

#include "version.h"
#include "basics.h"

#include "Cabinet.h"
#include "Chorus.h"
#include "Phaser.h"
#include "Sin.h"
#include "Fractals.h"
#include "Reverb.h"
#include "Compress.h"
#include "Click.h"
#include "Eq.h"
#include "Saturate.h"
#include "White.h"
#include "AutoFilter.h"
#include "Amp.h"
#include "Pan.h"
#include "Scape.h"
#include "ToneStack.h" 
#include "Noisegate.h" 
#ifdef SUMMER
#include "AmpVI.h"
#endif

#include "Descriptor.h"

#define N 36 

static DescriptorStub * descriptors[N+1];

extern "C" {

const LADSPA_Descriptor * 
ladspa_descriptor (unsigned long i) { return i < N ? descriptors[i] : 0; }

__attribute__ ((constructor)) 
void caps_so_init()
{
	DescriptorStub ** d = descriptors;
	/* make sure uninitialised array members are safe to pass to the host */
	memset (d, 0, sizeof (descriptors));

	*d++ = new Descriptor<Noisegate>(2602);
	*d++ = new Descriptor<Compress>(1772);
	*d++ = new Descriptor<CompressX2>(2598);

	*d++ = new Descriptor<ToneStack>(2589);
	*d++ = new Descriptor<AmpVTS>(2592);
	#ifdef SUMMER
	*d++ = new Descriptor<AmpVI>(1789);
	#endif
	*d++ = new Descriptor<CabinetIII>(2601);
	*d++ = new Descriptor<CabinetIV>(2606);

	#ifdef WITH_JVREV
	*d++ = new Descriptor<JVRev>(1778);
	#endif
	*d++ = new Descriptor<Plate>(1779);
	*d++ = new Descriptor<PlateX2>(1795);

	*d++ = new Descriptor<Saturate>(1771);
	*d++ = new Descriptor<Spice>(2603);
	*d++ = new Descriptor<SpiceX2>(2607);

	*d++ = new Descriptor<ChorusI>(1767);
	*d++ = new Descriptor<PhaserII>(2586);

	*d++ = new Descriptor<AutoFilter>(2593);
	*d++ = new Descriptor<Scape>(2588);
	#if 0
	*d++ = new Descriptor<DDDelay>(2610);
	#endif

	*d++ = new Descriptor<Eq10>(1773);
	*d++ = new Descriptor<Eq10X2>(2594);
	*d++ = new Descriptor<Eq4p>(2608);
	*d++ = new Descriptor<EqFA4p>(2609);

	*d++ = new Descriptor<Wider>(1788);
	*d++ = new Descriptor<Narrower>(2595);

	*d++ = new Descriptor<Sin>(1781);
	*d++ = new Descriptor<White>(1785);
	*d++ = new Descriptor<Fractal>(1774);

	*d++ = new Descriptor<Click>(1769);
	*d++ = new Descriptor<CEO>(1770);
	
	assert (d - descriptors <= N);
}

__attribute__ ((destructor)) 
void caps_so_fini()
{
	DescriptorStub ** d = descriptors;
	while (*d) delete *d++;
}

}; /* extern "C" */
