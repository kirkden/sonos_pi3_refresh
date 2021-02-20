#ifndef BIQUAD_H
#define BIQUAD_H

#define LN_2_2 0.34657359f // ln(2)/2

#include "ladspa-util.h"
#include <math.h>

#ifndef LIMIT
#define LIMIT(v,l,u) (v<l?l:(v>u?u:v))
#endif

// rt 8.1.2013: store biquad coefficients and accumulators in double
// precision, to limit effects of round-off error.  Change this to
// float if you need more speed: it probably won't make an audible
// difference unless you need filters with really high Q or low
// frequency.
#ifndef BIQUAD_TYPE
#define BIQUAD_TYPE double
#endif

// rt 25.6.2013: amplitude of square wave (at Nyquist freq)
// added to kill denormals ... may need to experiment with
// this value.  Note that 1.e-18 = -360dBFS
#define DENORMALKILLER 1.e-18;

typedef BIQUAD_TYPE bq_t;

/* Biquad filter (adapted from lisp code by Eli Brandt,
   http://www.cs.cmu.edu/~eli/) */

typedef struct {
	bq_t a1;
	bq_t a2;
	bq_t b0;
	bq_t b1;
	bq_t b2;
	bq_t x1;
	bq_t x2;
	bq_t y1;
	bq_t y2;
  bq_t dn;  // denormal state (rt 25.6.2013)
} biquad;

typedef struct {
	bq_t a1;
	bq_t b0;
	bq_t b1;
	bq_t x1;
	bq_t y1;
  bq_t dn;  // denormal state (rt 25.6.2013)
} bilin;


static inline void biquad_init(biquad *f) {
	f->x1 = 0.0f;
	f->x2 = 0.0f;
	f->y1 = 0.0f;
	f->y2 = 0.0f;
  f->dn = DENORMALKILLER;
}

static inline void bilin_init(bilin *f) {
	f->x1 = 0.0f;
	f->y1 = 0.0f;
  f->dn = DENORMALKILLER;
}

//static inline void eq_set_params(biquad *f, bq_t fc, bq_t gain, bq_t Q,
//			  bq_t fs);
static inline void eq_set_params(biquad *f, bq_t fc, bq_t gain, bq_t Q, bq_t fs)
{
	bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0f, fs/2.0f) / fs;
	bq_t J = pow(10.0f, gain * (bq_t) 0.025);
	bq_t cw = cos(w);
	bq_t sw = sin(w);
  Q = Q / J;  // adjust so input Q is standard elec. eng. Q
//	bq_t g = sw * sinhf(LN_2_2 * LIMIT(bw, 0.0001f, 4.0f) * w / sw); // case: bw
  bq_t g = sw/(2.0*Q);                                        // case: Q
//  bq_t g = sin(w0)/2.0 * sqrt( (J + 1.0/J)*(1.0/S - 1.0) + 2.0 )   // case: S

  bq_t a0 = 1.0f + g / J;
	bq_t a0r = 1.0f / a0;

	f->b0 = (1.0f + (g * J)) * a0r;
	f->b1 = (-2.0f * cw) * a0r;
	f->b2 = (1.0f - (g * J)) * a0r;
	f->a1 = -(f->b1);
	f->a2 = ((g / J) - 1.0f) * a0r;
}

//static inline void ls_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
//			  bq_t fs);
static inline void ls_set_params(biquad *f, bq_t fc, bq_t gain, bq_t Q,
			  bq_t fs)
{
	bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	bq_t cw = cos(w);
	bq_t sw = sin(w);
	bq_t A = powf(10.0f, (float) (gain * (bq_t) 0.025));

// rt 8.1.2013: increased slope limit from 1.0f to 5.0f:
//	bq_t b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 5.0f)) - ((A -
//					1.0f) * (A - 1.0)));

//	bq_t apc = cw * (A + 1.0f);
//	bq_t amc = cw * (A - 1.0f);
//	bq_t bs = b * sw;
//	bq_t a0r = 1.0f / (A + 1.0f + amc + bs);

//	f->b0 = a0r * A * (A + 1.0f - amc + bs);
//	f->b1 = a0r * 2.0f * A * (A - 1.0f - apc);
//	f->b2 = a0r * A * (A + 1.0f - amc - bs);
//	f->a1 = a0r * 2.0f * (A - 1.0f + apc);
//	f->a2 = a0r * (-A - 1.0f - amc + bs);

  // rt 22.1.2013 use Q instead of slope
  bq_t alpha = sw / (2.0f * Q);
  bq_t b = 2.0f * sqrt(A)*alpha;
  bq_t ap = A + 1.0f;
  bq_t am = A - 1.0f;
  bq_t a0 = ap + am*cw + b;
	bq_t a0r = 1.0f / a0;

	f->b0 = a0r *       A * (ap - am*cw + b);
	f->b1 = a0r * 2.0 * A * (am - ap*cw);
	f->b2 = a0r *       A * (ap - am*cw - b);
	f->a1 = 2.0f * a0r *     (am + ap*cw);
	f->a2 = -a0r *          (ap + am*cw - b);
}

//static inline void hs_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
//			  bq_t fs);
static inline void hs_set_params(biquad *f, bq_t fc, bq_t gain, bq_t Q, bq_t fs)
{
	bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	bq_t cw = cos(w);
	bq_t sw = sin(w);
	bq_t A = powf(10.0f, (float) (gain * (bq_t) 0.025f));
// rt 8.1.2013: increased slope limit from 1.0f to 5.0f:
//	bq_t b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 5.0f)) - ((A -
//					1.0f) * (A - 1.0f)));
//	bq_t apc = cw * (A + 1.0f);
//	bq_t amc = cw * (A - 1.0f);
//	bq_t bs = b * sw;
//	bq_t a0r = 1.0f / (A + 1.0f - amc + bs);

//	f->b0 = a0r * A * (A + 1.0f + amc + bs);
//	f->b1 = a0r * -2.0f * A * (A - 1.0f + apc);
//	f->b2 = a0r * A * (A + 1.0f + amc - bs);
//	f->a1 = a0r * -2.0f * (A - 1.0f - apc);
//	f->a2 = a0r * (-A - 1.0f + amc + bs);

  // rt 22.1.2013 use Q instead of slope
  bq_t alpha = sw / (2.0f * Q);
  bq_t b = 2.0f * sqrt(A)*alpha;
  bq_t ap = A + 1.0f;
  bq_t am = A - 1.0f;
  bq_t a0 = ap - am*cw + b;
	bq_t a0r = 1.0f / a0;

	f->b0 = a0r *       A * (ap + am*cw + b);
	f->b1 = -a0r * 2.0f * A * (am + ap*cw);
	f->b2 = a0r *       A * (ap + am*cw - b);
	f->a1 = -2.0f * a0r *    (am - ap*cw);
	f->a2 = -a0r *          (ap - am*cw - b);
}

static inline void lp_set_params(biquad *f, bq_t fc, bq_t Q, bq_t fs)
{
	bq_t omega = 2.0f * M_PI * fc/fs;
	bq_t sn = sin(omega);
	bq_t cs = cos(omega);
//	bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);
	bq_t alpha = sn / (2.0f * Q);

  const bq_t a0r = 1.0f / (1.0f + alpha);
  f->b0 = a0r * (1.0f - cs) * 0.5f;
	f->b1 = a0r * (1.0f - cs);
  f->b2 = a0r * (1.0f - cs) * 0.5f;
  f->a1 = a0r * (2.0f * cs);
  f->a2 = a0r * (alpha - 1.0f);
}

static inline void hp_set_params(biquad *f, bq_t fc, bq_t Q, bq_t fs)
{
	bq_t omega = 2.0f * M_PI * fc/fs;
	bq_t sn = sin(omega);
	bq_t cs = cos(omega);
//	bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);
	bq_t alpha = sn / (2.0f * Q);

  const bq_t a0r = 1.0f / (1.0f + alpha);
  f->b0 = a0r * (1.0f + cs) * 0.5f;
  f->b1 = a0r * -(1.0f + cs);
  f->b2 = a0r * (1.0f + cs) * 0.5f;
  f->a1 = a0r * (2.0f * cs);
  f->a2 = a0r * (alpha - 1.0f);
}

static inline void bp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs)
{
	bq_t omega = 2.0f * M_PI * fc/fs;
	bq_t sn = sin(omega);
	bq_t cs = cos(omega);
	bq_t alpha = sn * sinh(M_LN2 / 2.0f * bw * omega / sn);

  const bq_t a0r = 1.0f / (1.0f + alpha);
  f->b0 = a0r * alpha;
  f->b1 = 0.0f;
  f->b2 = a0r * -alpha;
  f->a1 = a0r * (2.0f * cs);
  f->a2 = a0r * (alpha - 1.0f);
}

// rt 8.1.2013: allpass from biquad cookbook
static inline void ap_set_params(biquad *f, bq_t fc, bq_t Q, bq_t fs)
{
	bq_t omega = 2.0f * M_PI * fc / fs;
	bq_t sn = sin(omega);
	bq_t cs = cos(omega);
//	bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);
	bq_t alpha = sn / (2.0f * Q);

  bq_t a0 = 1.0f + alpha;
  bq_t a0r = 1.0f / a0;
  f->b0 = a0r * (1.0f - alpha);
  f->b1 = a0r * (-2.0f * cs);
  f->b2 = 1.0;
  f->a1 = -f->b1;
  f->a2 = -f->b0;
}

static inline void ap1_set_params(bilin *f, bq_t fc, bq_t fs)
{
// rt 8.1.2013: implementation of first-order digital allpass found here:
//  https://ccrma.stanford.edu/realsimple/DelayVar/Phasing_First_Order_Allpass_Filters.html
// see here for turning transfer function into difference equation:
//  http://en.wikipedia.org/wiki/Digital_filter
  bq_t k = tan( M_PI * fc / fs);
  bq_t p = (1.0f - k) / (1.0f + k);

  // case 1: phase varies from 0 to 180 degrees
  f->b0 = -p;
  f->b1 = 1.0f;

  // case 2: phase varies from 180 to 360 degrees
//        f->b0 = p;
//        f->b1 = -1.0f;

   f->a1 = p;
}

static inline void lp1_set_params(bilin *f, bq_t fc, bq_t fs)
{
// rt 16.7.2015: 1st-order lowpass derived from bilinear transform
// of the analog transfer function H(s) = wc/(wc+s)
// See maxima/lowpass1.wxm for derivation.

  bq_t k = tan( M_PI * fc / fs);

  f->a1 = -( k - 1.0f )/( k + 1.0f );
  f->b0 = k/( k + 1.0f );
  f->b1 = f->b0;
}

static inline void hp1_set_params(bilin *f, bq_t fc, bq_t fs)
{
// rt 16.7.2015: 1st-order highpass derived from bilinear transform
// of the analog transfer function H(s) = s/(wc+s)
// See maxima/highpass1.wxm for derivation.

  bq_t k = tan( M_PI * fc / fs);

  f->a1 = -( k - 1.0f )/( k + 1.0f );
  f->b0 = 1.0f / ( k + 1.0f );
  f->b1 = -(f->b0);
}


// routine that runs a biquad (i.e. 2nd-order) digital filter
static inline bq_t biquad_run(biquad *f, const bq_t x) {
	bq_t y;

// rt 15.5.2013: add a Nyquist-frequency square-wave to kill denormals:
	y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		      + f->a1 * f->y1 + f->a2 * f->y2 + f->dn;
  f->dn = -f->dn;

// see previous commment
//	y = flush_to_zero(y);

	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

// rt  2.9.2013: special version of biquad_run that assumes the filter
// is an allpass; this takes advantage of factoring to reduce floating-point
// multiplications from 5 to 2
static inline bq_t ap2_run(biquad *f, const bq_t x) {
	bq_t y;

// rt 15.5.2013: add a Nyquist-frequency square-wave to kill denormals:
	y = f->b0 * (x - f->y2) + f->b1 * (f->x1 - f->y1) + f->x2 + f->dn;
  f->dn = -f->dn;

// see previous commment
//	y = flush_to_zero(y);

	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

// routine that runs a bilinear (i.e. 1st-order) digital filter
static inline bq_t bilin_run(bilin *f, const bq_t x) {
	bq_t y;

// rt 15.5.2013: add a Nyquist-frequency square-wave to kill denormals:
	y = f->b0 * x + f->b1 * f->x1 + f->a1 * f->y1 + f->dn;
  f->dn = -f->dn;

// see previous commment
//	y = flush_to_zero(y);

	f->x1 = x;
	f->y1 = y;

	return y;
}

// rt  2.9.2013: special version of bilin_run that assumes the filter
// is an allpass; takes advantage of factoring to reduce floating-point
// multiplications from 3 to 1
static inline bq_t ap1_run(bilin *f, const bq_t x) {
	bq_t y;

// rt 15.5.2013: adding a Nyquist-frequency square-wave to kill denormals:
	y = f->b0 * (x - f->y1) + f->x1 + f->dn;
  f->dn = -f->dn;

// see previous commment
//	y = flush_to_zero(y);

	f->x1 = x;
	f->y1 = y;

	return y;
}

#endif
