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

#define LOWPASS_FC                  0
#define LOWPASS_Q                   1
#define LOWPASS_INPUT               2
#define LOWPASS_OUTPUT              3

static LADSPA_Descriptor *lowPassDescriptor = NULL;

typedef struct {
	LADSPA_Data *fc;
	LADSPA_Data *Q;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
} LowPass;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return lowPassDescriptor;
	default:
		return NULL;
	}
}

static void activateLowPass(LADSPA_Handle instance) {
	LowPass *plugin_data = (LowPass *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupLowPass(LADSPA_Handle instance) {
	LowPass *plugin_data = (LowPass *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortLowPass(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LowPass *plugin;

	plugin = (LowPass *)instance;
	switch (port) {
	case LOWPASS_FC:
		plugin->fc = data;
		break;
	case LOWPASS_Q:
		plugin->Q = data;
		break;
	case LOWPASS_INPUT:
		plugin->input = data;
		break;
	case LOWPASS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLowPass(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LowPass *plugin_data = (LowPass *)malloc(sizeof(LowPass));
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

static void runLowPass(LADSPA_Handle instance, unsigned long sample_count) {
	LowPass *plugin_data = (LowPass *)instance;

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

	lp_set_params(filter, fc, Q, fs);

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

	lowPassDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lowPassDescriptor) {
		lowPassDescriptor->UniqueID = 9006;
		lowPassDescriptor->Label = "RTlowpass";
		lowPassDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lowPassDescriptor->Name =
		 D_("RT 2nd-order lowpass");
		lowPassDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		lowPassDescriptor->Copyright =
		 "GPL";
		lowPassDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		lowPassDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		lowPassDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		lowPassDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency (Hz) */
		port_descriptors[LOWPASS_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWPASS_FC] =
		 D_("Frequency (Hz) [-3dB]");
		port_range_hints[LOWPASS_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[LOWPASS_FC].LowerBound = 0;
		port_range_hints[LOWPASS_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Q */
		port_descriptors[LOWPASS_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWPASS_Q] =
		 D_("Q");
		port_range_hints[LOWPASS_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[LOWPASS_Q].LowerBound = 0;
		port_range_hints[LOWPASS_Q].UpperBound = 5;

		/* Parameters for Input */
		port_descriptors[LOWPASS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LOWPASS_INPUT] =
		 D_("Input");
		port_range_hints[LOWPASS_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LOWPASS_INPUT].LowerBound = -1.0;
		port_range_hints[LOWPASS_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[LOWPASS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LOWPASS_OUTPUT] =
		 D_("Output");
		port_range_hints[LOWPASS_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LOWPASS_OUTPUT].LowerBound = -1.0;
		port_range_hints[LOWPASS_OUTPUT].UpperBound = +1.0;

		lowPassDescriptor->activate = activateLowPass;
		lowPassDescriptor->cleanup = cleanupLowPass;
		lowPassDescriptor->connect_port = connectPortLowPass;
		lowPassDescriptor->deactivate = NULL;
		lowPassDescriptor->instantiate = instantiateLowPass;
		lowPassDescriptor->run = runLowPass;
		lowPassDescriptor->run_adding = NULL;
		lowPassDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (lowPassDescriptor) {
		free((LADSPA_PortDescriptor *)lowPassDescriptor->PortDescriptors);
		free((char **)lowPassDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lowPassDescriptor->PortRangeHints);
		free(lowPassDescriptor);
	}

}

