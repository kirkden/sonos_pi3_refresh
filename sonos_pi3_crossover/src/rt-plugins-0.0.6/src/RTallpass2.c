/* Copyright 2013 Richard Taylor

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  CREDITS:

  Much of the code here was adapted from Steve Harris's swh-plugins
  (version 0.4.15) found at http://plugin.org.uk/.  Thanks Steve for
  making your code available.

  The guts of the code, in util/biquad.h, is an implementation
  of the biquad filters described in Robert Bristow-Johnson's "Cookbook
  formulae for audio EQ biquad filter coefficients" found here:
  http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
*/

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <ladspa.h>

#include "biquad.h"

#define ALLPASS_FC                  0
#define ALLPASS_Q                   1
#define ALLPASS_INPUT               2
#define ALLPASS_OUTPUT              3

static LADSPA_Descriptor *allPassDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *fc;
	LADSPA_Data *Q;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
} AllPass;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return allPassDescriptor;
	default:
		return NULL;
	}
}

static void activateAllPass(LADSPA_Handle instance) {
	AllPass *plugin_data = (AllPass *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupAllPass(LADSPA_Handle instance) {
	AllPass *plugin_data = (AllPass *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortAllPass(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	AllPass *plugin;

	plugin = (AllPass *)instance;
	switch (port) {
	case ALLPASS_FC:
		plugin->fc = data;
		break;
	case ALLPASS_Q:
		plugin->Q = data;
		break;
	case ALLPASS_INPUT:
		plugin->input = data;
		break;
	case ALLPASS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateAllPass(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	AllPass *plugin_data = (AllPass *)malloc(sizeof(AllPass));
	biquad *filter = NULL;
	float fs;

	fs = (float)s_rate;
	filter = malloc(sizeof(biquad));
	biquad_init(filter);

	plugin_data->filter = filter;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#define buffer_write(b, v) (b = v)

static void runAllPass(LADSPA_Handle instance, unsigned long sample_count) {
	AllPass *plugin_data = (AllPass *)instance;

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Bandwidth (octaves) (float value) */
	const LADSPA_Data Q = *(plugin_data->Q);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

	biquad * filter = plugin_data->filter;
	float fs = plugin_data->fs;

	unsigned long pos;

	ap_set_params(filter, fc, Q, fs);

	for (pos = 0; pos < sample_count; pos++) {
		// RT 2.9.2013: replace biquad_run with ap2_run to cut floating-
		// point multiplications from 5 to 2:
	  buffer_write(output[pos], (LADSPA_Data) ap2_run(filter, input[pos]));
	}

}

void _init(void);
void _init(void) {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#define D_(s) (s)

	allPassDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (allPassDescriptor) {
		allPassDescriptor->UniqueID = 9005;
		allPassDescriptor->Label = "RTallpass2";
		allPassDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		allPassDescriptor->Name =
		 D_("RT 2nd-Order Allpass");
		allPassDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		allPassDescriptor->Copyright =
		 "GPL";
		allPassDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		allPassDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		allPassDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		allPassDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency (Hz) */
		port_descriptors[ALLPASS_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_FC] =
		 D_("Frequency (Hz)");
		port_range_hints[ALLPASS_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[ALLPASS_FC].LowerBound = 0;
		port_range_hints[ALLPASS_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Bandwidth (octaves) */
		port_descriptors[ALLPASS_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_Q] =
		 D_("Q");
		port_range_hints[ALLPASS_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[ALLPASS_Q].LowerBound = 0;
		port_range_hints[ALLPASS_Q].UpperBound = 10;

		/* Parameters for Input */
		port_descriptors[ALLPASS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_INPUT] =
		 D_("Input");
		port_range_hints[ALLPASS_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[ALLPASS_INPUT].LowerBound = -1.0;
		port_range_hints[ALLPASS_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[ALLPASS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_OUTPUT] =
		 D_("Output");
		port_range_hints[ALLPASS_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[ALLPASS_OUTPUT].LowerBound = -1.0;
		port_range_hints[ALLPASS_OUTPUT].UpperBound = +1.0;

		allPassDescriptor->activate = activateAllPass;
		allPassDescriptor->cleanup = cleanupAllPass;
		allPassDescriptor->connect_port = connectPortAllPass;
		allPassDescriptor->deactivate = NULL;
		allPassDescriptor->instantiate = instantiateAllPass;
		allPassDescriptor->run = runAllPass;
		allPassDescriptor->run_adding = NULL;
		allPassDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (allPassDescriptor) {
		free((LADSPA_PortDescriptor *)allPassDescriptor->PortDescriptors);
		free((char **)allPassDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)allPassDescriptor->PortRangeHints);
		free(allPassDescriptor);
	}

}

