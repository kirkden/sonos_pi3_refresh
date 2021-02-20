#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ao/ao.h>
#include "ao.h"
#include "util.h"
#include "sampleconv.h"

struct ao_state {
	ao_device *dev;
	struct ao_enc_info *enc_info;
};

struct ao_enc_info {
	const char *name;
	int bytes, prec;
	void (*write_func)(sample_t *, char *, ssize_t);
};

static const char codec_name[] = "ao";
static int ao_open_count = 0;

static struct ao_enc_info encodings[] = {
	{ "s16", 2, 16, write_buf_s16 },
	{ "u8",  1, 8,  write_buf_u8 },
	{ "s32", 4, 32, write_buf_s32 },
};

static struct ao_enc_info * ao_get_enc_info(const char *enc)
{
	int i;
	if (enc == NULL)
		return &encodings[0];
	for (i = 0; i < LENGTH(encodings); ++i)
		if (strcmp(enc, encodings[i].name) == 0)
			return &encodings[i];
	return NULL;
}

ssize_t ao_read(struct codec *c, sample_t *buf, ssize_t frames)
{
	return 0;
}

ssize_t ao_write(struct codec *c, sample_t *buf, ssize_t frames)
{
	struct ao_state *state = (struct ao_state *) c->data;

	state->enc_info->write_func(buf, (char *) buf, frames * c->channels);
	if (ao_play(state->dev, (char *) buf, frames * c->channels * state->enc_info->bytes) == 0) {
		LOG_FMT(LL_ERROR, "%s: ao_play(): write failed", codec_name);
		return 0;
	}
	return frames;
}

ssize_t ao_seek(struct codec *c, ssize_t pos)
{
	return -1;
}

ssize_t ao_delay(struct codec *c)
{
	return 0;
}

void ao_drop(struct codec *c)
{
	/* do nothing */
}

void ao_pause(struct codec *c, int p)
{
	/* do nothing */
}

void ao_destroy(struct codec *c)
{
	struct ao_state *state = (struct ao_state *) c->data;
	ao_close(state->dev);
	--ao_open_count;
	if (ao_open_count == 0)
		ao_shutdown();
	free(state);
}

struct codec * ao_codec_init(const char *path, const char *type, const char *enc, int fs, int channels, int endian, int mode)
{
	int driver;
	struct ao_enc_info *enc_info;
	struct ao_sample_format format;
	ao_device *dev = NULL;
	struct ao_state *state = NULL;
	struct codec *c = NULL;

	if (ao_open_count == 0)
		ao_initialize();
	if ((driver = ao_default_driver_id()) == -1) {
		LOG_FMT(LL_OPEN_ERROR, "%s: error: failed get default driver id", codec_name);
		goto fail;
	}
	if ((enc_info = ao_get_enc_info(enc)) == NULL) {
		LOG_FMT(LL_ERROR, "%s: error: bad encoding: %s", codec_name, enc);
		goto fail;
	}
	format.bits = enc_info->prec;
	format.rate = fs;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = NULL;
	if ((dev = ao_open_live(driver, &format, NULL)) == NULL) {
		LOG_FMT(LL_OPEN_ERROR, "%s: error: could not open device", codec_name);
		goto fail;
	}

	state = calloc(1, sizeof(struct ao_state));
	state->dev = dev;
	state->enc_info = enc_info;

	c = calloc(1, sizeof(struct codec));
	c->path = path;
	c->type = type;
	c->enc = enc_info->name;
	c->fs = fs;
	c->channels = channels;
	c->prec = enc_info->prec;
	c->can_dither = 1;  /* all formats are fixed-point LPCM */
	c->interactive = 1;
	c->frames = -1;
	c->read = ao_read;
	c->write = ao_write;
	c->seek = ao_seek;
	c->delay = ao_delay;
	c->drop = ao_drop;
	c->pause = ao_pause;
	c->destroy = ao_destroy;
	c->data = state;

	++ao_open_count;

	return c;

	fail:
	if (ao_open_count == 0)
		ao_shutdown();
	return NULL;
}

void ao_codec_print_encodings(const char *type)
{
	int i;
	for (i = 0; i < LENGTH(encodings); ++i)
		fprintf(stdout, " %s", encodings[i].name);
}
