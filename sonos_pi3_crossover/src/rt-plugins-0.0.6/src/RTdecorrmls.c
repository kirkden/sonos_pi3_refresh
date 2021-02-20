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
*/

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <ladspa.h>

/* This is an experimental plugin [06.2016].  It takes two input channels,
   convolving one with an mls signal and convolving the other with the reverse
   of the same mls signal.  Since an mls signal and its reverse have low
   cross-correlation, this provides a degree of decorrelation (essentially
   by applying allpass filters that alter phase in a pseudo-random way).
   The two channels can optionally be summed to mono in ch.1.
*/

/* length of mls signal to convolve with; must be 512 or 1024 */
#define MLSLEN 1024

/* delay buffer length; must be a power of 2 and */
/* at least as long as filter length */
#define D_SIZE MLSLEN
#define MS (D_SIZE - 1)

#define DECORRMLS_MONO                0               
#define DECORRMLS_INPUT1              1
#define DECORRMLS_INPUT2              2
#define DECORRMLS_OUTPUT1             3
#define DECORRMLS_OUTPUT2             4

static LADSPA_Descriptor *decorrMLSDescriptor = NULL;

typedef struct {
  LADSPA_Data *mono;
	LADSPA_Data *input1;
	LADSPA_Data *input2;
	LADSPA_Data *output1;
	LADSPA_Data *output2;
  LADSPA_Data *d1;
  LADSPA_Data *d2;
  unsigned int dptr;
} DecorrMLS;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	switch (index) {
	case 0:
		return decorrMLSDescriptor;
	default:
		return NULL;
	}
}

static void cleanupDecorrMLS(LADSPA_Handle instance) {
	DecorrMLS *plugin_data = (DecorrMLS *)instance;
	free(plugin_data->d1);
	free(plugin_data->d2);
	free(instance);
}

static void connectPortDecorrMLS(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	DecorrMLS *plugin;

	plugin = (DecorrMLS *)instance;
	switch (port) {
	case DECORRMLS_MONO:
		plugin->mono = data;
		break;
	case DECORRMLS_INPUT1:
		plugin->input1 = data;
		break;
	case DECORRMLS_INPUT2:
		plugin->input2 = data;
		break;
	case DECORRMLS_OUTPUT1:
		plugin->output1 = data;
		break;
	case DECORRMLS_OUTPUT2:
		plugin->output2 = data;
		break;
	}
}

static LADSPA_Handle instantiateDecorrMLS(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	DecorrMLS *plugin_data = (DecorrMLS *)malloc(sizeof(DecorrMLS));
  LADSPA_Data *d1 = NULL;
  LADSPA_Data *d2 = NULL;
  unsigned int dptr;

  d1 = calloc(D_SIZE, sizeof(LADSPA_Data));
  d2 = calloc(D_SIZE, sizeof(LADSPA_Data));
  dptr = 0;

  plugin_data->d1 = d1;
  plugin_data->d2 = d2;
  plugin_data->dptr = dptr;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#define buffer_write(b, v) (b = v)

static void runDecorrMLS(LADSPA_Handle instance, unsigned long 
sample_count) {
	DecorrMLS *plugin_data = (DecorrMLS *)instance;

  /* Inputs (arrays of floats of length sample_count) */
  const LADSPA_Data * const input1 = plugin_data->input1;
  const LADSPA_Data * const input2 = plugin_data->input2;

  /* Outputs (arrays of floats of length sample_count) */
  LADSPA_Data * const output1 = plugin_data->output1;
  LADSPA_Data * const output2 = plugin_data->output2;

  /* Mono summing mode (float value) */
  const LADSPA_Data mono = *(plugin_data->mono);

  LADSPA_Data * d1 = plugin_data->d1;
  LADSPA_Data * d2 = plugin_data->d2;
  unsigned int dptr = plugin_data->dptr;

  unsigned long pos;
/*  unsigned int i;*/
  LADSPA_Data out1, out2, Lout, Rout;

  for (pos = 0; pos < sample_count; pos++) {
    // update buffers:
    d1[dptr] = input1[pos];
    d2[dptr] = input2[pos];
    // brute-force convolution; FFT would be faster
/*    out1 = 0.0f; out2 = 0.0f;*/
/*    for (i = 0; i < NTAPS; i++) { */
/*      out1 += (coeffs1[i] * d1[(dptr - i) & M_SIZE]);*/
/*      out2 += (coeffs2[i] * d2[(dptr - i) & M_SIZE]);*/
/*    }*/

// explicitly unroll sums... this runs 2x faster than convolution
// coefficients of MLS (and its reverse) and construction of the following expressions
// are done in piano_amp2/phasing/decorr1.R
#if MLSLEN == 512
out1 =
+d1[(dptr-0) & MS]+d1[(dptr-1) & MS]-d1[(dptr-2) & MS]-d1[(dptr-3) & MS]+d1[(dptr-4) & 
MS]-d1[(dptr-5) & MS]+d1[(dptr-6) & MS]+d1[(dptr-7) & MS]-d1[(dptr-8) & MS]+d1[(dptr-9) & 
MS]-d1[(dptr-10) & MS]-d1[(dptr-11) & MS]-d1[(dptr-12) & MS]-d1[(dptr-13) & 
MS]-d1[(dptr-14) & MS]-d1[(dptr-15) & MS]-d1[(dptr-16) & MS]+d1[(dptr-17) & 
MS]-d1[(dptr-18) & MS]+d1[(dptr-19) & MS]+d1[(dptr-20) & MS]+d1[(dptr-21) & 
MS]-d1[(dptr-22) & MS]+d1[(dptr-23) & MS]-d1[(dptr-24) & MS]-d1[(dptr-25) & 
MS]+d1[(dptr-26) & MS]+d1[(dptr-27) & MS]+d1[(dptr-28) & MS]-d1[(dptr-29) & 
MS]-d1[(dptr-30) & MS]-d1[(dptr-31) & MS]+d1[(dptr-32) & MS]-d1[(dptr-33) & 
MS]+d1[(dptr-34) & MS]-d1[(dptr-35) & MS]-d1[(dptr-36) & MS]+d1[(dptr-37) & 
MS]+d1[(dptr-38) & MS]-d1[(dptr-39) & MS]+d1[(dptr-40) & MS]-d1[(dptr-41) & 
MS]-d1[(dptr-42) & MS]+d1[(dptr-43) & MS]+d1[(dptr-44) & MS]-d1[(dptr-45) & 
MS]-d1[(dptr-46) & MS]-d1[(dptr-47) & MS]-d1[(dptr-48) & MS]+d1[(dptr-49) & 
MS]+d1[(dptr-50) & MS]+d1[(dptr-51) & MS]-d1[(dptr-52) & MS]-d1[(dptr-53) & 
MS]-d1[(dptr-54) & MS]-d1[(dptr-55) & MS]-d1[(dptr-56) & MS]+d1[(dptr-57) & 
MS]-d1[(dptr-58) & MS]-d1[(dptr-59) & MS]-d1[(dptr-60) & MS]+d1[(dptr-61) & 
MS]-d1[(dptr-62) & MS]+d1[(dptr-63) & MS]+d1[(dptr-64) & MS]+d1[(dptr-65) & 
MS]+d1[(dptr-66) & MS]+d1[(dptr-67) & MS]-d1[(dptr-68) & MS]-d1[(dptr-69) & 
MS]+d1[(dptr-70) & MS]-d1[(dptr-71) & MS]+d1[(dptr-72) & MS]-d1[(dptr-73) & 
MS]-d1[(dptr-74) & MS]+d1[(dptr-75) & MS]-d1[(dptr-76) & MS]-d1[(dptr-77) & 
MS]+d1[(dptr-78) & MS]-d1[(dptr-79) & MS]-d1[(dptr-80) & MS]-d1[(dptr-81) & 
MS]+d1[(dptr-82) & MS]-d1[(dptr-83) & MS]-d1[(dptr-84) & MS]+d1[(dptr-85) & 
MS]+d1[(dptr-86) & MS]+d1[(dptr-87) & MS]+d1[(dptr-88) & MS]+d1[(dptr-89) & 
MS]-d1[(dptr-90) & MS]+d1[(dptr-91) & MS]-d1[(dptr-92) & MS]-d1[(dptr-93) & 
MS]+d1[(dptr-94) & MS]-d1[(dptr-95) & MS]+d1[(dptr-96) & MS]-d1[(dptr-97) & 
MS]-d1[(dptr-98) & MS]-d1[(dptr-99) & MS]-d1[(dptr-100) & MS]-d1[(dptr-101) & 
MS]+d1[(dptr-102) & MS]-d1[(dptr-103) & MS]+d1[(dptr-104) & MS]-d1[(dptr-105) & 
MS]+d1[(dptr-106) & MS]-d1[(dptr-107) & MS]+d1[(dptr-108) & MS]-d1[(dptr-109) & 
MS]+d1[(dptr-110) & MS]+d1[(dptr-111) & MS]+d1[(dptr-112) & MS]+d1[(dptr-113) & 
MS]+d1[(dptr-114) & MS]+d1[(dptr-115) & MS]-d1[(dptr-116) & MS]+d1[(dptr-117) & 
MS]-d1[(dptr-118) & MS]+d1[(dptr-119) & MS]+d1[(dptr-120) & MS]-d1[(dptr-121) & 
MS]+d1[(dptr-122) & MS]-d1[(dptr-123) & MS]+d1[(dptr-124) & MS]-d1[(dptr-125) & 
MS]-d1[(dptr-126) & MS]-d1[(dptr-127) & MS]-d1[(dptr-128) & MS]+d1[(dptr-129) & 
MS]+d1[(dptr-130) & MS]-d1[(dptr-131) & MS]+d1[(dptr-132) & MS]-d1[(dptr-133) & 
MS]-d1[(dptr-134) & MS]-d1[(dptr-135) & MS]+d1[(dptr-136) & MS]-d1[(dptr-137) & 
MS]-d1[(dptr-138) & MS]-d1[(dptr-139) & MS]+d1[(dptr-140) & MS]+d1[(dptr-141) & 
MS]+d1[(dptr-142) & MS]+d1[(dptr-143) & MS]+d1[(dptr-144) & MS]+d1[(dptr-145) & 
MS]-d1[(dptr-146) & MS]-d1[(dptr-147) & MS]-d1[(dptr-148) & MS]+d1[(dptr-149) & 
MS]+d1[(dptr-150) & MS]-d1[(dptr-151) & MS]-d1[(dptr-152) & MS]-d1[(dptr-153) & 
MS]+d1[(dptr-154) & MS]-d1[(dptr-155) & MS]+d1[(dptr-156) & MS]+d1[(dptr-157) & 
MS]-d1[(dptr-158) & MS]+d1[(dptr-159) & MS]+d1[(dptr-160) & MS]-d1[(dptr-161) & 
MS]-d1[(dptr-162) & MS]-d1[(dptr-163) & MS]-d1[(dptr-164) & MS]+d1[(dptr-165) & 
MS]-d1[(dptr-166) & MS]+d1[(dptr-167) & MS]-d1[(dptr-168) & MS]-d1[(dptr-169) & 
MS]-d1[(dptr-170) & MS]+d1[(dptr-171) & MS]-d1[(dptr-172) & MS]+d1[(dptr-173) & 
MS]-d1[(dptr-174) & MS]+d1[(dptr-175) & MS]+d1[(dptr-176) & MS]+d1[(dptr-177) & 
MS]-d1[(dptr-178) & MS]+d1[(dptr-179) & MS]+d1[(dptr-180) & MS]-d1[(dptr-181) & 
MS]+d1[(dptr-182) & MS]+d1[(dptr-183) & MS]+d1[(dptr-184) & MS]+d1[(dptr-185) & 
MS]-d1[(dptr-186) & MS]-d1[(dptr-187) & MS]+d1[(dptr-188) & MS]+d1[(dptr-189) & 
MS]-d1[(dptr-190) & MS]-d1[(dptr-191) & MS]-d1[(dptr-192) & MS]+d1[(dptr-193) & 
MS]+d1[(dptr-194) & MS]+d1[(dptr-195) & MS]+d1[(dptr-196) & MS]-d1[(dptr-197) & 
MS]+d1[(dptr-198) & MS]-d1[(dptr-199) & MS]-d1[(dptr-200) & MS]-d1[(dptr-201) & 
MS]-d1[(dptr-202) & MS]+d1[(dptr-203) & MS]-d1[(dptr-204) & MS]-d1[(dptr-205) & 
MS]+d1[(dptr-206) & MS]-d1[(dptr-207) & MS]-d1[(dptr-208) & MS]+d1[(dptr-209) & 
MS]+d1[(dptr-210) & MS]-d1[(dptr-211) & MS]-d1[(dptr-212) & MS]+d1[(dptr-213) & 
MS]-d1[(dptr-214) & MS]+d1[(dptr-215) & MS]+d1[(dptr-216) & MS]+d1[(dptr-217) & 
MS]+d1[(dptr-218) & MS]-d1[(dptr-219) & MS]-d1[(dptr-220) & MS]-d1[(dptr-221) & 
MS]+d1[(dptr-222) & MS]-d1[(dptr-223) & MS]-d1[(dptr-224) & MS]-d1[(dptr-225) & 
MS]-d1[(dptr-226) & MS]+d1[(dptr-227) & MS]+d1[(dptr-228) & MS]+d1[(dptr-229) & 
MS]+d1[(dptr-230) & MS]-d1[(dptr-231) & MS]-d1[(dptr-232) & MS]-d1[(dptr-233) & 
MS]-d1[(dptr-234) & MS]-d1[(dptr-235) & MS]-d1[(dptr-236) & MS]-d1[(dptr-237) & 
MS]-d1[(dptr-238) & MS]-d1[(dptr-239) & MS]+d1[(dptr-240) & MS]+d1[(dptr-241) & 
MS]+d1[(dptr-242) & MS]+d1[(dptr-243) & MS]+d1[(dptr-244) & MS]-d1[(dptr-245) & 
MS]-d1[(dptr-246) & MS]-d1[(dptr-247) & MS]-d1[(dptr-248) & MS]+d1[(dptr-249) & 
MS]-d1[(dptr-250) & MS]-d1[(dptr-251) & MS]-d1[(dptr-252) & MS]-d1[(dptr-253) & 
MS]-d1[(dptr-254) & MS]+d1[(dptr-255) & MS]+d1[(dptr-256) & MS]+d1[(dptr-257) & 
MS]-d1[(dptr-258) & MS]+d1[(dptr-259) & MS]-d1[(dptr-260) & MS]-d1[(dptr-261) & 
MS]-d1[(dptr-262) & MS]+d1[(dptr-263) & MS]+d1[(dptr-264) & MS]-d1[(dptr-265) & 
MS]-d1[(dptr-266) & MS]+d1[(dptr-267) & MS]+d1[(dptr-268) & MS]-d1[(dptr-269) & 
MS]+d1[(dptr-270) & MS]+d1[(dptr-271) & MS]+d1[(dptr-272) & MS]+d1[(dptr-273) & 
MS]+d1[(dptr-274) & MS]-d1[(dptr-275) & MS]+d1[(dptr-276) & MS]+d1[(dptr-277) & 
MS]-d1[(dptr-278) & MS]+d1[(dptr-279) & MS]-d1[(dptr-280) & MS]+d1[(dptr-281) & 
MS]+d1[(dptr-282) & MS]-d1[(dptr-283) & MS]-d1[(dptr-284) & MS]-d1[(dptr-285) & 
MS]+d1[(dptr-286) & MS]-d1[(dptr-287) & MS]-d1[(dptr-288) & MS]+d1[(dptr-289) & 
MS]-d1[(dptr-290) & MS]+d1[(dptr-291) & MS]+d1[(dptr-292) & MS]+d1[(dptr-293) & 
MS]-d1[(dptr-294) & MS]-d1[(dptr-295) & MS]-d1[(dptr-296) & MS]-d1[(dptr-297) & 
MS]+d1[(dptr-298) & MS]+d1[(dptr-299) & MS]-d1[(dptr-300) & MS]-d1[(dptr-301) & 
MS]-d1[(dptr-302) & MS]-d1[(dptr-303) & MS]-d1[(dptr-304) & MS]+d1[(dptr-305) & 
MS]+d1[(dptr-306) & MS]-d1[(dptr-307) & MS]-d1[(dptr-308) & MS]+d1[(dptr-309) & 
MS]-d1[(dptr-310) & MS]-d1[(dptr-311) & MS]+d1[(dptr-312) & MS]+d1[(dptr-313) & 
MS]+d1[(dptr-314) & MS]-d1[(dptr-315) & MS]+d1[(dptr-316) & MS]-d1[(dptr-317) & 
MS]+d1[(dptr-318) & MS]-d1[(dptr-319) & MS]+d1[(dptr-320) & MS]+d1[(dptr-321) & 
MS]-d1[(dptr-322) & MS]+d1[(dptr-323) & MS]+d1[(dptr-324) & MS]+d1[(dptr-325) & 
MS]-d1[(dptr-326) & MS]-d1[(dptr-327) & MS]-d1[(dptr-328) & MS]+d1[(dptr-329) & 
MS]+d1[(dptr-330) & MS]+d1[(dptr-331) & MS]-d1[(dptr-332) & MS]-d1[(dptr-333) & 
MS]+d1[(dptr-334) & MS]-d1[(dptr-335) & MS]-d1[(dptr-336) & MS]+d1[(dptr-337) & 
MS]-d1[(dptr-338) & MS]+d1[(dptr-339) & MS]-d1[(dptr-340) & MS]+d1[(dptr-341) & 
MS]-d1[(dptr-342) & MS]-d1[(dptr-343) & MS]-d1[(dptr-344) & MS]+d1[(dptr-345) & 
MS]+d1[(dptr-346) & MS]+d1[(dptr-347) & MS]-d1[(dptr-348) & MS]+d1[(dptr-349) & 
MS]+d1[(dptr-350) & MS]-d1[(dptr-351) & MS]-d1[(dptr-352) & MS]+d1[(dptr-353) & 
MS]+d1[(dptr-354) & MS]+d1[(dptr-355) & MS]-d1[(dptr-356) & MS]+d1[(dptr-357) & 
MS]+d1[(dptr-358) & MS]+d1[(dptr-359) & MS]-d1[(dptr-360) & MS]+d1[(dptr-361) & 
MS]+d1[(dptr-362) & MS]+d1[(dptr-363) & MS]+d1[(dptr-364) & MS]+d1[(dptr-365) & 
MS]+d1[(dptr-366) & MS]+d1[(dptr-367) & MS]+d1[(dptr-368) & MS]-d1[(dptr-369) & 
MS]+d1[(dptr-370) & MS]+d1[(dptr-371) & MS]+d1[(dptr-372) & MS]+d1[(dptr-373) & 
MS]-d1[(dptr-374) & MS]+d1[(dptr-375) & MS]+d1[(dptr-376) & MS]+d1[(dptr-377) & 
MS]-d1[(dptr-378) & MS]-d1[(dptr-379) & MS]+d1[(dptr-380) & MS]+d1[(dptr-381) & 
MS]+d1[(dptr-382) & MS]+d1[(dptr-383) & MS]-d1[(dptr-384) & MS]+d1[(dptr-385) & 
MS]+d1[(dptr-386) & MS]-d1[(dptr-387) & MS]-d1[(dptr-388) & MS]-d1[(dptr-389) & 
MS]+d1[(dptr-390) & MS]+d1[(dptr-391) & MS]-d1[(dptr-392) & MS]+d1[(dptr-393) & 
MS]-d1[(dptr-394) & MS]+d1[(dptr-395) & MS]-d1[(dptr-396) & MS]+d1[(dptr-397) & 
MS]-d1[(dptr-398) & MS]-d1[(dptr-399) & MS]+d1[(dptr-400) & MS]+d1[(dptr-401) & 
MS]+d1[(dptr-402) & MS]+d1[(dptr-403) & MS]-d1[(dptr-404) & MS]-d1[(dptr-405) & 
MS]+d1[(dptr-406) & MS]-d1[(dptr-407) & MS]-d1[(dptr-408) & MS]-d1[(dptr-409) & 
MS]-d1[(dptr-410) & MS]+d1[(dptr-411) & MS]-d1[(dptr-412) & MS]+d1[(dptr-413) & 
MS]+d1[(dptr-414) & MS]-d1[(dptr-415) & MS]-d1[(dptr-416) & MS]+d1[(dptr-417) & 
MS]-d1[(dptr-418) & MS]-d1[(dptr-419) & MS]-d1[(dptr-420) & MS]+d1[(dptr-421) & 
MS]+d1[(dptr-422) & MS]-d1[(dptr-423) & MS]+d1[(dptr-424) & MS]+d1[(dptr-425) & 
MS]+d1[(dptr-426) & MS]-d1[(dptr-427) & MS]+d1[(dptr-428) & MS]-d1[(dptr-429) & 
MS]+d1[(dptr-430) & MS]+d1[(dptr-431) & MS]+d1[(dptr-432) & MS]+d1[(dptr-433) & 
MS]-d1[(dptr-434) & MS]+d1[(dptr-435) & MS]-d1[(dptr-436) & MS]+d1[(dptr-437) & 
MS]-d1[(dptr-438) & MS]-d1[(dptr-439) & MS]+d1[(dptr-440) & MS]-d1[(dptr-441) & 
MS]+d1[(dptr-442) & MS]+d1[(dptr-443) & MS]-d1[(dptr-444) & MS]-d1[(dptr-445) & 
MS]-d1[(dptr-446) & MS]-d1[(dptr-447) & MS]-d1[(dptr-448) & MS]-d1[(dptr-449) & 
MS]+d1[(dptr-450) & MS]-d1[(dptr-451) & MS]-d1[(dptr-452) & MS]+d1[(dptr-453) & 
MS]+d1[(dptr-454) & MS]-d1[(dptr-455) & MS]+d1[(dptr-456) & MS]+d1[(dptr-457) & 
MS]-d1[(dptr-458) & MS]+d1[(dptr-459) & MS]+d1[(dptr-460) & MS]-d1[(dptr-461) & 
MS]+d1[(dptr-462) & MS]-d1[(dptr-463) & MS]-d1[(dptr-464) & MS]+d1[(dptr-465) & 
MS]-d1[(dptr-466) & MS]-d1[(dptr-467) & MS]-d1[(dptr-468) & MS]-d1[(dptr-469) & 
MS]-d1[(dptr-470) & MS]-d1[(dptr-471) & MS]+d1[(dptr-472) & MS]+d1[(dptr-473) & 
MS]-d1[(dptr-474) & MS]+d1[(dptr-475) & MS]+d1[(dptr-476) & MS]-d1[(dptr-477) & 
MS]-d1[(dptr-478) & MS]+d1[(dptr-479) & MS]-d1[(dptr-480) & MS]+d1[(dptr-481) & 
MS]-d1[(dptr-482) & MS]+d1[(dptr-483) & MS]+d1[(dptr-484) & MS]-d1[(dptr-485) & 
MS]-d1[(dptr-486) & MS]+d1[(dptr-487) & MS]+d1[(dptr-488) & MS]-d1[(dptr-489) & 
MS]-d1[(dptr-490) & MS]+d1[(dptr-491) & MS]+d1[(dptr-492) & MS]+d1[(dptr-493) & 
MS]+d1[(dptr-494) & MS]+d1[(dptr-495) & MS]+d1[(dptr-496) & MS]+d1[(dptr-497) & 
MS]-d1[(dptr-498) & MS]-d1[(dptr-499) & MS]+d1[(dptr-500) & MS]+d1[(dptr-501) & 
MS]+d1[(dptr-502) & MS]-d1[(dptr-503) & MS]-d1[(dptr-504) & MS]+d1[(dptr-505) & 
MS]+d1[(dptr-506) & MS]-d1[(dptr-507) & MS]+d1[(dptr-508) & MS]-d1[(dptr-509) & 
MS]+d1[(dptr-510) & MS]-d1[(dptr-511) & MS];
out2 =
+d2[(dptr-0) & MS]-d2[(dptr-1) & MS]+d2[(dptr-2) & MS]-d2[(dptr-3) & MS]+d2[(dptr-4) & 
MS]+d2[(dptr-5) & MS]-d2[(dptr-6) & MS]-d2[(dptr-7) & MS]+d2[(dptr-8) & MS]+d2[(dptr-9) & 
MS]+d2[(dptr-10) & MS]-d2[(dptr-11) & MS]-d2[(dptr-12) & MS]+d2[(dptr-13) & 
MS]+d2[(dptr-14) & MS]+d2[(dptr-15) & MS]+d2[(dptr-16) & MS]+d2[(dptr-17) & 
MS]+d2[(dptr-18) & MS]+d2[(dptr-19) & MS]-d2[(dptr-20) & MS]-d2[(dptr-21) & 
MS]+d2[(dptr-22) & MS]+d2[(dptr-23) & MS]-d2[(dptr-24) & MS]-d2[(dptr-25) & 
MS]+d2[(dptr-26) & MS]+d2[(dptr-27) & MS]-d2[(dptr-28) & MS]+d2[(dptr-29) & 
MS]-d2[(dptr-30) & MS]+d2[(dptr-31) & MS]-d2[(dptr-32) & MS]-d2[(dptr-33) & 
MS]+d2[(dptr-34) & MS]+d2[(dptr-35) & MS]-d2[(dptr-36) & MS]+d2[(dptr-37) & 
MS]+d2[(dptr-38) & MS]-d2[(dptr-39) & MS]-d2[(dptr-40) & MS]-d2[(dptr-41) & 
MS]-d2[(dptr-42) & MS]-d2[(dptr-43) & MS]-d2[(dptr-44) & MS]+d2[(dptr-45) & 
MS]-d2[(dptr-46) & MS]-d2[(dptr-47) & MS]+d2[(dptr-48) & MS]-d2[(dptr-49) & 
MS]+d2[(dptr-50) & MS]+d2[(dptr-51) & MS]-d2[(dptr-52) & MS]+d2[(dptr-53) & 
MS]+d2[(dptr-54) & MS]-d2[(dptr-55) & MS]+d2[(dptr-56) & MS]+d2[(dptr-57) & 
MS]-d2[(dptr-58) & MS]-d2[(dptr-59) & MS]+d2[(dptr-60) & MS]-d2[(dptr-61) & 
MS]-d2[(dptr-62) & MS]-d2[(dptr-63) & MS]-d2[(dptr-64) & MS]-d2[(dptr-65) & 
MS]-d2[(dptr-66) & MS]+d2[(dptr-67) & MS]+d2[(dptr-68) & MS]-d2[(dptr-69) & 
MS]+d2[(dptr-70) & MS]-d2[(dptr-71) & MS]-d2[(dptr-72) & MS]+d2[(dptr-73) & 
MS]-d2[(dptr-74) & MS]+d2[(dptr-75) & MS]-d2[(dptr-76) & MS]+d2[(dptr-77) & 
MS]+d2[(dptr-78) & MS]+d2[(dptr-79) & MS]+d2[(dptr-80) & MS]-d2[(dptr-81) & 
MS]+d2[(dptr-82) & MS]-d2[(dptr-83) & MS]+d2[(dptr-84) & MS]+d2[(dptr-85) & 
MS]+d2[(dptr-86) & MS]-d2[(dptr-87) & MS]+d2[(dptr-88) & MS]+d2[(dptr-89) & 
MS]-d2[(dptr-90) & MS]-d2[(dptr-91) & MS]-d2[(dptr-92) & MS]+d2[(dptr-93) & 
MS]-d2[(dptr-94) & MS]-d2[(dptr-95) & MS]+d2[(dptr-96) & MS]+d2[(dptr-97) & 
MS]-d2[(dptr-98) & MS]+d2[(dptr-99) & MS]-d2[(dptr-100) & MS]-d2[(dptr-101) & 
MS]-d2[(dptr-102) & MS]-d2[(dptr-103) & MS]+d2[(dptr-104) & MS]-d2[(dptr-105) & 
MS]-d2[(dptr-106) & MS]+d2[(dptr-107) & MS]+d2[(dptr-108) & MS]+d2[(dptr-109) & 
MS]+d2[(dptr-110) & MS]-d2[(dptr-111) & MS]-d2[(dptr-112) & MS]+d2[(dptr-113) & 
MS]-d2[(dptr-114) & MS]+d2[(dptr-115) & MS]-d2[(dptr-116) & MS]+d2[(dptr-117) & 
MS]-d2[(dptr-118) & MS]+d2[(dptr-119) & MS]+d2[(dptr-120) & MS]-d2[(dptr-121) & 
MS]-d2[(dptr-122) & MS]-d2[(dptr-123) & MS]+d2[(dptr-124) & MS]+d2[(dptr-125) & 
MS]-d2[(dptr-126) & MS]+d2[(dptr-127) & MS]+d2[(dptr-128) & MS]+d2[(dptr-129) & 
MS]+d2[(dptr-130) & MS]-d2[(dptr-131) & MS]-d2[(dptr-132) & MS]+d2[(dptr-133) & 
MS]+d2[(dptr-134) & MS]+d2[(dptr-135) & MS]-d2[(dptr-136) & MS]+d2[(dptr-137) & 
MS]+d2[(dptr-138) & MS]+d2[(dptr-139) & MS]+d2[(dptr-140) & MS]-d2[(dptr-141) & 
MS]+d2[(dptr-142) & MS]+d2[(dptr-143) & MS]+d2[(dptr-144) & MS]+d2[(dptr-145) & 
MS]+d2[(dptr-146) & MS]+d2[(dptr-147) & MS]+d2[(dptr-148) & MS]+d2[(dptr-149) & 
MS]-d2[(dptr-150) & MS]+d2[(dptr-151) & MS]+d2[(dptr-152) & MS]+d2[(dptr-153) & 
MS]-d2[(dptr-154) & MS]+d2[(dptr-155) & MS]+d2[(dptr-156) & MS]+d2[(dptr-157) & 
MS]-d2[(dptr-158) & MS]-d2[(dptr-159) & MS]+d2[(dptr-160) & MS]+d2[(dptr-161) & 
MS]-d2[(dptr-162) & MS]+d2[(dptr-163) & MS]+d2[(dptr-164) & MS]+d2[(dptr-165) & 
MS]-d2[(dptr-166) & MS]-d2[(dptr-167) & MS]-d2[(dptr-168) & MS]+d2[(dptr-169) & 
MS]-d2[(dptr-170) & MS]+d2[(dptr-171) & MS]-d2[(dptr-172) & MS]+d2[(dptr-173) & 
MS]-d2[(dptr-174) & MS]-d2[(dptr-175) & MS]+d2[(dptr-176) & MS]-d2[(dptr-177) & 
MS]-d2[(dptr-178) & MS]+d2[(dptr-179) & MS]+d2[(dptr-180) & MS]+d2[(dptr-181) & 
MS]-d2[(dptr-182) & MS]-d2[(dptr-183) & MS]-d2[(dptr-184) & MS]+d2[(dptr-185) & 
MS]+d2[(dptr-186) & MS]+d2[(dptr-187) & MS]-d2[(dptr-188) & MS]+d2[(dptr-189) & 
MS]+d2[(dptr-190) & MS]-d2[(dptr-191) & MS]+d2[(dptr-192) & MS]-d2[(dptr-193) & 
MS]+d2[(dptr-194) & MS]-d2[(dptr-195) & MS]+d2[(dptr-196) & MS]+d2[(dptr-197) & 
MS]+d2[(dptr-198) & MS]-d2[(dptr-199) & MS]-d2[(dptr-200) & MS]+d2[(dptr-201) & 
MS]-d2[(dptr-202) & MS]-d2[(dptr-203) & MS]+d2[(dptr-204) & MS]+d2[(dptr-205) & 
MS]-d2[(dptr-206) & MS]-d2[(dptr-207) & MS]-d2[(dptr-208) & MS]-d2[(dptr-209) & 
MS]-d2[(dptr-210) & MS]+d2[(dptr-211) & MS]+d2[(dptr-212) & MS]-d2[(dptr-213) & 
MS]-d2[(dptr-214) & MS]-d2[(dptr-215) & MS]-d2[(dptr-216) & MS]+d2[(dptr-217) & 
MS]+d2[(dptr-218) & MS]+d2[(dptr-219) & MS]-d2[(dptr-220) & MS]+d2[(dptr-221) & 
MS]-d2[(dptr-222) & MS]-d2[(dptr-223) & MS]+d2[(dptr-224) & MS]-d2[(dptr-225) & 
MS]-d2[(dptr-226) & MS]-d2[(dptr-227) & MS]+d2[(dptr-228) & MS]+d2[(dptr-229) & 
MS]-d2[(dptr-230) & MS]+d2[(dptr-231) & MS]-d2[(dptr-232) & MS]+d2[(dptr-233) & 
MS]+d2[(dptr-234) & MS]-d2[(dptr-235) & MS]+d2[(dptr-236) & MS]+d2[(dptr-237) & 
MS]+d2[(dptr-238) & MS]+d2[(dptr-239) & MS]+d2[(dptr-240) & MS]-d2[(dptr-241) & 
MS]+d2[(dptr-242) & MS]+d2[(dptr-243) & MS]-d2[(dptr-244) & MS]-d2[(dptr-245) & 
MS]+d2[(dptr-246) & MS]+d2[(dptr-247) & MS]-d2[(dptr-248) & MS]-d2[(dptr-249) & 
MS]-d2[(dptr-250) & MS]+d2[(dptr-251) & MS]-d2[(dptr-252) & MS]+d2[(dptr-253) & 
MS]+d2[(dptr-254) & MS]+d2[(dptr-255) & MS]-d2[(dptr-256) & MS]-d2[(dptr-257) & 
MS]-d2[(dptr-258) & MS]-d2[(dptr-259) & MS]-d2[(dptr-260) & MS]+d2[(dptr-261) & 
MS]-d2[(dptr-262) & MS]-d2[(dptr-263) & MS]-d2[(dptr-264) & MS]-d2[(dptr-265) & 
MS]+d2[(dptr-266) & MS]+d2[(dptr-267) & MS]+d2[(dptr-268) & MS]+d2[(dptr-269) & 
MS]+d2[(dptr-270) & MS]-d2[(dptr-271) & MS]-d2[(dptr-272) & MS]-d2[(dptr-273) & 
MS]-d2[(dptr-274) & MS]-d2[(dptr-275) & MS]-d2[(dptr-276) & MS]-d2[(dptr-277) & 
MS]-d2[(dptr-278) & MS]-d2[(dptr-279) & MS]+d2[(dptr-280) & MS]+d2[(dptr-281) & 
MS]+d2[(dptr-282) & MS]+d2[(dptr-283) & MS]-d2[(dptr-284) & MS]-d2[(dptr-285) & 
MS]-d2[(dptr-286) & MS]-d2[(dptr-287) & MS]+d2[(dptr-288) & MS]-d2[(dptr-289) & 
MS]-d2[(dptr-290) & MS]-d2[(dptr-291) & MS]+d2[(dptr-292) & MS]+d2[(dptr-293) & 
MS]+d2[(dptr-294) & MS]+d2[(dptr-295) & MS]-d2[(dptr-296) & MS]+d2[(dptr-297) & 
MS]-d2[(dptr-298) & MS]-d2[(dptr-299) & MS]+d2[(dptr-300) & MS]+d2[(dptr-301) & 
MS]-d2[(dptr-302) & MS]-d2[(dptr-303) & MS]+d2[(dptr-304) & MS]-d2[(dptr-305) & 
MS]-d2[(dptr-306) & MS]+d2[(dptr-307) & MS]-d2[(dptr-308) & MS]-d2[(dptr-309) & 
MS]-d2[(dptr-310) & MS]-d2[(dptr-311) & MS]+d2[(dptr-312) & MS]-d2[(dptr-313) & 
MS]+d2[(dptr-314) & MS]+d2[(dptr-315) & MS]+d2[(dptr-316) & MS]+d2[(dptr-317) & 
MS]-d2[(dptr-318) & MS]-d2[(dptr-319) & MS]-d2[(dptr-320) & MS]+d2[(dptr-321) & 
MS]+d2[(dptr-322) & MS]-d2[(dptr-323) & MS]-d2[(dptr-324) & MS]+d2[(dptr-325) & 
MS]+d2[(dptr-326) & MS]+d2[(dptr-327) & MS]+d2[(dptr-328) & MS]-d2[(dptr-329) & 
MS]+d2[(dptr-330) & MS]+d2[(dptr-331) & MS]-d2[(dptr-332) & MS]+d2[(dptr-333) & 
MS]+d2[(dptr-334) & MS]+d2[(dptr-335) & MS]-d2[(dptr-336) & MS]+d2[(dptr-337) & 
MS]-d2[(dptr-338) & MS]+d2[(dptr-339) & MS]-d2[(dptr-340) & MS]-d2[(dptr-341) & 
MS]-d2[(dptr-342) & MS]+d2[(dptr-343) & MS]-d2[(dptr-344) & MS]+d2[(dptr-345) & 
MS]-d2[(dptr-346) & MS]-d2[(dptr-347) & MS]-d2[(dptr-348) & MS]-d2[(dptr-349) & 
MS]+d2[(dptr-350) & MS]+d2[(dptr-351) & MS]-d2[(dptr-352) & MS]+d2[(dptr-353) & 
MS]+d2[(dptr-354) & MS]-d2[(dptr-355) & MS]+d2[(dptr-356) & MS]-d2[(dptr-357) & 
MS]-d2[(dptr-358) & MS]-d2[(dptr-359) & MS]+d2[(dptr-360) & MS]+d2[(dptr-361) & 
MS]-d2[(dptr-362) & MS]-d2[(dptr-363) & MS]-d2[(dptr-364) & MS]+d2[(dptr-365) & 
MS]+d2[(dptr-366) & MS]+d2[(dptr-367) & MS]+d2[(dptr-368) & MS]+d2[(dptr-369) & 
MS]+d2[(dptr-370) & MS]-d2[(dptr-371) & MS]-d2[(dptr-372) & MS]-d2[(dptr-373) & 
MS]+d2[(dptr-374) & MS]-d2[(dptr-375) & MS]-d2[(dptr-376) & MS]-d2[(dptr-377) & 
MS]+d2[(dptr-378) & MS]-d2[(dptr-379) & MS]+d2[(dptr-380) & MS]+d2[(dptr-381) & 
MS]-d2[(dptr-382) & MS]-d2[(dptr-383) & MS]-d2[(dptr-384) & MS]-d2[(dptr-385) & 
MS]+d2[(dptr-386) & MS]-d2[(dptr-387) & MS]+d2[(dptr-388) & MS]-d2[(dptr-389) & 
MS]+d2[(dptr-390) & MS]+d2[(dptr-391) & MS]-d2[(dptr-392) & MS]+d2[(dptr-393) & 
MS]-d2[(dptr-394) & MS]+d2[(dptr-395) & MS]+d2[(dptr-396) & MS]+d2[(dptr-397) & 
MS]+d2[(dptr-398) & MS]+d2[(dptr-399) & MS]+d2[(dptr-400) & MS]-d2[(dptr-401) & 
MS]+d2[(dptr-402) & MS]-d2[(dptr-403) & MS]+d2[(dptr-404) & MS]-d2[(dptr-405) & 
MS]+d2[(dptr-406) & MS]-d2[(dptr-407) & MS]+d2[(dptr-408) & MS]-d2[(dptr-409) & 
MS]-d2[(dptr-410) & MS]-d2[(dptr-411) & MS]-d2[(dptr-412) & MS]-d2[(dptr-413) & 
MS]+d2[(dptr-414) & MS]-d2[(dptr-415) & MS]+d2[(dptr-416) & MS]-d2[(dptr-417) & 
MS]-d2[(dptr-418) & MS]+d2[(dptr-419) & MS]-d2[(dptr-420) & MS]+d2[(dptr-421) & 
MS]+d2[(dptr-422) & MS]+d2[(dptr-423) & MS]+d2[(dptr-424) & MS]+d2[(dptr-425) & 
MS]-d2[(dptr-426) & MS]-d2[(dptr-427) & MS]+d2[(dptr-428) & MS]-d2[(dptr-429) & 
MS]-d2[(dptr-430) & MS]-d2[(dptr-431) & MS]+d2[(dptr-432) & MS]-d2[(dptr-433) & 
MS]-d2[(dptr-434) & MS]+d2[(dptr-435) & MS]-d2[(dptr-436) & MS]-d2[(dptr-437) & 
MS]+d2[(dptr-438) & MS]-d2[(dptr-439) & MS]+d2[(dptr-440) & MS]-d2[(dptr-441) & 
MS]-d2[(dptr-442) & MS]+d2[(dptr-443) & MS]+d2[(dptr-444) & MS]+d2[(dptr-445) & 
MS]+d2[(dptr-446) & MS]+d2[(dptr-447) & MS]-d2[(dptr-448) & MS]+d2[(dptr-449) & 
MS]-d2[(dptr-450) & MS]-d2[(dptr-451) & MS]-d2[(dptr-452) & MS]+d2[(dptr-453) & 
MS]-d2[(dptr-454) & MS]-d2[(dptr-455) & MS]-d2[(dptr-456) & MS]-d2[(dptr-457) & 
MS]-d2[(dptr-458) & MS]+d2[(dptr-459) & MS]+d2[(dptr-460) & MS]+d2[(dptr-461) & 
MS]-d2[(dptr-462) & MS]-d2[(dptr-463) & MS]-d2[(dptr-464) & MS]-d2[(dptr-465) & 
MS]+d2[(dptr-466) & MS]+d2[(dptr-467) & MS]-d2[(dptr-468) & MS]-d2[(dptr-469) & 
MS]+d2[(dptr-470) & MS]-d2[(dptr-471) & MS]+d2[(dptr-472) & MS]+d2[(dptr-473) & 
MS]-d2[(dptr-474) & MS]-d2[(dptr-475) & MS]+d2[(dptr-476) & MS]-d2[(dptr-477) & 
MS]+d2[(dptr-478) & MS]-d2[(dptr-479) & MS]-d2[(dptr-480) & MS]-d2[(dptr-481) & 
MS]+d2[(dptr-482) & MS]+d2[(dptr-483) & MS]+d2[(dptr-484) & MS]-d2[(dptr-485) & 
MS]-d2[(dptr-486) & MS]+d2[(dptr-487) & MS]-d2[(dptr-488) & MS]+d2[(dptr-489) & 
MS]+d2[(dptr-490) & MS]+d2[(dptr-491) & MS]-d2[(dptr-492) & MS]+d2[(dptr-493) & 
MS]-d2[(dptr-494) & MS]-d2[(dptr-495) & MS]-d2[(dptr-496) & MS]-d2[(dptr-497) & 
MS]-d2[(dptr-498) & MS]-d2[(dptr-499) & MS]-d2[(dptr-500) & MS]+d2[(dptr-501) & 
MS]-d2[(dptr-502) & MS]+d2[(dptr-503) & MS]+d2[(dptr-504) & MS]-d2[(dptr-505) & 
MS]+d2[(dptr-506) & MS]-d2[(dptr-507) & MS]-d2[(dptr-508) & MS]+d2[(dptr-509) & 
MS]+d2[(dptr-510) & MS]-d2[(dptr-511) & MS];
    // scale to prevent clipping;
    // factor experimentally determined to match rms level of input
    out1 *= 0.075437f;
    out2 *= 0.075437f;
#else
out1 =
-d1[(dptr-0) & MS]-d1[(dptr-1) & MS]+d1[(dptr-2) & MS]-d1[(dptr-3) & MS]+d1[(dptr-4) & 
MS]+d1[(dptr-5) & MS]-d1[(dptr-6) & MS]-d1[(dptr-7) & MS]-d1[(dptr-8) & MS]-d1[(dptr-9) & 
MS]+d1[(dptr-10) & MS]-d1[(dptr-11) & MS]+d1[(dptr-12) & MS]+d1[(dptr-13) & 
MS]-d1[(dptr-14) & MS]-d1[(dptr-15) & MS]+d1[(dptr-16) & MS]-d1[(dptr-17) & 
MS]+d1[(dptr-18) & MS]-d1[(dptr-19) & MS]+d1[(dptr-20) & MS]+d1[(dptr-21) & 
MS]-d1[(dptr-22) & MS]+d1[(dptr-23) & MS]+d1[(dptr-24) & MS]-d1[(dptr-25) & 
MS]-d1[(dptr-26) & MS]-d1[(dptr-27) & MS]+d1[(dptr-28) & MS]+d1[(dptr-29) & 
MS]+d1[(dptr-30) & MS]+d1[(dptr-31) & MS]+d1[(dptr-32) & MS]-d1[(dptr-33) & 
MS]-d1[(dptr-34) & MS]-d1[(dptr-35) & MS]-d1[(dptr-36) & MS]-d1[(dptr-37) & 
MS]+d1[(dptr-38) & MS]+d1[(dptr-39) & MS]-d1[(dptr-40) & MS]-d1[(dptr-41) & 
MS]-d1[(dptr-42) & MS]+d1[(dptr-43) & MS]+d1[(dptr-44) & MS]-d1[(dptr-45) & 
MS]-d1[(dptr-46) & MS]+d1[(dptr-47) & MS]-d1[(dptr-48) & MS]-d1[(dptr-49) & 
MS]-d1[(dptr-50) & MS]-d1[(dptr-51) & MS]+d1[(dptr-52) & MS]-d1[(dptr-53) & 
MS]+d1[(dptr-54) & MS]+d1[(dptr-55) & MS]+d1[(dptr-56) & MS]-d1[(dptr-57) & 
MS]+d1[(dptr-58) & MS]-d1[(dptr-59) & MS]+d1[(dptr-60) & MS]-d1[(dptr-61) & 
MS]+d1[(dptr-62) & MS]-d1[(dptr-63) & MS]-d1[(dptr-64) & MS]+d1[(dptr-65) & 
MS]-d1[(dptr-66) & MS]-d1[(dptr-67) & MS]-d1[(dptr-68) & MS]-d1[(dptr-69) & 
MS]-d1[(dptr-70) & MS]+d1[(dptr-71) & MS]+d1[(dptr-72) & MS]+d1[(dptr-73) & 
MS]+d1[(dptr-74) & MS]-d1[(dptr-75) & MS]+d1[(dptr-76) & MS]+d1[(dptr-77) & 
MS]-d1[(dptr-78) & MS]-d1[(dptr-79) & MS]-d1[(dptr-80) & MS]+d1[(dptr-81) & 
MS]-d1[(dptr-82) & MS]+d1[(dptr-83) & MS]+d1[(dptr-84) & MS]+d1[(dptr-85) & 
MS]-d1[(dptr-86) & MS]-d1[(dptr-87) & MS]-d1[(dptr-88) & MS]+d1[(dptr-89) & 
MS]-d1[(dptr-90) & MS]+d1[(dptr-91) & MS]-d1[(dptr-92) & MS]-d1[(dptr-93) & 
MS]-d1[(dptr-94) & MS]-d1[(dptr-95) & MS]-d1[(dptr-96) & MS]+d1[(dptr-97) & 
MS]-d1[(dptr-98) & MS]-d1[(dptr-99) & MS]+d1[(dptr-100) & MS]-d1[(dptr-101) & 
MS]+d1[(dptr-102) & MS]+d1[(dptr-103) & MS]-d1[(dptr-104) & MS]+d1[(dptr-105) & 
MS]+d1[(dptr-106) & MS]+d1[(dptr-107) & MS]+d1[(dptr-108) & MS]-d1[(dptr-109) & 
MS]+d1[(dptr-110) & MS]+d1[(dptr-111) & MS]+d1[(dptr-112) & MS]+d1[(dptr-113) & 
MS]-d1[(dptr-114) & MS]+d1[(dptr-115) & MS]-d1[(dptr-116) & MS]+d1[(dptr-117) & 
MS]+d1[(dptr-118) & MS]-d1[(dptr-119) & MS]+d1[(dptr-120) & MS]-d1[(dptr-121) & 
MS]+d1[(dptr-122) & MS]-d1[(dptr-123) & MS]-d1[(dptr-124) & MS]+d1[(dptr-125) & 
MS]+d1[(dptr-126) & MS]+d1[(dptr-127) & MS]-d1[(dptr-128) & MS]-d1[(dptr-129) & 
MS]-d1[(dptr-130) & MS]+d1[(dptr-131) & MS]+d1[(dptr-132) & MS]-d1[(dptr-133) & 
MS]-d1[(dptr-134) & MS]-d1[(dptr-135) & MS]-d1[(dptr-136) & MS]-d1[(dptr-137) & 
MS]-d1[(dptr-138) & MS]-d1[(dptr-139) & MS]+d1[(dptr-140) & MS]-d1[(dptr-141) & 
MS]-d1[(dptr-142) & MS]+d1[(dptr-143) & MS]+d1[(dptr-144) & MS]+d1[(dptr-145) & 
MS]+d1[(dptr-146) & MS]-d1[(dptr-147) & MS]+d1[(dptr-148) & MS]+d1[(dptr-149) & 
MS]+d1[(dptr-150) & MS]-d1[(dptr-151) & MS]-d1[(dptr-152) & MS]+d1[(dptr-153) & 
MS]-d1[(dptr-154) & MS]+d1[(dptr-155) & MS]+d1[(dptr-156) & MS]-d1[(dptr-157) & 
MS]-d1[(dptr-158) & MS]-d1[(dptr-159) & MS]+d1[(dptr-160) & MS]+d1[(dptr-161) & 
MS]-d1[(dptr-162) & MS]+d1[(dptr-163) & MS]+d1[(dptr-164) & MS]-d1[(dptr-165) & 
MS]-d1[(dptr-166) & MS]-d1[(dptr-167) & MS]-d1[(dptr-168) & MS]+d1[(dptr-169) & 
MS]+d1[(dptr-170) & MS]+d1[(dptr-171) & MS]+d1[(dptr-172) & MS]-d1[(dptr-173) & 
MS]-d1[(dptr-174) & MS]+d1[(dptr-175) & MS]-d1[(dptr-176) & MS]-d1[(dptr-177) & 
MS]-d1[(dptr-178) & MS]+d1[(dptr-179) & MS]-d1[(dptr-180) & MS]-d1[(dptr-181) & 
MS]+d1[(dptr-182) & MS]+d1[(dptr-183) & MS]+d1[(dptr-184) & MS]-d1[(dptr-185) & 
MS]-d1[(dptr-186) & MS]+d1[(dptr-187) & MS]+d1[(dptr-188) & MS]+d1[(dptr-189) & 
MS]-d1[(dptr-190) & MS]-d1[(dptr-191) & MS]-d1[(dptr-192) & MS]-d1[(dptr-193) & 
MS]+d1[(dptr-194) & MS]-d1[(dptr-195) & MS]-d1[(dptr-196) & MS]-d1[(dptr-197) & 
MS]-d1[(dptr-198) & MS]-d1[(dptr-199) & MS]+d1[(dptr-200) & MS]-d1[(dptr-201) & 
MS]+d1[(dptr-202) & MS]+d1[(dptr-203) & MS]-d1[(dptr-204) & MS]+d1[(dptr-205) & 
MS]+d1[(dptr-206) & MS]-d1[(dptr-207) & MS]+d1[(dptr-208) & MS]-d1[(dptr-209) & 
MS]+d1[(dptr-210) & MS]+d1[(dptr-211) & MS]+d1[(dptr-212) & MS]+d1[(dptr-213) & 
MS]+d1[(dptr-214) & MS]+d1[(dptr-215) & MS]-d1[(dptr-216) & MS]-d1[(dptr-217) & 
MS]+d1[(dptr-218) & MS]-d1[(dptr-219) & MS]+d1[(dptr-220) & MS]+d1[(dptr-221) & 
MS]+d1[(dptr-222) & MS]-d1[(dptr-223) & MS]-d1[(dptr-224) & MS]+d1[(dptr-225) & 
MS]+d1[(dptr-226) & MS]-d1[(dptr-227) & MS]+d1[(dptr-228) & MS]-d1[(dptr-229) & 
MS]-d1[(dptr-230) & MS]-d1[(dptr-231) & MS]+d1[(dptr-232) & MS]-d1[(dptr-233) & 
MS]+d1[(dptr-234) & MS]+d1[(dptr-235) & MS]-d1[(dptr-236) & MS]+d1[(dptr-237) & 
MS]-d1[(dptr-238) & MS]-d1[(dptr-239) & MS]+d1[(dptr-240) & MS]-d1[(dptr-241) & 
MS]+d1[(dptr-242) & MS]+d1[(dptr-243) & MS]+d1[(dptr-244) & MS]-d1[(dptr-245) & 
MS]+d1[(dptr-246) & MS]+d1[(dptr-247) & MS]+d1[(dptr-248) & MS]-d1[(dptr-249) & 
MS]+d1[(dptr-250) & MS]-d1[(dptr-251) & MS]-d1[(dptr-252) & MS]+d1[(dptr-253) & 
MS]+d1[(dptr-254) & MS]-d1[(dptr-255) & MS]-d1[(dptr-256) & MS]+d1[(dptr-257) & 
MS]-d1[(dptr-258) & MS]+d1[(dptr-259) & MS]+d1[(dptr-260) & MS]-d1[(dptr-261) & 
MS]+d1[(dptr-262) & MS]-d1[(dptr-263) & MS]+d1[(dptr-264) & MS]+d1[(dptr-265) & 
MS]-d1[(dptr-266) & MS]+d1[(dptr-267) & MS]+d1[(dptr-268) & MS]+d1[(dptr-269) & 
MS]-d1[(dptr-270) & MS]-d1[(dptr-271) & MS]+d1[(dptr-272) & MS]+d1[(dptr-273) & 
MS]+d1[(dptr-274) & MS]+d1[(dptr-275) & MS]-d1[(dptr-276) & MS]-d1[(dptr-277) & 
MS]-d1[(dptr-278) & MS]+d1[(dptr-279) & MS]-d1[(dptr-280) & MS]-d1[(dptr-281) & 
MS]+d1[(dptr-282) & MS]-d1[(dptr-283) & MS]-d1[(dptr-284) & MS]-d1[(dptr-285) & 
MS]-d1[(dptr-286) & MS]+d1[(dptr-287) & MS]+d1[(dptr-288) & MS]+d1[(dptr-289) & 
MS]+d1[(dptr-290) & MS]+d1[(dptr-291) & MS]-d1[(dptr-292) & MS]+d1[(dptr-293) & 
MS]-d1[(dptr-294) & MS]-d1[(dptr-295) & MS]-d1[(dptr-296) & MS]+d1[(dptr-297) & 
MS]+d1[(dptr-298) & MS]-d1[(dptr-299) & MS]+d1[(dptr-300) & MS]-d1[(dptr-301) & 
MS]+d1[(dptr-302) & MS]-d1[(dptr-303) & MS]-d1[(dptr-304) & MS]-d1[(dptr-305) & 
MS]+d1[(dptr-306) & MS]+d1[(dptr-307) & MS]-d1[(dptr-308) & MS]-d1[(dptr-309) & 
MS]-d1[(dptr-310) & MS]+d1[(dptr-311) & MS]-d1[(dptr-312) & MS]-d1[(dptr-313) & 
MS]-d1[(dptr-314) & MS]+d1[(dptr-315) & MS]-d1[(dptr-316) & MS]-d1[(dptr-317) & 
MS]-d1[(dptr-318) & MS]+d1[(dptr-319) & MS]+d1[(dptr-320) & MS]-d1[(dptr-321) & 
MS]-d1[(dptr-322) & MS]+d1[(dptr-323) & MS]+d1[(dptr-324) & MS]-d1[(dptr-325) & 
MS]-d1[(dptr-326) & MS]-d1[(dptr-327) & MS]+d1[(dptr-328) & MS]-d1[(dptr-329) & 
MS]+d1[(dptr-330) & MS]-d1[(dptr-331) & MS]+d1[(dptr-332) & MS]-d1[(dptr-333) & 
MS]-d1[(dptr-334) & MS]-d1[(dptr-335) & MS]+d1[(dptr-336) & MS]-d1[(dptr-337) & 
MS]-d1[(dptr-338) & MS]-d1[(dptr-339) & MS]-d1[(dptr-340) & MS]+d1[(dptr-341) & 
MS]-d1[(dptr-342) & MS]-d1[(dptr-343) & MS]+d1[(dptr-344) & MS]+d1[(dptr-345) & 
MS]-d1[(dptr-346) & MS]+d1[(dptr-347) & MS]-d1[(dptr-348) & MS]+d1[(dptr-349) & 
MS]+d1[(dptr-350) & MS]+d1[(dptr-351) & MS]-d1[(dptr-352) & MS]+d1[(dptr-353) & 
MS]+d1[(dptr-354) & MS]-d1[(dptr-355) & MS]-d1[(dptr-356) & MS]+d1[(dptr-357) & 
MS]-d1[(dptr-358) & MS]-d1[(dptr-359) & MS]+d1[(dptr-360) & MS]+d1[(dptr-361) & 
MS]+d1[(dptr-362) & MS]-d1[(dptr-363) & MS]+d1[(dptr-364) & MS]+d1[(dptr-365) & 
MS]+d1[(dptr-366) & MS]+d1[(dptr-367) & MS]-d1[(dptr-368) & MS]-d1[(dptr-369) & 
MS]-d1[(dptr-370) & MS]+d1[(dptr-371) & MS]+d1[(dptr-372) & MS]-d1[(dptr-373) & 
MS]+d1[(dptr-374) & MS]-d1[(dptr-375) & MS]-d1[(dptr-376) & MS]-d1[(dptr-377) & 
MS]-d1[(dptr-378) & MS]-d1[(dptr-379) & MS]+d1[(dptr-380) & MS]+d1[(dptr-381) & 
MS]-d1[(dptr-382) & MS]+d1[(dptr-383) & MS]-d1[(dptr-384) & MS]+d1[(dptr-385) & 
MS]+d1[(dptr-386) & MS]-d1[(dptr-387) & MS]-d1[(dptr-388) & MS]+d1[(dptr-389) & 
MS]+d1[(dptr-390) & MS]-d1[(dptr-391) & MS]-d1[(dptr-392) & MS]+d1[(dptr-393) & 
MS]+d1[(dptr-394) & MS]-d1[(dptr-395) & MS]+d1[(dptr-396) & MS]-d1[(dptr-397) & 
MS]+d1[(dptr-398) & MS]-d1[(dptr-399) & MS]+d1[(dptr-400) & MS]-d1[(dptr-401) & 
MS]+d1[(dptr-402) & MS]+d1[(dptr-403) & MS]-d1[(dptr-404) & MS]-d1[(dptr-405) & 
MS]-d1[(dptr-406) & MS]-d1[(dptr-407) & MS]-d1[(dptr-408) & MS]-d1[(dptr-409) & 
MS]+d1[(dptr-410) & MS]+d1[(dptr-411) & MS]-d1[(dptr-412) & MS]-d1[(dptr-413) & 
MS]+d1[(dptr-414) & MS]+d1[(dptr-415) & MS]+d1[(dptr-416) & MS]-d1[(dptr-417) & 
MS]-d1[(dptr-418) & MS]+d1[(dptr-419) & MS]-d1[(dptr-420) & MS]+d1[(dptr-421) & 
MS]-d1[(dptr-422) & MS]-d1[(dptr-423) & MS]-d1[(dptr-424) & MS]-d1[(dptr-425) & 
MS]+d1[(dptr-426) & MS]+d1[(dptr-427) & MS]-d1[(dptr-428) & MS]-d1[(dptr-429) & 
MS]+d1[(dptr-430) & MS]-d1[(dptr-431) & MS]+d1[(dptr-432) & MS]-d1[(dptr-433) & 
MS]-d1[(dptr-434) & MS]+d1[(dptr-435) & MS]-d1[(dptr-436) & MS]+d1[(dptr-437) & 
MS]+d1[(dptr-438) & MS]-d1[(dptr-439) & MS]-d1[(dptr-440) & MS]+d1[(dptr-441) & 
MS]+d1[(dptr-442) & MS]+d1[(dptr-443) & MS]-d1[(dptr-444) & MS]+d1[(dptr-445) & 
MS]+d1[(dptr-446) & MS]-d1[(dptr-447) & MS]+d1[(dptr-448) & MS]-d1[(dptr-449) & 
MS]-d1[(dptr-450) & MS]-d1[(dptr-451) & MS]+d1[(dptr-452) & MS]+d1[(dptr-453) & 
MS]+d1[(dptr-454) & MS]+d1[(dptr-455) & MS]-d1[(dptr-456) & MS]+d1[(dptr-457) & 
MS]-d1[(dptr-458) & MS]-d1[(dptr-459) & MS]-d1[(dptr-460) & MS]-d1[(dptr-461) & 
MS]+d1[(dptr-462) & MS]-d1[(dptr-463) & MS]+d1[(dptr-464) & MS]-d1[(dptr-465) & 
MS]+d1[(dptr-466) & MS]-d1[(dptr-467) & MS]+d1[(dptr-468) & MS]-d1[(dptr-469) & 
MS]+d1[(dptr-470) & MS]-d1[(dptr-471) & MS]-d1[(dptr-472) & MS]-d1[(dptr-473) & 
MS]-d1[(dptr-474) & MS]-d1[(dptr-475) & MS]-d1[(dptr-476) & MS]-d1[(dptr-477) & 
MS]-d1[(dptr-478) & MS]+d1[(dptr-479) & MS]-d1[(dptr-480) & MS]+d1[(dptr-481) & 
MS]+d1[(dptr-482) & MS]+d1[(dptr-483) & MS]+d1[(dptr-484) & MS]+d1[(dptr-485) & 
MS]-d1[(dptr-486) & MS]+d1[(dptr-487) & MS]-d1[(dptr-488) & MS]+d1[(dptr-489) & 
MS]-d1[(dptr-490) & MS]+d1[(dptr-491) & MS]+d1[(dptr-492) & MS]-d1[(dptr-493) & 
MS]+d1[(dptr-494) & MS]-d1[(dptr-495) & MS]-d1[(dptr-496) & MS]-d1[(dptr-497) & 
MS]-d1[(dptr-498) & MS]+d1[(dptr-499) & MS]+d1[(dptr-500) & MS]+d1[(dptr-501) & 
MS]-d1[(dptr-502) & MS]+d1[(dptr-503) & MS]-d1[(dptr-504) & MS]+d1[(dptr-505) & 
MS]-d1[(dptr-506) & MS]-d1[(dptr-507) & MS]-d1[(dptr-508) & MS]-d1[(dptr-509) & 
MS]+d1[(dptr-510) & MS]-d1[(dptr-511) & MS]-d1[(dptr-512) & MS]-d1[(dptr-513) & 
MS]+d1[(dptr-514) & MS]-d1[(dptr-515) & MS]+d1[(dptr-516) & MS]-d1[(dptr-517) & 
MS]+d1[(dptr-518) & MS]+d1[(dptr-519) & MS]-d1[(dptr-520) & MS]-d1[(dptr-521) & 
MS]+d1[(dptr-522) & MS]-d1[(dptr-523) & MS]-d1[(dptr-524) & MS]-d1[(dptr-525) & 
MS]+d1[(dptr-526) & MS]+d1[(dptr-527) & MS]-d1[(dptr-528) & MS]+d1[(dptr-529) & 
MS]+d1[(dptr-530) & MS]+d1[(dptr-531) & MS]-d1[(dptr-532) & MS]-d1[(dptr-533) & 
MS]-d1[(dptr-534) & MS]+d1[(dptr-535) & MS]+d1[(dptr-536) & MS]+d1[(dptr-537) & 
MS]-d1[(dptr-538) & MS]-d1[(dptr-539) & MS]-d1[(dptr-540) & MS]-d1[(dptr-541) & 
MS]-d1[(dptr-542) & MS]-d1[(dptr-543) & MS]-d1[(dptr-544) & MS]-d1[(dptr-545) & 
MS]-d1[(dptr-546) & MS]-d1[(dptr-547) & MS]+d1[(dptr-548) & MS]+d1[(dptr-549) & 
MS]+d1[(dptr-550) & MS]+d1[(dptr-551) & MS]+d1[(dptr-552) & MS]+d1[(dptr-553) & 
MS]+d1[(dptr-554) & MS]-d1[(dptr-555) & MS]-d1[(dptr-556) & MS]-d1[(dptr-557) & 
MS]+d1[(dptr-558) & MS]+d1[(dptr-559) & MS]+d1[(dptr-560) & MS]+d1[(dptr-561) & 
MS]-d1[(dptr-562) & MS]-d1[(dptr-563) & MS]-d1[(dptr-564) & MS]-d1[(dptr-565) & 
MS]-d1[(dptr-566) & MS]-d1[(dptr-567) & MS]+d1[(dptr-568) & MS]-d1[(dptr-569) & 
MS]-d1[(dptr-570) & MS]-d1[(dptr-571) & MS]+d1[(dptr-572) & MS]+d1[(dptr-573) & 
MS]+d1[(dptr-574) & MS]-d1[(dptr-575) & MS]+d1[(dptr-576) & MS]+d1[(dptr-577) & 
MS]-d1[(dptr-578) & MS]-d1[(dptr-579) & MS]-d1[(dptr-580) & MS]-d1[(dptr-581) & 
MS]-d1[(dptr-582) & MS]+d1[(dptr-583) & MS]+d1[(dptr-584) & MS]+d1[(dptr-585) & 
MS]-d1[(dptr-586) & MS]-d1[(dptr-587) & MS]+d1[(dptr-588) & MS]+d1[(dptr-589) & 
MS]-d1[(dptr-590) & MS]-d1[(dptr-591) & MS]-d1[(dptr-592) & MS]-d1[(dptr-593) & 
MS]-d1[(dptr-594) & MS]+d1[(dptr-595) & MS]-d1[(dptr-596) & MS]+d1[(dptr-597) & 
MS]-d1[(dptr-598) & MS]-d1[(dptr-599) & MS]+d1[(dptr-600) & MS]+d1[(dptr-601) & 
MS]-d1[(dptr-602) & MS]+d1[(dptr-603) & MS]-d1[(dptr-604) & MS]-d1[(dptr-605) & 
MS]+d1[(dptr-606) & MS]+d1[(dptr-607) & MS]-d1[(dptr-608) & MS]+d1[(dptr-609) & 
MS]+d1[(dptr-610) & MS]-d1[(dptr-611) & MS]+d1[(dptr-612) & MS]+d1[(dptr-613) & 
MS]-d1[(dptr-614) & MS]+d1[(dptr-615) & MS]+d1[(dptr-616) & MS]+d1[(dptr-617) & 
MS]+d1[(dptr-618) & MS]+d1[(dptr-619) & MS]+d1[(dptr-620) & MS]+d1[(dptr-621) & 
MS]+d1[(dptr-622) & MS]+d1[(dptr-623) & MS]-d1[(dptr-624) & MS]+d1[(dptr-625) & 
MS]+d1[(dptr-626) & MS]+d1[(dptr-627) & MS]+d1[(dptr-628) & MS]+d1[(dptr-629) & 
MS]+d1[(dptr-630) & MS]-d1[(dptr-631) & MS]+d1[(dptr-632) & MS]+d1[(dptr-633) & 
MS]-d1[(dptr-634) & MS]+d1[(dptr-635) & MS]+d1[(dptr-636) & MS]+d1[(dptr-637) & 
MS]-d1[(dptr-638) & MS]+d1[(dptr-639) & MS]+d1[(dptr-640) & MS]+d1[(dptr-641) & 
MS]+d1[(dptr-642) & MS]+d1[(dptr-643) & MS]-d1[(dptr-644) & MS]-d1[(dptr-645) & 
MS]+d1[(dptr-646) & MS]+d1[(dptr-647) & MS]-d1[(dptr-648) & MS]+d1[(dptr-649) & 
MS]+d1[(dptr-650) & MS]-d1[(dptr-651) & MS]-d1[(dptr-652) & MS]+d1[(dptr-653) & 
MS]-d1[(dptr-654) & MS]+d1[(dptr-655) & MS]+d1[(dptr-656) & MS]+d1[(dptr-657) & 
MS]+d1[(dptr-658) & MS]-d1[(dptr-659) & MS]+d1[(dptr-660) & MS]+d1[(dptr-661) & 
MS]-d1[(dptr-662) & MS]+d1[(dptr-663) & MS]-d1[(dptr-664) & MS]+d1[(dptr-665) & 
MS]-d1[(dptr-666) & MS]+d1[(dptr-667) & MS]+d1[(dptr-668) & MS]+d1[(dptr-669) & 
MS]+d1[(dptr-670) & MS]-d1[(dptr-671) & MS]-d1[(dptr-672) & MS]-d1[(dptr-673) & 
MS]-d1[(dptr-674) & MS]+d1[(dptr-675) & MS]-d1[(dptr-676) & MS]+d1[(dptr-677) & 
MS]-d1[(dptr-678) & MS]-d1[(dptr-679) & MS]-d1[(dptr-680) & MS]+d1[(dptr-681) & 
MS]-d1[(dptr-682) & MS]+d1[(dptr-683) & MS]-d1[(dptr-684) & MS]-d1[(dptr-685) & 
MS]+d1[(dptr-686) & MS]-d1[(dptr-687) & MS]-d1[(dptr-688) & MS]+d1[(dptr-689) & 
MS]-d1[(dptr-690) & MS]-d1[(dptr-691) & MS]+d1[(dptr-692) & MS]+d1[(dptr-693) & 
MS]+d1[(dptr-694) & MS]+d1[(dptr-695) & MS]+d1[(dptr-696) & MS]+d1[(dptr-697) & 
MS]+d1[(dptr-698) & MS]+d1[(dptr-699) & MS]-d1[(dptr-700) & MS]-d1[(dptr-701) & 
MS]+d1[(dptr-702) & MS]+d1[(dptr-703) & MS]+d1[(dptr-704) & MS]+d1[(dptr-705) & 
MS]+d1[(dptr-706) & MS]-d1[(dptr-707) & MS]-d1[(dptr-708) & MS]+d1[(dptr-709) & 
MS]-d1[(dptr-710) & MS]-d1[(dptr-711) & MS]+d1[(dptr-712) & MS]+d1[(dptr-713) & 
MS]-d1[(dptr-714) & MS]-d1[(dptr-715) & MS]+d1[(dptr-716) & MS]+d1[(dptr-717) & 
MS]+d1[(dptr-718) & MS]+d1[(dptr-719) & MS]-d1[(dptr-720) & MS]+d1[(dptr-721) & 
MS]-d1[(dptr-722) & MS]+d1[(dptr-723) & MS]-d1[(dptr-724) & MS]-d1[(dptr-725) & 
MS]+d1[(dptr-726) & MS]-d1[(dptr-727) & MS]+d1[(dptr-728) & MS]-d1[(dptr-729) & 
MS]-d1[(dptr-730) & MS]-d1[(dptr-731) & MS]+d1[(dptr-732) & MS]+d1[(dptr-733) & 
MS]+d1[(dptr-734) & MS]-d1[(dptr-735) & MS]-d1[(dptr-736) & MS]+d1[(dptr-737) & 
MS]-d1[(dptr-738) & MS]-d1[(dptr-739) & MS]-d1[(dptr-740) & MS]-d1[(dptr-741) & 
MS]-d1[(dptr-742) & MS]-d1[(dptr-743) & MS]+d1[(dptr-744) & MS]+d1[(dptr-745) & 
MS]+d1[(dptr-746) & MS]-d1[(dptr-747) & MS]+d1[(dptr-748) & MS]+d1[(dptr-749) & 
MS]+d1[(dptr-750) & MS]-d1[(dptr-751) & MS]-d1[(dptr-752) & MS]-d1[(dptr-753) & 
MS]-d1[(dptr-754) & MS]+d1[(dptr-755) & MS]+d1[(dptr-756) & MS]-d1[(dptr-757) & 
MS]-d1[(dptr-758) & MS]-d1[(dptr-759) & MS]-d1[(dptr-760) & MS]+d1[(dptr-761) & 
MS]-d1[(dptr-762) & MS]-d1[(dptr-763) & MS]+d1[(dptr-764) & MS]-d1[(dptr-765) & 
MS]-d1[(dptr-766) & MS]+d1[(dptr-767) & MS]-d1[(dptr-768) & MS]+d1[(dptr-769) & 
MS]+d1[(dptr-770) & MS]+d1[(dptr-771) & MS]+d1[(dptr-772) & MS]+d1[(dptr-773) & 
MS]+d1[(dptr-774) & MS]+d1[(dptr-775) & MS]-d1[(dptr-776) & MS]+d1[(dptr-777) & 
MS]-d1[(dptr-778) & MS]+d1[(dptr-779) & MS]+d1[(dptr-780) & MS]+d1[(dptr-781) & 
MS]+d1[(dptr-782) & MS]-d1[(dptr-783) & MS]+d1[(dptr-784) & MS]-d1[(dptr-785) & 
MS]-d1[(dptr-786) & MS]+d1[(dptr-787) & MS]-d1[(dptr-788) & MS]+d1[(dptr-789) & 
MS]-d1[(dptr-790) & MS]+d1[(dptr-791) & MS]-d1[(dptr-792) & MS]+d1[(dptr-793) & 
MS]+d1[(dptr-794) & MS]+d1[(dptr-795) & MS]-d1[(dptr-796) & MS]-d1[(dptr-797) & 
MS]-d1[(dptr-798) & MS]-d1[(dptr-799) & MS]-d1[(dptr-800) & MS]+d1[(dptr-801) & 
MS]-d1[(dptr-802) & MS]-d1[(dptr-803) & MS]-d1[(dptr-804) & MS]-d1[(dptr-805) & 
MS]+d1[(dptr-806) & MS]+d1[(dptr-807) & MS]-d1[(dptr-808) & MS]+d1[(dptr-809) & 
MS]+d1[(dptr-810) & MS]-d1[(dptr-811) & MS]+d1[(dptr-812) & MS]-d1[(dptr-813) & 
MS]-d1[(dptr-814) & MS]+d1[(dptr-815) & MS]+d1[(dptr-816) & MS]+d1[(dptr-817) & 
MS]+d1[(dptr-818) & MS]+d1[(dptr-819) & MS]-d1[(dptr-820) & MS]+d1[(dptr-821) & 
MS]+d1[(dptr-822) & MS]-d1[(dptr-823) & MS]-d1[(dptr-824) & MS]+d1[(dptr-825) & 
MS]+d1[(dptr-826) & MS]-d1[(dptr-827) & MS]+d1[(dptr-828) & MS]+d1[(dptr-829) & 
MS]+d1[(dptr-830) & MS]-d1[(dptr-831) & MS]+d1[(dptr-832) & MS]-d1[(dptr-833) & 
MS]+d1[(dptr-834) & MS]+d1[(dptr-835) & MS]+d1[(dptr-836) & MS]-d1[(dptr-837) & 
MS]-d1[(dptr-838) & MS]+d1[(dptr-839) & MS]-d1[(dptr-840) & MS]-d1[(dptr-841) & 
MS]+d1[(dptr-842) & MS]-d1[(dptr-843) & MS]-d1[(dptr-844) & MS]-d1[(dptr-845) & 
MS]+d1[(dptr-846) & MS]+d1[(dptr-847) & MS]+d1[(dptr-848) & MS]+d1[(dptr-849) & 
MS]+d1[(dptr-850) & MS]+d1[(dptr-851) & MS]-d1[(dptr-852) & MS]-d1[(dptr-853) & 
MS]-d1[(dptr-854) & MS]-d1[(dptr-855) & MS]+d1[(dptr-856) & MS]+d1[(dptr-857) & 
MS]+d1[(dptr-858) & MS]-d1[(dptr-859) & MS]-d1[(dptr-860) & MS]-d1[(dptr-861) & 
MS]+d1[(dptr-862) & MS]-d1[(dptr-863) & MS]-d1[(dptr-864) & MS]-d1[(dptr-865) & 
MS]-d1[(dptr-866) & MS]-d1[(dptr-867) & MS]-d1[(dptr-868) & MS]-d1[(dptr-869) & 
MS]+d1[(dptr-870) & MS]+d1[(dptr-871) & MS]-d1[(dptr-872) & MS]+d1[(dptr-873) & 
MS]+d1[(dptr-874) & MS]+d1[(dptr-875) & MS]+d1[(dptr-876) & MS]-d1[(dptr-877) & 
MS]-d1[(dptr-878) & MS]+d1[(dptr-879) & MS]+d1[(dptr-880) & MS]+d1[(dptr-881) & 
MS]-d1[(dptr-882) & MS]+d1[(dptr-883) & MS]-d1[(dptr-884) & MS]-d1[(dptr-885) & 
MS]+d1[(dptr-886) & MS]-d1[(dptr-887) & MS]-d1[(dptr-888) & MS]-d1[(dptr-889) & 
MS]+d1[(dptr-890) & MS]-d1[(dptr-891) & MS]+d1[(dptr-892) & MS]+d1[(dptr-893) & 
MS]+d1[(dptr-894) & MS]+d1[(dptr-895) & MS]-d1[(dptr-896) & MS]-d1[(dptr-897) & 
MS]+d1[(dptr-898) & MS]-d1[(dptr-899) & MS]+d1[(dptr-900) & MS]-d1[(dptr-901) & 
MS]+d1[(dptr-902) & MS]-d1[(dptr-903) & MS]-d1[(dptr-904) & MS]+d1[(dptr-905) & 
MS]+d1[(dptr-906) & MS]-d1[(dptr-907) & MS]-d1[(dptr-908) & MS]-d1[(dptr-909) & 
MS]-d1[(dptr-910) & MS]+d1[(dptr-911) & MS]+d1[(dptr-912) & MS]-d1[(dptr-913) & 
MS]+d1[(dptr-914) & MS]-d1[(dptr-915) & MS]-d1[(dptr-916) & MS]+d1[(dptr-917) & 
MS]-d1[(dptr-918) & MS]-d1[(dptr-919) & MS]+d1[(dptr-920) & MS]+d1[(dptr-921) & 
MS]-d1[(dptr-922) & MS]+d1[(dptr-923) & MS]+d1[(dptr-924) & MS]+d1[(dptr-925) & 
MS]+d1[(dptr-926) & MS]+d1[(dptr-927) & MS]-d1[(dptr-928) & MS]+d1[(dptr-929) & 
MS]+d1[(dptr-930) & MS]+d1[(dptr-931) & MS]-d1[(dptr-932) & MS]+d1[(dptr-933) & 
MS]+d1[(dptr-934) & MS]-d1[(dptr-935) & MS]+d1[(dptr-936) & MS]+d1[(dptr-937) & 
MS]-d1[(dptr-938) & MS]-d1[(dptr-939) & MS]+d1[(dptr-940) & MS]+d1[(dptr-941) & 
MS]+d1[(dptr-942) & MS]+d1[(dptr-943) & MS]+d1[(dptr-944) & MS]+d1[(dptr-945) & 
MS]-d1[(dptr-946) & MS]+d1[(dptr-947) & MS]-d1[(dptr-948) & MS]-d1[(dptr-949) & 
MS]+d1[(dptr-950) & MS]+d1[(dptr-951) & MS]+d1[(dptr-952) & MS]-d1[(dptr-953) & 
MS]+d1[(dptr-954) & MS]-d1[(dptr-955) & MS]+d1[(dptr-956) & MS]+d1[(dptr-957) & 
MS]-d1[(dptr-958) & MS]-d1[(dptr-959) & MS]-d1[(dptr-960) & MS]+d1[(dptr-961) & 
MS]-d1[(dptr-962) & MS]-d1[(dptr-963) & MS]+d1[(dptr-964) & MS]+d1[(dptr-965) & 
MS]-d1[(dptr-966) & MS]-d1[(dptr-967) & MS]-d1[(dptr-968) & MS]+d1[(dptr-969) & 
MS]+d1[(dptr-970) & MS]+d1[(dptr-971) & MS]-d1[(dptr-972) & MS]+d1[(dptr-973) & 
MS]-d1[(dptr-974) & MS]-d1[(dptr-975) & MS]-d1[(dptr-976) & MS]-d1[(dptr-977) & 
MS]-d1[(dptr-978) & MS]-d1[(dptr-979) & MS]+d1[(dptr-980) & MS]-d1[(dptr-981) & 
MS]+d1[(dptr-982) & MS]-d1[(dptr-983) & MS]+d1[(dptr-984) & MS]+d1[(dptr-985) & 
MS]+d1[(dptr-986) & MS]-d1[(dptr-987) & MS]+d1[(dptr-988) & MS]-d1[(dptr-989) & 
MS]-d1[(dptr-990) & MS]-d1[(dptr-991) & MS]+d1[(dptr-992) & MS]-d1[(dptr-993) & 
MS]-d1[(dptr-994) & MS]+d1[(dptr-995) & MS]-d1[(dptr-996) & MS]+d1[(dptr-997) & 
MS]-d1[(dptr-998) & MS]-d1[(dptr-999) & MS]+d1[(dptr-1000) & MS]+d1[(dptr-1001) & 
MS]+d1[(dptr-1002) & MS]+d1[(dptr-1003) & MS]-d1[(dptr-1004) & MS]-d1[(dptr-1005) & 
MS]+d1[(dptr-1006) & MS]+d1[(dptr-1007) & MS]-d1[(dptr-1008) & MS]-d1[(dptr-1009) & 
MS]+d1[(dptr-1010) & MS]-d1[(dptr-1011) & MS]-d1[(dptr-1012) & MS]+d1[(dptr-1013) & 
MS]-d1[(dptr-1014) & MS]+d1[(dptr-1015) & MS]-d1[(dptr-1016) & MS]+d1[(dptr-1017) & 
MS]+d1[(dptr-1018) & MS]+d1[(dptr-1019) & MS]+d1[(dptr-1020) & MS]+d1[(dptr-1021) & 
MS]-d1[(dptr-1022) & MS]-d1[(dptr-1023) & MS];
out2 =
-d2[(dptr-0) & MS]+d2[(dptr-1) & MS]+d2[(dptr-2) & MS]+d2[(dptr-3) & MS]+d2[(dptr-4) & 
MS]+d2[(dptr-5) & MS]-d2[(dptr-6) & MS]+d2[(dptr-7) & MS]-d2[(dptr-8) & MS]+d2[(dptr-9) & 
MS]-d2[(dptr-10) & MS]-d2[(dptr-11) & MS]+d2[(dptr-12) & MS]-d2[(dptr-13) & 
MS]-d2[(dptr-14) & MS]+d2[(dptr-15) & MS]+d2[(dptr-16) & MS]-d2[(dptr-17) & 
MS]-d2[(dptr-18) & MS]+d2[(dptr-19) & MS]+d2[(dptr-20) & MS]+d2[(dptr-21) & 
MS]+d2[(dptr-22) & MS]-d2[(dptr-23) & MS]-d2[(dptr-24) & MS]+d2[(dptr-25) & 
MS]-d2[(dptr-26) & MS]+d2[(dptr-27) & MS]-d2[(dptr-28) & MS]-d2[(dptr-29) & 
MS]+d2[(dptr-30) & MS]-d2[(dptr-31) & MS]-d2[(dptr-32) & MS]-d2[(dptr-33) & 
MS]+d2[(dptr-34) & MS]-d2[(dptr-35) & MS]+d2[(dptr-36) & MS]+d2[(dptr-37) & 
MS]+d2[(dptr-38) & MS]-d2[(dptr-39) & MS]+d2[(dptr-40) & MS]-d2[(dptr-41) & 
MS]+d2[(dptr-42) & MS]-d2[(dptr-43) & MS]-d2[(dptr-44) & MS]-d2[(dptr-45) & 
MS]-d2[(dptr-46) & MS]-d2[(dptr-47) & MS]-d2[(dptr-48) & MS]+d2[(dptr-49) & 
MS]-d2[(dptr-50) & MS]+d2[(dptr-51) & MS]+d2[(dptr-52) & MS]+d2[(dptr-53) & 
MS]-d2[(dptr-54) & MS]-d2[(dptr-55) & MS]-d2[(dptr-56) & MS]+d2[(dptr-57) & 
MS]+d2[(dptr-58) & MS]-d2[(dptr-59) & MS]-d2[(dptr-60) & MS]+d2[(dptr-61) & 
MS]-d2[(dptr-62) & MS]-d2[(dptr-63) & MS]-d2[(dptr-64) & MS]+d2[(dptr-65) & 
MS]+d2[(dptr-66) & MS]-d2[(dptr-67) & MS]+d2[(dptr-68) & MS]-d2[(dptr-69) & 
MS]+d2[(dptr-70) & MS]+d2[(dptr-71) & MS]+d2[(dptr-72) & MS]-d2[(dptr-73) & 
MS]-d2[(dptr-74) & MS]+d2[(dptr-75) & MS]-d2[(dptr-76) & MS]+d2[(dptr-77) & 
MS]+d2[(dptr-78) & MS]+d2[(dptr-79) & MS]+d2[(dptr-80) & MS]+d2[(dptr-81) & 
MS]+d2[(dptr-82) & MS]-d2[(dptr-83) & MS]-d2[(dptr-84) & MS]+d2[(dptr-85) & 
MS]+d2[(dptr-86) & MS]-d2[(dptr-87) & MS]+d2[(dptr-88) & MS]+d2[(dptr-89) & 
MS]-d2[(dptr-90) & MS]+d2[(dptr-91) & MS]+d2[(dptr-92) & MS]+d2[(dptr-93) & 
MS]-d2[(dptr-94) & MS]+d2[(dptr-95) & MS]+d2[(dptr-96) & MS]+d2[(dptr-97) & 
MS]+d2[(dptr-98) & MS]+d2[(dptr-99) & MS]-d2[(dptr-100) & MS]+d2[(dptr-101) & 
MS]+d2[(dptr-102) & MS]-d2[(dptr-103) & MS]-d2[(dptr-104) & MS]+d2[(dptr-105) & 
MS]-d2[(dptr-106) & MS]-d2[(dptr-107) & MS]+d2[(dptr-108) & MS]-d2[(dptr-109) & 
MS]+d2[(dptr-110) & MS]+d2[(dptr-111) & MS]-d2[(dptr-112) & MS]-d2[(dptr-113) & 
MS]-d2[(dptr-114) & MS]-d2[(dptr-115) & MS]+d2[(dptr-116) & MS]+d2[(dptr-117) & 
MS]-d2[(dptr-118) & MS]-d2[(dptr-119) & MS]+d2[(dptr-120) & MS]-d2[(dptr-121) & 
MS]+d2[(dptr-122) & MS]-d2[(dptr-123) & MS]+d2[(dptr-124) & MS]-d2[(dptr-125) & 
MS]-d2[(dptr-126) & MS]+d2[(dptr-127) & MS]+d2[(dptr-128) & MS]+d2[(dptr-129) & 
MS]+d2[(dptr-130) & MS]-d2[(dptr-131) & MS]+d2[(dptr-132) & MS]-d2[(dptr-133) & 
MS]-d2[(dptr-134) & MS]-d2[(dptr-135) & MS]+d2[(dptr-136) & MS]-d2[(dptr-137) & 
MS]-d2[(dptr-138) & MS]+d2[(dptr-139) & MS]-d2[(dptr-140) & MS]+d2[(dptr-141) & 
MS]+d2[(dptr-142) & MS]+d2[(dptr-143) & MS]-d2[(dptr-144) & MS]-d2[(dptr-145) & 
MS]+d2[(dptr-146) & MS]+d2[(dptr-147) & MS]+d2[(dptr-148) & MS]+d2[(dptr-149) & 
MS]-d2[(dptr-150) & MS]+d2[(dptr-151) & MS]+d2[(dptr-152) & MS]-d2[(dptr-153) & 
MS]-d2[(dptr-154) & MS]-d2[(dptr-155) & MS]-d2[(dptr-156) & MS]-d2[(dptr-157) & 
MS]-d2[(dptr-158) & MS]-d2[(dptr-159) & MS]+d2[(dptr-160) & MS]-d2[(dptr-161) & 
MS]-d2[(dptr-162) & MS]-d2[(dptr-163) & MS]+d2[(dptr-164) & MS]+d2[(dptr-165) & 
MS]+d2[(dptr-166) & MS]-d2[(dptr-167) & MS]-d2[(dptr-168) & MS]-d2[(dptr-169) & 
MS]-d2[(dptr-170) & MS]+d2[(dptr-171) & MS]+d2[(dptr-172) & MS]+d2[(dptr-173) & 
MS]+d2[(dptr-174) & MS]+d2[(dptr-175) & MS]+d2[(dptr-176) & MS]-d2[(dptr-177) & 
MS]-d2[(dptr-178) & MS]-d2[(dptr-179) & MS]+d2[(dptr-180) & MS]-d2[(dptr-181) & 
MS]-d2[(dptr-182) & MS]+d2[(dptr-183) & MS]-d2[(dptr-184) & MS]-d2[(dptr-185) & 
MS]+d2[(dptr-186) & MS]+d2[(dptr-187) & MS]+d2[(dptr-188) & MS]-d2[(dptr-189) & 
MS]+d2[(dptr-190) & MS]-d2[(dptr-191) & MS]+d2[(dptr-192) & MS]+d2[(dptr-193) & 
MS]+d2[(dptr-194) & MS]-d2[(dptr-195) & MS]+d2[(dptr-196) & MS]+d2[(dptr-197) & 
MS]-d2[(dptr-198) & MS]-d2[(dptr-199) & MS]+d2[(dptr-200) & MS]+d2[(dptr-201) & 
MS]-d2[(dptr-202) & MS]+d2[(dptr-203) & MS]+d2[(dptr-204) & MS]+d2[(dptr-205) & 
MS]+d2[(dptr-206) & MS]+d2[(dptr-207) & MS]-d2[(dptr-208) & MS]-d2[(dptr-209) & 
MS]+d2[(dptr-210) & MS]-d2[(dptr-211) & MS]+d2[(dptr-212) & MS]+d2[(dptr-213) & 
MS]-d2[(dptr-214) & MS]+d2[(dptr-215) & MS]+d2[(dptr-216) & MS]-d2[(dptr-217) & 
MS]-d2[(dptr-218) & MS]-d2[(dptr-219) & MS]-d2[(dptr-220) & MS]+d2[(dptr-221) & 
MS]-d2[(dptr-222) & MS]-d2[(dptr-223) & MS]-d2[(dptr-224) & MS]-d2[(dptr-225) & 
MS]-d2[(dptr-226) & MS]+d2[(dptr-227) & MS]+d2[(dptr-228) & MS]+d2[(dptr-229) & 
MS]-d2[(dptr-230) & MS]+d2[(dptr-231) & MS]-d2[(dptr-232) & MS]+d2[(dptr-233) & 
MS]-d2[(dptr-234) & MS]+d2[(dptr-235) & MS]-d2[(dptr-236) & MS]-d2[(dptr-237) & 
MS]+d2[(dptr-238) & MS]-d2[(dptr-239) & MS]+d2[(dptr-240) & MS]+d2[(dptr-241) & 
MS]+d2[(dptr-242) & MS]+d2[(dptr-243) & MS]-d2[(dptr-244) & MS]+d2[(dptr-245) & 
MS]-d2[(dptr-246) & MS]+d2[(dptr-247) & MS]+d2[(dptr-248) & MS]+d2[(dptr-249) & 
MS]+d2[(dptr-250) & MS]+d2[(dptr-251) & MS]+d2[(dptr-252) & MS]+d2[(dptr-253) & 
MS]-d2[(dptr-254) & MS]+d2[(dptr-255) & MS]-d2[(dptr-256) & MS]-d2[(dptr-257) & 
MS]+d2[(dptr-258) & MS]-d2[(dptr-259) & MS]-d2[(dptr-260) & MS]+d2[(dptr-261) & 
MS]-d2[(dptr-262) & MS]-d2[(dptr-263) & MS]-d2[(dptr-264) & MS]-d2[(dptr-265) & 
MS]+d2[(dptr-266) & MS]+d2[(dptr-267) & MS]-d2[(dptr-268) & MS]-d2[(dptr-269) & 
MS]-d2[(dptr-270) & MS]-d2[(dptr-271) & MS]+d2[(dptr-272) & MS]+d2[(dptr-273) & 
MS]+d2[(dptr-274) & MS]-d2[(dptr-275) & MS]+d2[(dptr-276) & MS]+d2[(dptr-277) & 
MS]+d2[(dptr-278) & MS]-d2[(dptr-279) & MS]-d2[(dptr-280) & MS]-d2[(dptr-281) & 
MS]-d2[(dptr-282) & MS]-d2[(dptr-283) & MS]-d2[(dptr-284) & MS]+d2[(dptr-285) & 
MS]-d2[(dptr-286) & MS]-d2[(dptr-287) & MS]+d2[(dptr-288) & MS]+d2[(dptr-289) & 
MS]+d2[(dptr-290) & MS]-d2[(dptr-291) & MS]-d2[(dptr-292) & MS]-d2[(dptr-293) & 
MS]+d2[(dptr-294) & MS]-d2[(dptr-295) & MS]+d2[(dptr-296) & MS]-d2[(dptr-297) & 
MS]-d2[(dptr-298) & MS]+d2[(dptr-299) & MS]-d2[(dptr-300) & MS]+d2[(dptr-301) & 
MS]-d2[(dptr-302) & MS]+d2[(dptr-303) & MS]+d2[(dptr-304) & MS]+d2[(dptr-305) & 
MS]+d2[(dptr-306) & MS]-d2[(dptr-307) & MS]-d2[(dptr-308) & MS]+d2[(dptr-309) & 
MS]+d2[(dptr-310) & MS]-d2[(dptr-311) & MS]-d2[(dptr-312) & MS]+d2[(dptr-313) & 
MS]-d2[(dptr-314) & MS]-d2[(dptr-315) & MS]+d2[(dptr-316) & MS]+d2[(dptr-317) & 
MS]+d2[(dptr-318) & MS]+d2[(dptr-319) & MS]+d2[(dptr-320) & MS]-d2[(dptr-321) & 
MS]-d2[(dptr-322) & MS]+d2[(dptr-323) & MS]+d2[(dptr-324) & MS]+d2[(dptr-325) & 
MS]+d2[(dptr-326) & MS]+d2[(dptr-327) & MS]+d2[(dptr-328) & MS]+d2[(dptr-329) & 
MS]+d2[(dptr-330) & MS]-d2[(dptr-331) & MS]-d2[(dptr-332) & MS]+d2[(dptr-333) & 
MS]-d2[(dptr-334) & MS]-d2[(dptr-335) & MS]+d2[(dptr-336) & MS]-d2[(dptr-337) & 
MS]-d2[(dptr-338) & MS]+d2[(dptr-339) & MS]-d2[(dptr-340) & MS]+d2[(dptr-341) & 
MS]-d2[(dptr-342) & MS]-d2[(dptr-343) & MS]-d2[(dptr-344) & MS]+d2[(dptr-345) & 
MS]-d2[(dptr-346) & MS]+d2[(dptr-347) & MS]-d2[(dptr-348) & MS]-d2[(dptr-349) & 
MS]-d2[(dptr-350) & MS]-d2[(dptr-351) & MS]+d2[(dptr-352) & MS]+d2[(dptr-353) & 
MS]+d2[(dptr-354) & MS]+d2[(dptr-355) & MS]-d2[(dptr-356) & MS]+d2[(dptr-357) & 
MS]-d2[(dptr-358) & MS]+d2[(dptr-359) & MS]-d2[(dptr-360) & MS]+d2[(dptr-361) & 
MS]+d2[(dptr-362) & MS]-d2[(dptr-363) & MS]+d2[(dptr-364) & MS]+d2[(dptr-365) & 
MS]+d2[(dptr-366) & MS]+d2[(dptr-367) & MS]-d2[(dptr-368) & MS]+d2[(dptr-369) & 
MS]-d2[(dptr-370) & MS]-d2[(dptr-371) & MS]+d2[(dptr-372) & MS]+d2[(dptr-373) & 
MS]-d2[(dptr-374) & MS]+d2[(dptr-375) & MS]+d2[(dptr-376) & MS]-d2[(dptr-377) & 
MS]-d2[(dptr-378) & MS]+d2[(dptr-379) & MS]+d2[(dptr-380) & MS]+d2[(dptr-381) & 
MS]+d2[(dptr-382) & MS]+d2[(dptr-383) & MS]-d2[(dptr-384) & MS]+d2[(dptr-385) & 
MS]+d2[(dptr-386) & MS]+d2[(dptr-387) & MS]-d2[(dptr-388) & MS]+d2[(dptr-389) & 
MS]+d2[(dptr-390) & MS]-d2[(dptr-391) & MS]+d2[(dptr-392) & MS]+d2[(dptr-393) & 
MS]+d2[(dptr-394) & MS]+d2[(dptr-395) & MS]+d2[(dptr-396) & MS]+d2[(dptr-397) & 
MS]-d2[(dptr-398) & MS]+d2[(dptr-399) & MS]+d2[(dptr-400) & MS]+d2[(dptr-401) & 
MS]+d2[(dptr-402) & MS]+d2[(dptr-403) & MS]+d2[(dptr-404) & MS]+d2[(dptr-405) & 
MS]+d2[(dptr-406) & MS]+d2[(dptr-407) & MS]-d2[(dptr-408) & MS]+d2[(dptr-409) & 
MS]+d2[(dptr-410) & MS]-d2[(dptr-411) & MS]+d2[(dptr-412) & MS]+d2[(dptr-413) & 
MS]-d2[(dptr-414) & MS]+d2[(dptr-415) & MS]+d2[(dptr-416) & MS]-d2[(dptr-417) & 
MS]-d2[(dptr-418) & MS]+d2[(dptr-419) & MS]-d2[(dptr-420) & MS]+d2[(dptr-421) & 
MS]+d2[(dptr-422) & MS]-d2[(dptr-423) & MS]-d2[(dptr-424) & MS]+d2[(dptr-425) & 
MS]-d2[(dptr-426) & MS]+d2[(dptr-427) & MS]-d2[(dptr-428) & MS]-d2[(dptr-429) & 
MS]-d2[(dptr-430) & MS]-d2[(dptr-431) & MS]-d2[(dptr-432) & MS]+d2[(dptr-433) & 
MS]+d2[(dptr-434) & MS]-d2[(dptr-435) & MS]-d2[(dptr-436) & MS]+d2[(dptr-437) & 
MS]+d2[(dptr-438) & MS]+d2[(dptr-439) & MS]-d2[(dptr-440) & MS]-d2[(dptr-441) & 
MS]-d2[(dptr-442) & MS]-d2[(dptr-443) & MS]-d2[(dptr-444) & MS]+d2[(dptr-445) & 
MS]+d2[(dptr-446) & MS]-d2[(dptr-447) & MS]+d2[(dptr-448) & MS]+d2[(dptr-449) & 
MS]+d2[(dptr-450) & MS]-d2[(dptr-451) & MS]-d2[(dptr-452) & MS]-d2[(dptr-453) & 
MS]+d2[(dptr-454) & MS]-d2[(dptr-455) & MS]-d2[(dptr-456) & MS]-d2[(dptr-457) & 
MS]-d2[(dptr-458) & MS]-d2[(dptr-459) & MS]-d2[(dptr-460) & MS]+d2[(dptr-461) & 
MS]+d2[(dptr-462) & MS]+d2[(dptr-463) & MS]+d2[(dptr-464) & MS]-d2[(dptr-465) & 
MS]-d2[(dptr-466) & MS]-d2[(dptr-467) & MS]+d2[(dptr-468) & MS]+d2[(dptr-469) & 
MS]+d2[(dptr-470) & MS]+d2[(dptr-471) & MS]+d2[(dptr-472) & MS]+d2[(dptr-473) & 
MS]+d2[(dptr-474) & MS]-d2[(dptr-475) & MS]-d2[(dptr-476) & MS]-d2[(dptr-477) & 
MS]-d2[(dptr-478) & MS]-d2[(dptr-479) & MS]-d2[(dptr-480) & MS]-d2[(dptr-481) & 
MS]-d2[(dptr-482) & MS]-d2[(dptr-483) & MS]-d2[(dptr-484) & MS]+d2[(dptr-485) & 
MS]+d2[(dptr-486) & MS]+d2[(dptr-487) & MS]-d2[(dptr-488) & MS]-d2[(dptr-489) & 
MS]-d2[(dptr-490) & MS]+d2[(dptr-491) & MS]+d2[(dptr-492) & MS]+d2[(dptr-493) & 
MS]-d2[(dptr-494) & MS]+d2[(dptr-495) & MS]+d2[(dptr-496) & MS]-d2[(dptr-497) & 
MS]-d2[(dptr-498) & MS]-d2[(dptr-499) & MS]+d2[(dptr-500) & MS]-d2[(dptr-501) & 
MS]-d2[(dptr-502) & MS]+d2[(dptr-503) & MS]+d2[(dptr-504) & MS]-d2[(dptr-505) & 
MS]+d2[(dptr-506) & MS]-d2[(dptr-507) & MS]+d2[(dptr-508) & MS]-d2[(dptr-509) & 
MS]-d2[(dptr-510) & MS]-d2[(dptr-511) & MS]+d2[(dptr-512) & MS]-d2[(dptr-513) & 
MS]-d2[(dptr-514) & MS]-d2[(dptr-515) & MS]-d2[(dptr-516) & MS]+d2[(dptr-517) & 
MS]-d2[(dptr-518) & MS]+d2[(dptr-519) & MS]-d2[(dptr-520) & MS]+d2[(dptr-521) & 
MS]+d2[(dptr-522) & MS]+d2[(dptr-523) & MS]-d2[(dptr-524) & MS]-d2[(dptr-525) & 
MS]-d2[(dptr-526) & MS]-d2[(dptr-527) & MS]+d2[(dptr-528) & MS]-d2[(dptr-529) & 
MS]+d2[(dptr-530) & MS]+d2[(dptr-531) & MS]-d2[(dptr-532) & MS]+d2[(dptr-533) & 
MS]-d2[(dptr-534) & MS]+d2[(dptr-535) & MS]-d2[(dptr-536) & MS]+d2[(dptr-537) & 
MS]+d2[(dptr-538) & MS]+d2[(dptr-539) & MS]+d2[(dptr-540) & MS]+d2[(dptr-541) & 
MS]-d2[(dptr-542) & MS]+d2[(dptr-543) & MS]-d2[(dptr-544) & MS]-d2[(dptr-545) & 
MS]-d2[(dptr-546) & MS]-d2[(dptr-547) & MS]-d2[(dptr-548) & MS]-d2[(dptr-549) & 
MS]-d2[(dptr-550) & MS]-d2[(dptr-551) & MS]+d2[(dptr-552) & MS]-d2[(dptr-553) & 
MS]+d2[(dptr-554) & MS]-d2[(dptr-555) & MS]+d2[(dptr-556) & MS]-d2[(dptr-557) & 
MS]+d2[(dptr-558) & MS]-d2[(dptr-559) & MS]+d2[(dptr-560) & MS]-d2[(dptr-561) & 
MS]-d2[(dptr-562) & MS]-d2[(dptr-563) & MS]-d2[(dptr-564) & MS]+d2[(dptr-565) & 
MS]-d2[(dptr-566) & MS]+d2[(dptr-567) & MS]+d2[(dptr-568) & MS]+d2[(dptr-569) & 
MS]+d2[(dptr-570) & MS]-d2[(dptr-571) & MS]-d2[(dptr-572) & MS]-d2[(dptr-573) & 
MS]+d2[(dptr-574) & MS]-d2[(dptr-575) & MS]+d2[(dptr-576) & MS]+d2[(dptr-577) & 
MS]-d2[(dptr-578) & MS]+d2[(dptr-579) & MS]+d2[(dptr-580) & MS]+d2[(dptr-581) & 
MS]-d2[(dptr-582) & MS]-d2[(dptr-583) & MS]+d2[(dptr-584) & MS]+d2[(dptr-585) & 
MS]-d2[(dptr-586) & MS]+d2[(dptr-587) & MS]-d2[(dptr-588) & MS]-d2[(dptr-589) & 
MS]+d2[(dptr-590) & MS]-d2[(dptr-591) & MS]+d2[(dptr-592) & MS]-d2[(dptr-593) & 
MS]-d2[(dptr-594) & MS]+d2[(dptr-595) & MS]+d2[(dptr-596) & MS]-d2[(dptr-597) & 
MS]-d2[(dptr-598) & MS]-d2[(dptr-599) & MS]-d2[(dptr-600) & MS]+d2[(dptr-601) & 
MS]-d2[(dptr-602) & MS]+d2[(dptr-603) & MS]-d2[(dptr-604) & MS]-d2[(dptr-605) & 
MS]+d2[(dptr-606) & MS]+d2[(dptr-607) & MS]+d2[(dptr-608) & MS]-d2[(dptr-609) & 
MS]-d2[(dptr-610) & MS]+d2[(dptr-611) & MS]+d2[(dptr-612) & MS]-d2[(dptr-613) & 
MS]-d2[(dptr-614) & MS]-d2[(dptr-615) & MS]-d2[(dptr-616) & MS]-d2[(dptr-617) & 
MS]-d2[(dptr-618) & MS]+d2[(dptr-619) & MS]+d2[(dptr-620) & MS]-d2[(dptr-621) & 
MS]+d2[(dptr-622) & MS]-d2[(dptr-623) & MS]+d2[(dptr-624) & MS]-d2[(dptr-625) & 
MS]+d2[(dptr-626) & MS]-d2[(dptr-627) & MS]+d2[(dptr-628) & MS]+d2[(dptr-629) & 
MS]-d2[(dptr-630) & MS]-d2[(dptr-631) & MS]+d2[(dptr-632) & MS]+d2[(dptr-633) & 
MS]-d2[(dptr-634) & MS]-d2[(dptr-635) & MS]+d2[(dptr-636) & MS]+d2[(dptr-637) & 
MS]-d2[(dptr-638) & MS]+d2[(dptr-639) & MS]-d2[(dptr-640) & MS]+d2[(dptr-641) & 
MS]+d2[(dptr-642) & MS]-d2[(dptr-643) & MS]-d2[(dptr-644) & MS]-d2[(dptr-645) & 
MS]-d2[(dptr-646) & MS]-d2[(dptr-647) & MS]+d2[(dptr-648) & MS]-d2[(dptr-649) & 
MS]+d2[(dptr-650) & MS]+d2[(dptr-651) & MS]-d2[(dptr-652) & MS]-d2[(dptr-653) & 
MS]-d2[(dptr-654) & MS]+d2[(dptr-655) & MS]+d2[(dptr-656) & MS]+d2[(dptr-657) & 
MS]+d2[(dptr-658) & MS]-d2[(dptr-659) & MS]+d2[(dptr-660) & MS]+d2[(dptr-661) & 
MS]+d2[(dptr-662) & MS]-d2[(dptr-663) & MS]-d2[(dptr-664) & MS]+d2[(dptr-665) & 
MS]-d2[(dptr-666) & MS]-d2[(dptr-667) & MS]+d2[(dptr-668) & MS]+d2[(dptr-669) & 
MS]-d2[(dptr-670) & MS]+d2[(dptr-671) & MS]+d2[(dptr-672) & MS]+d2[(dptr-673) & 
MS]-d2[(dptr-674) & MS]+d2[(dptr-675) & MS]-d2[(dptr-676) & MS]+d2[(dptr-677) & 
MS]+d2[(dptr-678) & MS]-d2[(dptr-679) & MS]-d2[(dptr-680) & MS]+d2[(dptr-681) & 
MS]-d2[(dptr-682) & MS]-d2[(dptr-683) & MS]-d2[(dptr-684) & MS]-d2[(dptr-685) & 
MS]+d2[(dptr-686) & MS]-d2[(dptr-687) & MS]-d2[(dptr-688) & MS]-d2[(dptr-689) & 
MS]+d2[(dptr-690) & MS]-d2[(dptr-691) & MS]+d2[(dptr-692) & MS]-d2[(dptr-693) & 
MS]+d2[(dptr-694) & MS]-d2[(dptr-695) & MS]-d2[(dptr-696) & MS]-d2[(dptr-697) & 
MS]+d2[(dptr-698) & MS]+d2[(dptr-699) & MS]-d2[(dptr-700) & MS]-d2[(dptr-701) & 
MS]+d2[(dptr-702) & MS]+d2[(dptr-703) & MS]-d2[(dptr-704) & MS]-d2[(dptr-705) & 
MS]-d2[(dptr-706) & MS]+d2[(dptr-707) & MS]-d2[(dptr-708) & MS]-d2[(dptr-709) & 
MS]-d2[(dptr-710) & MS]+d2[(dptr-711) & MS]-d2[(dptr-712) & MS]-d2[(dptr-713) & 
MS]-d2[(dptr-714) & MS]+d2[(dptr-715) & MS]+d2[(dptr-716) & MS]-d2[(dptr-717) & 
MS]-d2[(dptr-718) & MS]-d2[(dptr-719) & MS]+d2[(dptr-720) & MS]-d2[(dptr-721) & 
MS]+d2[(dptr-722) & MS]-d2[(dptr-723) & MS]+d2[(dptr-724) & MS]+d2[(dptr-725) & 
MS]-d2[(dptr-726) & MS]-d2[(dptr-727) & MS]-d2[(dptr-728) & MS]+d2[(dptr-729) & 
MS]-d2[(dptr-730) & MS]+d2[(dptr-731) & MS]+d2[(dptr-732) & MS]+d2[(dptr-733) & 
MS]+d2[(dptr-734) & MS]+d2[(dptr-735) & MS]-d2[(dptr-736) & MS]-d2[(dptr-737) & 
MS]-d2[(dptr-738) & MS]-d2[(dptr-739) & MS]+d2[(dptr-740) & MS]-d2[(dptr-741) & 
MS]-d2[(dptr-742) & MS]+d2[(dptr-743) & MS]-d2[(dptr-744) & MS]-d2[(dptr-745) & 
MS]-d2[(dptr-746) & MS]+d2[(dptr-747) & MS]+d2[(dptr-748) & MS]+d2[(dptr-749) & 
MS]+d2[(dptr-750) & MS]-d2[(dptr-751) & MS]-d2[(dptr-752) & MS]+d2[(dptr-753) & 
MS]+d2[(dptr-754) & MS]+d2[(dptr-755) & MS]-d2[(dptr-756) & MS]+d2[(dptr-757) & 
MS]+d2[(dptr-758) & MS]-d2[(dptr-759) & MS]+d2[(dptr-760) & MS]-d2[(dptr-761) & 
MS]+d2[(dptr-762) & MS]+d2[(dptr-763) & MS]-d2[(dptr-764) & MS]+d2[(dptr-765) & 
MS]-d2[(dptr-766) & MS]-d2[(dptr-767) & MS]+d2[(dptr-768) & MS]+d2[(dptr-769) & 
MS]-d2[(dptr-770) & MS]-d2[(dptr-771) & MS]+d2[(dptr-772) & MS]-d2[(dptr-773) & 
MS]+d2[(dptr-774) & MS]+d2[(dptr-775) & MS]+d2[(dptr-776) & MS]-d2[(dptr-777) & 
MS]+d2[(dptr-778) & MS]+d2[(dptr-779) & MS]+d2[(dptr-780) & MS]-d2[(dptr-781) & 
MS]+d2[(dptr-782) & MS]-d2[(dptr-783) & MS]-d2[(dptr-784) & MS]+d2[(dptr-785) & 
MS]-d2[(dptr-786) & MS]+d2[(dptr-787) & MS]+d2[(dptr-788) & MS]-d2[(dptr-789) & 
MS]+d2[(dptr-790) & MS]-d2[(dptr-791) & MS]-d2[(dptr-792) & MS]-d2[(dptr-793) & 
MS]+d2[(dptr-794) & MS]-d2[(dptr-795) & MS]+d2[(dptr-796) & MS]+d2[(dptr-797) & 
MS]-d2[(dptr-798) & MS]-d2[(dptr-799) & MS]+d2[(dptr-800) & MS]+d2[(dptr-801) & 
MS]+d2[(dptr-802) & MS]-d2[(dptr-803) & MS]+d2[(dptr-804) & MS]-d2[(dptr-805) & 
MS]-d2[(dptr-806) & MS]+d2[(dptr-807) & MS]+d2[(dptr-808) & MS]+d2[(dptr-809) & 
MS]+d2[(dptr-810) & MS]+d2[(dptr-811) & MS]+d2[(dptr-812) & MS]-d2[(dptr-813) & 
MS]+d2[(dptr-814) & MS]-d2[(dptr-815) & MS]+d2[(dptr-816) & MS]+d2[(dptr-817) & 
MS]-d2[(dptr-818) & MS]+d2[(dptr-819) & MS]+d2[(dptr-820) & MS]-d2[(dptr-821) & 
MS]+d2[(dptr-822) & MS]-d2[(dptr-823) & MS]-d2[(dptr-824) & MS]-d2[(dptr-825) & 
MS]-d2[(dptr-826) & MS]-d2[(dptr-827) & MS]+d2[(dptr-828) & MS]-d2[(dptr-829) & 
MS]-d2[(dptr-830) & MS]-d2[(dptr-831) & MS]-d2[(dptr-832) & MS]+d2[(dptr-833) & 
MS]+d2[(dptr-834) & MS]+d2[(dptr-835) & MS]-d2[(dptr-836) & MS]-d2[(dptr-837) & 
MS]+d2[(dptr-838) & MS]+d2[(dptr-839) & MS]+d2[(dptr-840) & MS]-d2[(dptr-841) & 
MS]-d2[(dptr-842) & MS]+d2[(dptr-843) & MS]-d2[(dptr-844) & MS]-d2[(dptr-845) & 
MS]-d2[(dptr-846) & MS]+d2[(dptr-847) & MS]-d2[(dptr-848) & MS]-d2[(dptr-849) & 
MS]+d2[(dptr-850) & MS]+d2[(dptr-851) & MS]+d2[(dptr-852) & MS]+d2[(dptr-853) & 
MS]-d2[(dptr-854) & MS]-d2[(dptr-855) & MS]-d2[(dptr-856) & MS]-d2[(dptr-857) & 
MS]+d2[(dptr-858) & MS]+d2[(dptr-859) & MS]-d2[(dptr-860) & MS]+d2[(dptr-861) & 
MS]+d2[(dptr-862) & MS]-d2[(dptr-863) & MS]-d2[(dptr-864) & MS]-d2[(dptr-865) & 
MS]+d2[(dptr-866) & MS]+d2[(dptr-867) & MS]-d2[(dptr-868) & MS]+d2[(dptr-869) & 
MS]-d2[(dptr-870) & MS]-d2[(dptr-871) & MS]+d2[(dptr-872) & MS]+d2[(dptr-873) & 
MS]+d2[(dptr-874) & MS]-d2[(dptr-875) & MS]+d2[(dptr-876) & MS]+d2[(dptr-877) & 
MS]+d2[(dptr-878) & MS]+d2[(dptr-879) & MS]-d2[(dptr-880) & MS]-d2[(dptr-881) & 
MS]+d2[(dptr-882) & MS]-d2[(dptr-883) & MS]-d2[(dptr-884) & MS]-d2[(dptr-885) & 
MS]-d2[(dptr-886) & MS]-d2[(dptr-887) & MS]-d2[(dptr-888) & MS]-d2[(dptr-889) & 
MS]+d2[(dptr-890) & MS]+d2[(dptr-891) & MS]-d2[(dptr-892) & MS]-d2[(dptr-893) & 
MS]-d2[(dptr-894) & MS]+d2[(dptr-895) & MS]+d2[(dptr-896) & MS]+d2[(dptr-897) & 
MS]-d2[(dptr-898) & MS]-d2[(dptr-899) & MS]+d2[(dptr-900) & MS]-d2[(dptr-901) & 
MS]+d2[(dptr-902) & MS]-d2[(dptr-903) & MS]+d2[(dptr-904) & MS]+d2[(dptr-905) & 
MS]-d2[(dptr-906) & MS]+d2[(dptr-907) & MS]-d2[(dptr-908) & MS]+d2[(dptr-909) & 
MS]+d2[(dptr-910) & MS]+d2[(dptr-911) & MS]+d2[(dptr-912) & MS]-d2[(dptr-913) & 
MS]+d2[(dptr-914) & MS]+d2[(dptr-915) & MS]+d2[(dptr-916) & MS]+d2[(dptr-917) & 
MS]-d2[(dptr-918) & MS]+d2[(dptr-919) & MS]+d2[(dptr-920) & MS]-d2[(dptr-921) & 
MS]+d2[(dptr-922) & MS]-d2[(dptr-923) & MS]-d2[(dptr-924) & MS]+d2[(dptr-925) & 
MS]-d2[(dptr-926) & MS]-d2[(dptr-927) & MS]-d2[(dptr-928) & MS]-d2[(dptr-929) & 
MS]-d2[(dptr-930) & MS]+d2[(dptr-931) & MS]-d2[(dptr-932) & MS]+d2[(dptr-933) & 
MS]-d2[(dptr-934) & MS]-d2[(dptr-935) & MS]-d2[(dptr-936) & MS]+d2[(dptr-937) & 
MS]+d2[(dptr-938) & MS]+d2[(dptr-939) & MS]-d2[(dptr-940) & MS]+d2[(dptr-941) & 
MS]-d2[(dptr-942) & MS]-d2[(dptr-943) & MS]-d2[(dptr-944) & MS]+d2[(dptr-945) & 
MS]+d2[(dptr-946) & MS]-d2[(dptr-947) & MS]+d2[(dptr-948) & MS]+d2[(dptr-949) & 
MS]+d2[(dptr-950) & MS]+d2[(dptr-951) & MS]-d2[(dptr-952) & MS]-d2[(dptr-953) & 
MS]-d2[(dptr-954) & MS]-d2[(dptr-955) & MS]-d2[(dptr-956) & MS]+d2[(dptr-957) & 
MS]-d2[(dptr-958) & MS]-d2[(dptr-959) & MS]+d2[(dptr-960) & MS]-d2[(dptr-961) & 
MS]+d2[(dptr-962) & MS]-d2[(dptr-963) & MS]+d2[(dptr-964) & MS]-d2[(dptr-965) & 
MS]+d2[(dptr-966) & MS]+d2[(dptr-967) & MS]+d2[(dptr-968) & MS]-d2[(dptr-969) & 
MS]+d2[(dptr-970) & MS]-d2[(dptr-971) & MS]-d2[(dptr-972) & MS]-d2[(dptr-973) & 
MS]-d2[(dptr-974) & MS]+d2[(dptr-975) & MS]-d2[(dptr-976) & MS]-d2[(dptr-977) & 
MS]+d2[(dptr-978) & MS]+d2[(dptr-979) & MS]-d2[(dptr-980) & MS]-d2[(dptr-981) & 
MS]-d2[(dptr-982) & MS]+d2[(dptr-983) & MS]+d2[(dptr-984) & MS]-d2[(dptr-985) & 
MS]-d2[(dptr-986) & MS]-d2[(dptr-987) & MS]-d2[(dptr-988) & MS]-d2[(dptr-989) & 
MS]+d2[(dptr-990) & MS]+d2[(dptr-991) & MS]+d2[(dptr-992) & MS]+d2[(dptr-993) & 
MS]+d2[(dptr-994) & MS]-d2[(dptr-995) & MS]-d2[(dptr-996) & MS]-d2[(dptr-997) & 
MS]+d2[(dptr-998) & MS]+d2[(dptr-999) & MS]-d2[(dptr-1000) & MS]+d2[(dptr-1001) & 
MS]+d2[(dptr-1002) & MS]-d2[(dptr-1003) & MS]+d2[(dptr-1004) & MS]-d2[(dptr-1005) & 
MS]+d2[(dptr-1006) & MS]-d2[(dptr-1007) & MS]-d2[(dptr-1008) & MS]+d2[(dptr-1009) & 
MS]+d2[(dptr-1010) & MS]-d2[(dptr-1011) & MS]+d2[(dptr-1012) & MS]-d2[(dptr-1013) & 
MS]-d2[(dptr-1014) & MS]-d2[(dptr-1015) & MS]-d2[(dptr-1016) & MS]+d2[(dptr-1017) & 
MS]+d2[(dptr-1018) & MS]-d2[(dptr-1019) & MS]+d2[(dptr-1020) & MS]-d2[(dptr-1021) & 
MS]-d2[(dptr-1022) & MS]-d2[(dptr-1023) & MS];
    // scale to prevent clipping;
    // factor experimentally determined to match rms level of input
    out1 *= 0.04986f;
    out2 *= 0.04986f;
#endif

    // mix to mono in ch1, if mono==1
    Lout = (1.0f - mono)*out1 + mono * 0.5f*(out1+out2);
    Rout = (1.0f - mono)*out2;

    buffer_write(output1[pos], Lout);
    buffer_write(output2[pos], Rout);

    dptr = (dptr + 1) & (D_SIZE - 1);
  }

  plugin_data->dptr = dptr;

}

void _init(void);
void _init(void) {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#define D_(s) (s)

	decorrMLSDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (decorrMLSDescriptor) {
		decorrMLSDescriptor->UniqueID = 9011;
		decorrMLSDescriptor->Label = "RTdecorrmls";
		decorrMLSDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		decorrMLSDescriptor->Name =
		 D_("RT mls-based stereo decorrelation");
		decorrMLSDescriptor->Maker =
		 "Richard Taylor <rtaylor@tru.ca>";
		decorrMLSDescriptor->Copyright =
		 "GPL";
		decorrMLSDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		decorrMLSDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		decorrMLSDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		decorrMLSDescriptor->PortNames =
		 (const char **)port_names;

    /* Parameters for mono summing mode */
    port_descriptors[DECORRMLS_MONO] =
     LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    port_names[DECORRMLS_MONO] =
     D_("Mono summing mode");
    port_range_hints[DECORRMLS_MONO].HintDescriptor =
     LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | 
LADSPA_HINT_DEFAULT_0;
    port_range_hints[DECORRMLS_MONO].LowerBound = 0;
    port_range_hints[DECORRMLS_MONO].UpperBound = 1;

		/* Parameters for Input1 */
		port_descriptors[DECORRMLS_INPUT1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DECORRMLS_INPUT1] =
		 D_("Input 1");
		port_range_hints[DECORRMLS_INPUT1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECORRMLS_INPUT1].LowerBound = -1.0;
		port_range_hints[DECORRMLS_INPUT1].UpperBound = +1.0;

		/* Parameters for Input2 */
		port_descriptors[DECORRMLS_INPUT2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DECORRMLS_INPUT2] =
		 D_("Input 2");
		port_range_hints[DECORRMLS_INPUT2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECORRMLS_INPUT2].LowerBound = -1.0;
		port_range_hints[DECORRMLS_INPUT2].UpperBound = +1.0;

		/* Parameters for Output 1 */
		port_descriptors[DECORRMLS_OUTPUT1] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DECORRMLS_OUTPUT1] =
		 D_("Output 1");
		port_range_hints[DECORRMLS_OUTPUT1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECORRMLS_OUTPUT1].LowerBound = -1.0;
		port_range_hints[DECORRMLS_OUTPUT1].UpperBound = +1.0;

		/* Parameters for Output 2 */
		port_descriptors[DECORRMLS_OUTPUT2] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DECORRMLS_OUTPUT2] =
		 D_("Output 2");
		port_range_hints[DECORRMLS_OUTPUT2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECORRMLS_OUTPUT2].LowerBound = -1.0;
		port_range_hints[DECORRMLS_OUTPUT2].UpperBound = +1.0;

		decorrMLSDescriptor->activate = NULL;
/*		decorrMLSDescriptor->activate = activateDecorrMLS;*/
		decorrMLSDescriptor->cleanup = cleanupDecorrMLS;
		decorrMLSDescriptor->connect_port = connectPortDecorrMLS;
		decorrMLSDescriptor->deactivate = NULL;
		decorrMLSDescriptor->instantiate = instantiateDecorrMLS;
		decorrMLSDescriptor->run = runDecorrMLS;
		decorrMLSDescriptor->run_adding = NULL;
		decorrMLSDescriptor->set_run_adding_gain = NULL;
	}
}

void _fini(void);
void _fini(void) {
	if (decorrMLSDescriptor) {
		free((LADSPA_PortDescriptor *)decorrMLSDescriptor->PortDescriptors);
		free((char **)decorrMLSDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)decorrMLSDescriptor->PortRangeHints);
		free(decorrMLSDescriptor);
	}

}

