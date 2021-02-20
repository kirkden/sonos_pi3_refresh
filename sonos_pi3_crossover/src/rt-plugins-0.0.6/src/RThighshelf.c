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

#define HIGHSHELF_GAIN                0
#define HIGHSHELF_FC                  1
#define HIGHSHELF_Q                   2
#define HIGHSHELF_INPUT               3
#define HIGHSHELF_OUTPUT              4

static LADSPA_Descriptor *highShelfDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *fc;
	LADSPA_Data *Q;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
} HighShelf;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return highShelfDescriptor;
	default:
		return NULL;
	}
}

static void activateHighShelf(LADSPA_Handle instance) {
	HighShelf *plugin_data = (HighShelf *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupHighShelf(LADSPA_Handle instance) {
	HighShelf *plugin_data = (HighShelf *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortHighShelf(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	HighShelf *plugin;

	plugin = (HighShelf *)instance;
	switch (port) {
	case HIGHSHELF_GAIN:
		plugin->gain = data;
		break;
	case HIGHSHELF_FC:
		plugin->fc = data;
		break;
	case HIGHSHELF_Q:
		plugin->Q = data;
		break;
	case HIGHSHELF_INPUT:
		plugin->input = data;
		break;
	case HIGHSHELF_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHighShelf(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	HighShelf *plugin_data = (HighShelf *)malloc(sizeof(HighShelf));
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

static void runHighShelf(LADSPA_Handle instance, unsigned long sample_count) {
	HighShelf *plugin_data = (HighShelf *)instance;

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

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

	hs_set_params(filter, fc, gain, Q, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], (LADSPA_Data) biquad_run(filter, input[pos]));
	}

}

void _init(void);
void _init(void) {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#define D_(s) (s)

	highShelfDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (highShelfDescriptor) {
		highShelfDescriptor->UniqueID = 9003;
		highShelfDescriptor->Label = "RThighshelf";
		highShelfDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		highShelfDescriptor->Name =
		 D_("RT High Shelf");
		highShelfDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		highShelfDescriptor->Copyright =
		 "GPL";
		highShelfDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		highShelfDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		highShelfDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		highShelfDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Gain (dB) */
		port_descriptors[HIGHSHELF_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHSHELF_GAIN] =
		 D_("Gain (dB)");
		port_range_hints[HIGHSHELF_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HIGHSHELF_GAIN].LowerBound = -70;
		port_range_hints[HIGHSHELF_GAIN].UpperBound = +30;

		/* Parameters for Frequency (Hz) */
		port_descriptors[HIGHSHELF_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHSHELF_FC] =
		 D_("Frequency (Hz)");
		port_range_hints[HIGHSHELF_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HIGHSHELF_FC].LowerBound = 0;
		port_range_hints[HIGHSHELF_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Quality Factor */
		port_descriptors[HIGHSHELF_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHSHELF_Q] =
		 D_("Q");
		port_range_hints[HIGHSHELF_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[HIGHSHELF_Q].LowerBound = 0;
		port_range_hints[HIGHSHELF_Q].UpperBound = 5;

		/* Parameters for Input */
		port_descriptors[HIGHSHELF_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHSHELF_INPUT] =
		 D_("Input");
		port_range_hints[HIGHSHELF_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HIGHSHELF_INPUT].LowerBound = -1.0;
		port_range_hints[HIGHSHELF_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[HIGHSHELF_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHSHELF_OUTPUT] =
		 D_("Output");
		port_range_hints[HIGHSHELF_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HIGHSHELF_OUTPUT].LowerBound = -1.0;
		port_range_hints[HIGHSHELF_OUTPUT].UpperBound = +1.0;

		highShelfDescriptor->activate = activateHighShelf;
		highShelfDescriptor->cleanup = cleanupHighShelf;
		highShelfDescriptor->connect_port = connectPortHighShelf;
		highShelfDescriptor->deactivate = NULL;
		highShelfDescriptor->instantiate = instantiateHighShelf;
		highShelfDescriptor->run = runHighShelf;
		highShelfDescriptor->run_adding = NULL;
		highShelfDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (highShelfDescriptor) {
		free((LADSPA_PortDescriptor *)highShelfDescriptor->PortDescriptors);
		free((char **)highShelfDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)highShelfDescriptor->PortRangeHints);
		free(highShelfDescriptor);
	}

}

