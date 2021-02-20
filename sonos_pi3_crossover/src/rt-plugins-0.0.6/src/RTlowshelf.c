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

#define LOWSHELF_GAIN                0
#define LOWSHELF_FC                  1
#define LOWSHELF_Q                   2
#define LOWSHELF_INPUT               3
#define LOWSHELF_OUTPUT              4

static LADSPA_Descriptor *lowShelfDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *fc;
	LADSPA_Data *Q;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
} LowShelf;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return lowShelfDescriptor;
	default:
		return NULL;
	}
}

static void activateLowShelf(LADSPA_Handle instance) {
	LowShelf *plugin_data = (LowShelf *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupLowShelf(LADSPA_Handle instance) {
	LowShelf *plugin_data = (LowShelf *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortLowShelf(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LowShelf *plugin;

	plugin = (LowShelf *)instance;
	switch (port) {
	case LOWSHELF_GAIN:
		plugin->gain = data;
		break;
	case LOWSHELF_FC:
		plugin->fc = data;
		break;
	case LOWSHELF_Q:
		plugin->Q = data;
		break;
	case LOWSHELF_INPUT:
		plugin->input = data;
		break;
	case LOWSHELF_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLowShelf(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LowShelf *plugin_data = (LowShelf *)malloc(sizeof(LowShelf));
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

static void runLowShelf(LADSPA_Handle instance, unsigned long sample_count) {
	LowShelf *plugin_data = (LowShelf *)instance;

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

	ls_set_params(filter, fc, gain, Q, fs);

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

	lowShelfDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lowShelfDescriptor) {
		lowShelfDescriptor->UniqueID = 9002;
		lowShelfDescriptor->Label = "RTlowshelf";
		lowShelfDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lowShelfDescriptor->Name =
		 D_("RT Low Shelf");
		lowShelfDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		lowShelfDescriptor->Copyright =
		 "GPL";
		lowShelfDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		lowShelfDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		lowShelfDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		lowShelfDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Gain (dB) */
		port_descriptors[LOWSHELF_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWSHELF_GAIN] =
		 D_("Gain (dB)");
		port_range_hints[LOWSHELF_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[LOWSHELF_GAIN].LowerBound = -70;
		port_range_hints[LOWSHELF_GAIN].UpperBound = +30;

		/* Parameters for Frequency (Hz) */
		port_descriptors[LOWSHELF_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWSHELF_FC] =
		 D_("Frequency (Hz)");
		port_range_hints[LOWSHELF_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[LOWSHELF_FC].LowerBound = 0;
		port_range_hints[LOWSHELF_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Quality Factor */
		port_descriptors[LOWSHELF_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWSHELF_Q] =
		 D_("Q");
		port_range_hints[LOWSHELF_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[LOWSHELF_Q].LowerBound = 0;
		port_range_hints[LOWSHELF_Q].UpperBound = 5;

		/* Parameters for Input */
		port_descriptors[LOWSHELF_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LOWSHELF_INPUT] =
		 D_("Input");
		port_range_hints[LOWSHELF_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LOWSHELF_INPUT].LowerBound = -1.0;
		port_range_hints[LOWSHELF_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[LOWSHELF_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LOWSHELF_OUTPUT] =
		 D_("Output");
		port_range_hints[LOWSHELF_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LOWSHELF_OUTPUT].LowerBound = -1.0;
		port_range_hints[LOWSHELF_OUTPUT].UpperBound = +1.0;

		lowShelfDescriptor->activate = activateLowShelf;
		lowShelfDescriptor->cleanup = cleanupLowShelf;
		lowShelfDescriptor->connect_port = connectPortLowShelf;
		lowShelfDescriptor->deactivate = NULL;
		lowShelfDescriptor->instantiate = instantiateLowShelf;
		lowShelfDescriptor->run = runLowShelf;
		lowShelfDescriptor->run_adding = NULL;
		lowShelfDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (lowShelfDescriptor) {
		free((LADSPA_PortDescriptor *)lowShelfDescriptor->PortDescriptors);
		free((char **)lowShelfDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lowShelfDescriptor->PortRangeHints);
		free(lowShelfDescriptor);
	}

}

