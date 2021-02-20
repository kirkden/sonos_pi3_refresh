#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "biquad.h"
#include "util.h"

static double parse_width(const char *s, int *type, char **endptr)
{
	double w = strtod(s, endptr);
	if (*endptr != NULL && *endptr != s) {
		switch(**endptr) {
		case 'q':
			*type = BIQUAD_WIDTH_Q;
			++(*endptr);
			break;
		case 's':
			*type = BIQUAD_WIDTH_SLOPE;
			++(*endptr);
			break;
		case 'd':
			*type = BIQUAD_WIDTH_SLOPE_DB;
			++(*endptr);
			break;
		case 'o':
			*type = BIQUAD_WIDTH_BW_OCT;
			++(*endptr);
			break;
		case 'k':
			w *= 1000.0;
		case 'h':
			*type = BIQUAD_WIDTH_BW_HZ;
			++(*endptr);
			break;
		}
		if (**endptr != '\0') LOG_FMT(LL_ERROR, "%s(): trailing characters: %s", __func__, *endptr);
	}
	return w;
}

void biquad_init(struct biquad_state *state, double b0, double b1, double b2, double a0, double a1, double a2)
{
	state->c0 = b0 / a0;
	state->c1 = b1 / a0;
	state->c2 = b2 / a0;
	state->c3 = a1 / a0;
	state->c4 = a2 / a0;
	biquad_reset(state);
}

void biquad_reset(struct biquad_state *state)
{
#if BIQUAD_USE_TDF_2
	state->m0 = state->m1 = 0.0;
#else
	state->i0 = state->i1 = 0.0;
	state->o0 = state->o1 = 0.0;
#endif
}

void biquad_init_using_type(struct biquad_state *b, int type, double fs, double arg0, double arg1, double arg2, double arg3, int width_type)
{
	double b0, b1, b2, a0, a1, a2;
	double f0, width, gain, a, w0, sin_w0, cos_w0, alpha, c;
	double fz, qz, fp, qp, fc, d0i, d1i, d2i, c0i, c1i, c2i, gn, cci;

	if (type == BIQUAD_LINKWITZ_TRANSFORM) {
		fz = arg0;
		qz = arg1;
		fp = arg2;
		qp = arg3;

		fc = (fz + fp) / 2.0;

		d0i = pow(2.0 * M_PI * fz, 2.0);
		d1i = (2.0 * M_PI * fz) / qz;
		d2i = 1;

		c0i = pow(2.0 * M_PI * fp, 2.0);
		c1i = (2.0 * M_PI * fp) / qp;
		c2i = 1;

		gn = (2.0 * M_PI * fc) / tan(M_PI * fc / fs);
		cci = c0i + gn * c1i + pow(gn, 2.0) * c2i;

		b0 = (d0i + gn * d1i + pow(gn, 2.0) * d2i) / cci;
		b1 = 2 * (d0i - pow(gn, 2.0) * d2i) / cci;
		b2 = (d0i - gn * d1i + pow(gn, 2.0) * d2i) / cci;
		a0 = 1;
		a1 = (2.0 * (c0i - pow(gn, 2.0) * c2i) / cci);
		a2 = ((c0i - gn * c1i + pow(gn, 2.0) * c2i) / cci);
	}
	else {
		f0 = arg0;
		width = arg1;
		gain = arg2;

		if (width_type == BIQUAD_WIDTH_SLOPE_DB) {
			width_type = BIQUAD_WIDTH_SLOPE;
			width /= 12.0;
			if (type == BIQUAD_LOWSHELF)
				f0 *= pow(10.0, fabs(gain) / 80.0 / width);
			else if (type == BIQUAD_HIGHSHELF)
				f0 /= pow(10.0, fabs(gain) / 80.0 / width);
		}

		a = pow(10.0, gain / 40.0);
		w0 = 2 * M_PI * f0 / fs;
		sin_w0 = sin(w0);
		cos_w0 = cos(w0);

		switch (width_type) {
		case BIQUAD_WIDTH_SLOPE:
			alpha = sin_w0 / 2.0 * sqrt((a + 1 / a) * (1 / width - 1) + 2);
			break;
		case BIQUAD_WIDTH_BW_OCT:
			alpha = sin_w0 * sinh(log(2) / 2 * width * w0 / sin_w0);
			break;
		case BIQUAD_WIDTH_BW_HZ:
			alpha = sin_w0 / (2 * f0 / width);
			break;
		case BIQUAD_WIDTH_Q:
		default:
			alpha = sin_w0 / (2.0 * width);
		}

		switch (type) {
		case BIQUAD_LOWPASS_1:
			a0 = 1.0;
			a1 = -exp(-w0);
			a2 = 0.0;
			b0 = 1.0 + a1;
			b1 = b2 = 0.0;
			break;
		case BIQUAD_HIGHPASS_1:
			a0 = 1.0;
			a1 = -exp(-w0);
			a2 = 0.0;
			b0 = (1.0 - a1) / 2.0;
			b1 = -b0;
			b2 = 0.0;
			break;
		case BIQUAD_LOWPASS:
			b0 = (1.0 - cos_w0) / 2.0;
			b1 = 1.0 - cos_w0;
			b2 = b0;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;
		case BIQUAD_HIGHPASS:
			b0 = (1.0 + cos_w0) / 2.0;
			b1 = -(1.0 + cos_w0);
			b2 = b0;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;
		case BIQUAD_BANDPASS_SKIRT:
			b0 = sin_w0 / 2.0;
			b1 = 0.0;
			b2 = -b0;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;
		case BIQUAD_BANDPASS_PEAK:
			b0 = alpha;
			b1 = 0.0;
			b2 = -alpha;
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_w0;
			a2 = 1.0 - alpha;
			break;
		case BIQUAD_NOTCH:
			b0 = 1.0;
			b1 = -2.0 * cos_w0;
			b2 = 1.0;
			a0 = 1.0 + alpha;
			a1 = b1;
			a2 = 1.0 - alpha;
			break;
		case BIQUAD_ALLPASS:
			b0 = 1.0 - alpha;
			b1 = -2.0 * cos_w0;
			b2 = 1.0 + alpha;
			a0 = b2;
			a1 = b1;
			a2 = b0;
			break;
		case BIQUAD_PEAK:
			b0 = 1.0 + alpha * a;
			b1 = -2.0 * cos_w0;
			b2 = 1.0 - alpha * a;
			a0 = 1.0 + alpha / a;
			a1 = b1;
			a2 = 1.0 - alpha / a;
			break;
		case BIQUAD_LOWSHELF:
			c = 2.0 * sqrt(a) * alpha;
			b0 = a * ((a + 1.0) - (a - 1.0) * cos_w0 + c);
			b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cos_w0);
			b2 = a * ((a + 1.0) - (a - 1.0) * cos_w0 - c);
			a0 = (a + 1.0) + (a - 1.0) * cos_w0 + c;
			a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cos_w0);
			a2 = (a + 1.0) + (a - 1.0) * cos_w0 - c;
			break;
		case BIQUAD_HIGHSHELF:
			c = 2.0 * sqrt(a) * alpha;
			b0 = a * ((a + 1.0) + (a - 1.0) * cos_w0 + c);
			b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cos_w0);
			b2 = a * ((a + 1.0) + (a - 1.0) * cos_w0 - c);
			a0 = (a + 1.0) - (a - 1.0) * cos_w0 + c;
			a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cos_w0);
			a2 = (a + 1.0) - (a - 1.0) * cos_w0 - c;
			break;
		default:
			/* do nothing */
			b0 = 1.0;
			b1 = 0.0;
			b2 = 0.0;
			a0 = 1.0;
			a1 = 0.0;
			a2 = 0.0;
		}
	}
	biquad_init(b, b0, b1, b2, a0, a1, a2);
}

sample_t * biquad_effect_run(struct effect *e, ssize_t *frames, sample_t *ibuf, sample_t *obuf)
{
	ssize_t samples = *frames * e->ostream.channels, i, k;
	struct biquad_state **state = (struct biquad_state **) e->data;
	for (i = 0; i < samples; i += e->ostream.channels)
		for (k = 0; k < e->ostream.channels; ++k)
			if (state[k])
				ibuf[i + k] = biquad(state[k], ibuf[i + k]);
	return ibuf;
}

void biquad_effect_reset(struct effect *e)
{
	int i;
	struct biquad_state **state = (struct biquad_state **) e->data;
	for (i = 0; i < e->ostream.channels; ++i)
		if (state[i])
			biquad_reset(state[i]);
}

void biquad_effect_plot(struct effect *e, int i)
{
	struct biquad_state **state = (struct biquad_state **) e->data;
	int k, header_printed = 0;
	for (k = 0; k < e->ostream.channels; ++k) {
		if (state[k]) {
			if (!header_printed) {
				printf(
					"o%d=2*pi/%d\n"
					"c%d0=%.15e; c%d1=%.15e; c%d2=%.15e; c%d3=%.15e; c%d4=%.15e\n",
					i, e->ostream.fs, i, state[k]->c0, i, state[k]->c1, i, state[k]->c2, i, state[k]->c3, i, state[k]->c4
				);
				header_printed = 1;
			}
			printf(
				"H%d_%d(f)=20*log10(sqrt((c%d0*c%d0+c%d1*c%d1+c%d2*c%d2+2.*(c%d0*c%d1+c%d1*c%d2)*cos(f*o%d)+2.*(c%d0*c%d2)*cos(2.*f*o%d))/(1.+c%d3*c%d3+c%d4*c%d4+2.*(c%d3+c%d3*c%d4)*cos(f*o%d)+2.*c%d4*cos(2.*f*o%d))))\n",
				k, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i
			);
		}
		else
			printf("H%d_%d(f)=0\n", k, i);
	}
}

void biquad_effect_destroy(struct effect *e)
{
	int i;
	struct biquad_state **state = (struct biquad_state **) e->data;
	for (i = 0; i < e->istream.channels; ++i)
		free(state[i]);
	free(state);
}

#define GET_ARG(v, str, name) \
	do { \
		v = strtod(str, &endptr); \
		CHECK_ENDPTR(str, endptr, name, return NULL); \
	} while (0)

#define GET_FREQ_ARG(v, str, name) \
	do { \
		v = parse_freq(str, &endptr); \
		CHECK_ENDPTR(str, endptr, name, return NULL); \
		CHECK_FREQ(v, istream->fs, name, return NULL); \
	} while (0)

#define GET_WIDTH_ARG(v, str, name) \
	do { \
		v = parse_width(str, &width_type, &endptr); \
		CHECK_ENDPTR(str, endptr, name, return NULL); \
		CHECK_RANGE((v) > 0.0, name, return NULL); \
	} while (0)

#define BIQUAD_WIDTH_TEST_NO_SLOPE (width_type != BIQUAD_WIDTH_SLOPE && width_type != BIQUAD_WIDTH_SLOPE_DB)
#define CHECK_WIDTH_TYPE(cond) \
	if (!(cond)) { \
		LOG_FMT(LL_ERROR, "%s: error: invalid width type", argv[0]); \
		return NULL; \
	}

#define INIT_COMMON(n_args, b_type) \
	do { \
		if (argc != (n_args) + 1) { \
			LOG_FMT(LL_ERROR, "%s: usage: %s", argv[0], ei->usage); \
			return NULL; \
		} \
		type = b_type; \
	} while (0)

struct effect * biquad_effect_init(struct effect_info *ei, struct stream_info *istream, char *channel_selector, const char *dir, int argc, char **argv)
{
	int i, type, width_type = BIQUAD_WIDTH_Q;
	double arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	double b0 = 0, b1 = 0, b2 = 0, a0 = 0, a1 = 0, a2 = 0;
	struct biquad_state **state;
	struct effect *e;
	char *endptr;

	switch (ei->effect_number) {
	case BIQUAD_LOWPASS_1:
	case BIQUAD_HIGHPASS_1:
		INIT_COMMON(1, ei->effect_number);
		GET_FREQ_ARG(arg0, argv[1], "f0");
		break;
	case BIQUAD_LOWPASS:
	case BIQUAD_HIGHPASS:
	case BIQUAD_BANDPASS_SKIRT:
	case BIQUAD_BANDPASS_PEAK:
	case BIQUAD_NOTCH:
	case BIQUAD_ALLPASS:
		INIT_COMMON(2, ei->effect_number);
		GET_FREQ_ARG(arg0, argv[1], "f0");
		GET_WIDTH_ARG(arg1, argv[2], "width");
		CHECK_WIDTH_TYPE(BIQUAD_WIDTH_TEST_NO_SLOPE);
		break;
	case BIQUAD_PEAK:
	case BIQUAD_LOWSHELF:
	case BIQUAD_HIGHSHELF:
		INIT_COMMON(3, ei->effect_number);
		GET_FREQ_ARG(arg0, argv[1], "f0");
		GET_WIDTH_ARG(arg1, argv[2], "width");
		if (ei->effect_number == BIQUAD_PEAK) CHECK_WIDTH_TYPE(BIQUAD_WIDTH_TEST_NO_SLOPE);
		GET_ARG(arg2, argv[3], "gain");
		break;
	case BIQUAD_LINKWITZ_TRANSFORM:
		INIT_COMMON(4, BIQUAD_LINKWITZ_TRANSFORM);
		GET_FREQ_ARG(arg0, argv[1], "fz");
		GET_ARG(arg1, argv[2], "qz");
		CHECK_RANGE(arg1 > 0.0, "qz", return NULL);
		GET_FREQ_ARG(arg2, argv[3], "fp");
		GET_ARG(arg3, argv[4], "qp");
		CHECK_RANGE(arg3 > 0.0, "qp", return NULL);
		break;
	case BIQUAD_DEEMPH:
		INIT_COMMON(0, BIQUAD_HIGHSHELF);
		width_type = BIQUAD_WIDTH_SLOPE;
		switch (istream->fs) {
		case 44100:
			arg0 = 5283;
			arg1 = 0.4845;
			arg2 = -9.477;
			break;
		case 48000:
			arg0 = 5356;
			arg1 = 0.479;
			arg2 = -9.62;
			break;
		default:
			LOG_FMT(LL_ERROR, "%s: error: sample rate must be 44100 or 48000", argv[0]);
			return NULL;
		}
		break;
	case BIQUAD_BIQUAD:
		INIT_COMMON(6, BIQUAD_BIQUAD);
		GET_ARG(b0, argv[1], "b0");
		GET_ARG(b1, argv[2], "b1");
		GET_ARG(b2, argv[3], "b2");
		GET_ARG(a0, argv[4], "a0");
		GET_ARG(a1, argv[5], "a1");
		GET_ARG(a2, argv[6], "a2");
		break;
	default:
		LOG_FMT(LL_ERROR, "%s: BUG: unknown filter type: %s (%d)", __FILE__, argv[0], ei->effect_number);
		return NULL;
	}

	e = calloc(1, sizeof(struct effect));
	e->name = ei->name;
	e->istream.fs = e->ostream.fs = istream->fs;
	e->istream.channels = e->ostream.channels = istream->channels;
	e->run = biquad_effect_run;
	e->reset = biquad_effect_reset;
	e->plot = biquad_effect_plot;
	e->destroy = biquad_effect_destroy;
	state = calloc(istream->channels, sizeof(struct biquad_state *));
	for (i = 0; i < istream->channels; ++i) {
		if (GET_BIT(channel_selector, i)) {
			state[i] = calloc(1, sizeof(struct biquad_state));
			if (type == BIQUAD_BIQUAD)
				biquad_init(state[i], b0, b1, b2, a0, a1, a2);
			else
				biquad_init_using_type(state[i], type, istream->fs, arg0, arg1, arg2, arg3, width_type);
		}
	}
	e->data = state;
	return e;
}
