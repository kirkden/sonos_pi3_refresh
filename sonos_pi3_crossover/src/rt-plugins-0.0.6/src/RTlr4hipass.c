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

#define LR4HIGHPASS_FC                  0
#define LR4HIGHPASS_INPUT               1
#define LR4HIGHPASS_OUTPUT              2

static LADSPA_Descriptor *lr4highPassDescriptor = NULL;

typedef struct {
	LADSPA_Data *fc;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filters;
	float        fs;
} LR4HighPass;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return lr4highPassDescriptor;
	default:
		return NULL;
	}
}

static void activateLR4HighPass(LADSPA_Handle instance) {
	LR4HighPass *plugin_data = (LR4HighPass *)instance;
	biquad *filters = plugin_data->filters;
	float fs = plugin_data->fs;
	biquad_init(&filters[0]);
	biquad_init(&filters[1]);
	plugin_data->filters = filters;
	plugin_data->fs = fs;
}

static void cleanupLR4HighPass(LADSPA_Handle instance) {
	LR4HighPass *plugin_data = (LR4HighPass *)instance;
	free(plugin_data->filters);
	free(instance);
}

static void connectPortLR4HighPass(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LR4HighPass *plugin;

	plugin = (LR4HighPass *)instance;
	switch (port) {
	case LR4HIGHPASS_FC:
		plugin->fc = data;
		break;
	case LR4HIGHPASS_INPUT:
		plugin->input = data;
		break;
	case LR4HIGHPASS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLR4HighPass(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LR4HighPass *plugin_data = (LR4HighPass *)malloc(sizeof(LR4HighPass));
	biquad *filters = NULL;
	float fs;

	fs = (float)s_rate;

	filters = calloc(2, sizeof(biquad));
	biquad_init(&filters[0]);
	biquad_init(&filters[1]);

	plugin_data->filters = filters;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#define buffer_write(b, v) (b = v)

static void runLR4HighPass(LADSPA_Handle instance, unsigned long sample_count) {
	LR4HighPass *plugin_data = (LR4HighPass *)instance;

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

	unsigned long pos;
  bq_t in;

	hp_set_params(&filters[0], fc, 0.7071068, fs);
	hp_set_params(&filters[1], fc, 0.7071068, fs);

	for (pos = 0; pos < sample_count; pos++) {
    in = biquad_run(&filters[0], input[pos]);
    in = biquad_run(&filters[1], in);
    buffer_write(output[pos], (LADSPA_Data) in);
	}

}

void _init(void);
void _init(void) {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#define D_(s) (s)

	lr4highPassDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lr4highPassDescriptor) {
		lr4highPassDescriptor->UniqueID = 9021;
		lr4highPassDescriptor->Label = "RTlr4hipass";
		lr4highPassDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lr4highPassDescriptor->Name =
		 D_("RT LR4 highpass");
		lr4highPassDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		lr4highPassDescriptor->Copyright =
		 "GPL";
		lr4highPassDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		lr4highPassDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		lr4highPassDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		lr4highPassDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency (Hz) */
		port_descriptors[LR4HIGHPASS_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LR4HIGHPASS_FC] =
		 D_("Frequency (Hz) [-6dB]");
		port_range_hints[LR4HIGHPASS_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[LR4HIGHPASS_FC].LowerBound = 0;
		port_range_hints[LR4HIGHPASS_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Input */
		port_descriptors[LR4HIGHPASS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LR4HIGHPASS_INPUT] =
		 D_("Input");
		port_range_hints[LR4HIGHPASS_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LR4HIGHPASS_INPUT].LowerBound = -1.0;
		port_range_hints[LR4HIGHPASS_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[LR4HIGHPASS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LR4HIGHPASS_OUTPUT] =
		 D_("Output");
		port_range_hints[LR4HIGHPASS_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[LR4HIGHPASS_OUTPUT].LowerBound = -1.0;
		port_range_hints[LR4HIGHPASS_OUTPUT].UpperBound = +1.0;

		lr4highPassDescriptor->activate = activateLR4HighPass;
		lr4highPassDescriptor->cleanup = cleanupLR4HighPass;
		lr4highPassDescriptor->connect_port = connectPortLR4HighPass;
		lr4highPassDescriptor->deactivate = NULL;
		lr4highPassDescriptor->instantiate = instantiateLR4HighPass;
		lr4highPassDescriptor->run = runLR4HighPass;
		lr4highPassDescriptor->run_adding = NULL;
		lr4highPassDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (lr4highPassDescriptor) {
		free((LADSPA_PortDescriptor *)lr4highPassDescriptor->PortDescriptors);
		free((char **)lr4highPassDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lr4highPassDescriptor->PortRangeHints);
		free(lr4highPassDescriptor);
	}

}

