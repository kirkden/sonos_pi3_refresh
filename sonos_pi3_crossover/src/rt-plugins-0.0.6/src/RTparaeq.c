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

#define SINGLEPARA_GAIN                0
#define SINGLEPARA_FC                  1
#define SINGLEPARA_Q                   2
#define SINGLEPARA_INPUT               3
#define SINGLEPARA_OUTPUT              4

static LADSPA_Descriptor *singleParaDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *fc;
	LADSPA_Data *Q;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
} SinglePara;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return singleParaDescriptor;
	default:
		return NULL;
	}
}

static void activateSinglePara(LADSPA_Handle instance) {
	SinglePara *plugin_data = (SinglePara *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupSinglePara(LADSPA_Handle instance) {
	SinglePara *plugin_data = (SinglePara *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortSinglePara(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SinglePara *plugin;

	plugin = (SinglePara *)instance;
	switch (port) {
	case SINGLEPARA_GAIN:
		plugin->gain = data;
		break;
	case SINGLEPARA_FC:
		plugin->fc = data;
		break;
	case SINGLEPARA_Q:
		plugin->Q = data;
		break;
	case SINGLEPARA_INPUT:
		plugin->input = data;
		break;
	case SINGLEPARA_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSinglePara(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SinglePara *plugin_data = (SinglePara *)malloc(sizeof(SinglePara));
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

static void runSinglePara(LADSPA_Handle instance, unsigned long sample_count) {
	SinglePara *plugin_data = (SinglePara *)instance;

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Quality factor (float value) */
	const LADSPA_Data Q = *(plugin_data->Q);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filter = plugin_data->filter;
	float fs = plugin_data->fs;

	unsigned long pos;

	eq_set_params(filter, fc, gain, Q, fs);

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

	singleParaDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (singleParaDescriptor) {
		singleParaDescriptor->UniqueID = 9001;
		singleParaDescriptor->Label = "RTparaeq";
		singleParaDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		singleParaDescriptor->Name =
		 D_("RT parametric eq");
		singleParaDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		singleParaDescriptor->Copyright =
		 "GPL";
		singleParaDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		singleParaDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		singleParaDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		singleParaDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Gain (dB) */
		port_descriptors[SINGLEPARA_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_GAIN] =
		 D_("Gain (dB)");
		port_range_hints[SINGLEPARA_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SINGLEPARA_GAIN].LowerBound = -70;
		port_range_hints[SINGLEPARA_GAIN].UpperBound = +30;

		/* Parameters for Frequency (Hz) */
		port_descriptors[SINGLEPARA_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_FC] =
		 D_("Frequency (Hz)");
		port_range_hints[SINGLEPARA_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[SINGLEPARA_FC].LowerBound = 0;
		port_range_hints[SINGLEPARA_FC].UpperBound = (LADSPA_Data) 0.4;

		/* Parameters for Quality Factor */
		port_descriptors[SINGLEPARA_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_Q] =
		 D_("Q");
		port_range_hints[SINGLEPARA_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SINGLEPARA_Q].LowerBound = 0;
		port_range_hints[SINGLEPARA_Q].UpperBound = 100;

		/* Parameters for Input */
		port_descriptors[SINGLEPARA_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SINGLEPARA_INPUT] =
		 D_("Input");
		port_range_hints[SINGLEPARA_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINGLEPARA_INPUT].LowerBound = -1.0;
		port_range_hints[SINGLEPARA_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[SINGLEPARA_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SINGLEPARA_OUTPUT] =
		 D_("Output");
		port_range_hints[SINGLEPARA_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINGLEPARA_OUTPUT].LowerBound = -1.0;
		port_range_hints[SINGLEPARA_OUTPUT].UpperBound = +1.0;

		singleParaDescriptor->activate = activateSinglePara;
		singleParaDescriptor->cleanup = cleanupSinglePara;
		singleParaDescriptor->connect_port = connectPortSinglePara;
		singleParaDescriptor->deactivate = NULL;
		singleParaDescriptor->instantiate = instantiateSinglePara;
		singleParaDescriptor->run = runSinglePara;
		singleParaDescriptor->run_adding = NULL;
		singleParaDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (singleParaDescriptor) {
		free((LADSPA_PortDescriptor *)singleParaDescriptor->PortDescriptors);
		free((char **)singleParaDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)singleParaDescriptor->PortRangeHints);
		free(singleParaDescriptor);
	}

}

