#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "delay.h"
#include "util.h"

struct delay_state {
	sample_t **bufs;
	ssize_t len, p;
};

sample_t * delay_effect_run(struct effect *e, ssize_t *frames, sample_t *ibuf, sample_t *obuf)
{
	ssize_t i, k;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < *frames; ++i) {
		for (k = 0; k < e->istream.channels; ++k) {
			if (state->bufs[k] && state->len > 0) {
				obuf[i * e->istream.channels + k] = state->bufs[k][state->p];
				state->bufs[k][state->p] = ibuf[i * e->istream.channels + k];
			}
			else
				obuf[i * e->istream.channels + k] = ibuf[i * e->istream.channels + k];
		}
		state->p = (state->p + 1 >= state->len) ? 0 : state->p + 1;
	}
	return obuf;
}

void delay_effect_reset(struct effect *e)
{
	int i;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < e->istream.channels; ++i)
		if (state->bufs[i] && state->len > 0)
			memset(state->bufs[i], 0, state->len * sizeof(sample_t));
	state->p = 0;
}

void delay_effect_plot(struct effect *e, int i)
{
	int k;
	for (k = 0; k < e->ostream.channels; ++k)
		printf("H%d_%d(f)=0\n", k, i);
}

void delay_effect_destroy(struct effect *e)
{
	int i;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < e->istream.channels; ++i)
		free(state->bufs[i]);
	free(state->bufs);
	free(state);
}

struct effect * delay_effect_init(struct effect_info *ei, struct stream_info *istream, char *channel_selector, const char *dir, int argc, char **argv)
{
	char *endptr;
	struct effect *e;
	struct delay_state *state;
	int i;
	ssize_t samples;

	if (argc != 2) {
		LOG_FMT(LL_ERROR, "%s: usage: %s", argv[0], ei->usage);
		return NULL;
	}

	samples = parse_len(argv[1], istream->fs, &endptr);
	CHECK_ENDPTR(argv[1], endptr, "delay", return NULL);
	CHECK_RANGE(samples >= 0, "delay", return NULL);
	LOG_FMT(LL_VERBOSE, "%s: info: actual delay is %gs (%zd sample%s)", argv[0], (double) samples / istream->fs, samples, (samples == 1) ? "" : "s");
	state = calloc(1, sizeof(struct delay_state));
	state->len = samples;
	state->bufs = calloc(istream->channels, sizeof(sample_t *));
	for (i = 0; i < istream->channels; ++i)
		if (GET_BIT(channel_selector, i) && state->len > 0)
			state->bufs[i] = calloc(state->len, sizeof(sample_t));

	e = calloc(1, sizeof(struct effect));
	e->name = ei->name;
	e->istream.fs = e->ostream.fs = istream->fs;
	e->istream.channels = e->ostream.channels = istream->channels;
	e->run = delay_effect_run;
	e->reset = delay_effect_reset;
	e->plot = delay_effect_plot;
	e->destroy = delay_effect_destroy;
	e->data = state;
	return e;
}
