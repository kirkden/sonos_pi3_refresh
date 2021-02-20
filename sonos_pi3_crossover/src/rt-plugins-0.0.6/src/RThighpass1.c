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

#define HIGHPASS1_FC                    0
#define HIGHPASS1_INPUT                 1
#define HIGHPASS1_OUTPUT                2

static LADSPA_Descriptor *highPassDescriptor = NULL;

typedef struct {
	LADSPA_Data *fc;
	LADSPA_Data *input;
	LADSPA_Data *output;
	bilin *     filter;
	float        fs;
} HighPass1;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return highPassDescriptor;
	default:
		return NULL;
	}
}

static void activateHighPass1(LADSPA_Handle instance) {
	HighPass1 *plugin_data = (HighPass1 *)instance;
	bilin *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	bilin_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupHighPass1(LADSPA_Handle instance) {
	HighPass1 *plugin_data = (HighPass1 *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortHighPass1(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	HighPass1 *plugin;

	plugin = (HighPass1 *)instance;
	switch (port) {
	case HIGHPASS1_FC:
		plugin->fc = data;
		break;
	case HIGHPASS1_INPUT:
		plugin->input = data;
		break;
	case HIGHPASS1_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHighPass1(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	HighPass1 *plugin_data = (HighPass1 *)malloc(sizeof(HighPass1));
	bilin *filter = NULL;
	float fs;

	fs = (float)s_rate;
	filter = malloc(sizeof(bilin));
	bilin_init(filter);

	plugin_data->filter = filter;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#define buffer_write(b, v) (b = v)

static void runHighPass1(LADSPA_Handle instance, unsigned long sample_count) {
	HighPass1 *plugin_data = (HighPass1 *)instance;

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

	bilin * filter = plugin_data->filter;
	float fs = plugin_data->fs;

	unsigned long pos;

	hp1_set_params(filter, fc, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], (LADSPA_Data) bilin_run(filter, input[pos]));
	}

}

void _init(void);
void _init(void) {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#define D_(s) (s)

	highPassDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (highPassDescriptor) {
		highPassDescriptor->UniqueID = 9016;
		highPassDescriptor->Label = "RThighpass1";
		highPassDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		highPassDescriptor->Name =
		 D_("RT First Order Highpass");
		highPassDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		highPassDescriptor->Copyright =
		 "GPL";
		highPassDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		highPassDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		highPassDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		highPassDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency (Hz) */
		port_descriptors[HIGHPASS1_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHPASS1_FC] =
		 D_("Frequency (Hz) [-3dB]");
		port_range_hints[HIGHPASS1_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HIGHPASS1_FC].LowerBound = 0.0;
		port_range_hints[HIGHPASS1_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Input */
		port_descriptors[HIGHPASS1_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHPASS1_INPUT] =
		 D_("Input");
		port_range_hints[HIGHPASS1_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HIGHPASS1_INPUT].LowerBound = -1.0;
		port_range_hints[HIGHPASS1_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[HIGHPASS1_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHPASS1_OUTPUT] =
		 D_("Output");
		port_range_hints[HIGHPASS1_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HIGHPASS1_OUTPUT].LowerBound = -1.0;
		port_range_hints[HIGHPASS1_OUTPUT].UpperBound = +1.0;

		highPassDescriptor->activate = activateHighPass1;
		highPassDescriptor->cleanup = cleanupHighPass1;
		highPassDescriptor->connect_port = connectPortHighPass1;
		highPassDescriptor->deactivate = NULL;
		highPassDescriptor->instantiate = instantiateHighPass1;
		highPassDescriptor->run = runHighPass1;
		highPassDescriptor->run_adding = NULL;
		highPassDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (highPassDescriptor) {
		free((LADSPA_PortDescriptor *)highPassDescriptor->PortDescriptors);
		free((char **)highPassDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)highPassDescriptor->PortRangeHints);
		free(highPassDescriptor);
	}

}

