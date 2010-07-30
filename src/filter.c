/*****************************************************************************
 *
 * filter.c
 *
 * PHASEX:  [P]hase [H]armonic [A]dvanced [S]ynthesis [EX]periment
 *
 * Copyright (C) 1999-2009 William Weston <weston@sysex.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "wave.h"
#include "jack.h"
#include "filter.h"


sample_t	filter_res[NUM_FILTER_TYPES][128];
sample_t	filter_table[TUNING_RESOLUTION * 648];
sample_t	filter_dist_1[16];
sample_t	filter_dist_2[16];

int		filter_limit = 1;


/*****************************************************************************
 *
 * BUILD_FILTER_TABLES()
 *
 * Builds tables with filter saturation, dampening, and resonance curves
 *
 *****************************************************************************/
void
build_filter_tables(void) {
    int		j;
    int		k;
    sample_t	step;
    sample_t	freq;
    sample_t	a;

    /* 120 steps between halfstep */
    step = exp (log (2.0) / (12.0 * F_TUNING_RESOLUTION));

    /* saturation and resonance curves */

    /* Both filter types use the same resonance curve for now */
    for (j = 0; j < 128; j++) {
	filter_res[FILTER_TYPE_DIST][j]  = 1.0 - ((sample_t)(j) / 128.0);
	filter_res[FILTER_TYPE_RETRO][j] = 1.0 - ((sample_t)(j) / 128.0);
    }

    /* build the high res table for fine filter adjustments */
    for (j = 0; j < 648; j++) {
	freq = freq_table[64][j];
	for (k = j * TUNING_RESOLUTION; k < ((j + 1) * TUNING_RESOLUTION); k++) {
	    filter_table[k] = sin (M_PI * 2.0 * freq / (f_sample_rate * F_FILTER_OVERSAMPLE));
	    if (freq < nyquist_freq) {
		filter_limit = k;
	    }
	    freq *= step;
	}
    }

    if (debug) {
	fprintf (stderr, "Filter Limit:  %d\n", filter_limit);
    }

    /* build distortion value table to be used in oversampled chamberlin */
    a = 1.0;
    for (j = 0; j < 16; j++) {
	a += 1.0;
	filter_dist_1[j] = a;
	filter_dist_2[j] = 1.0 / a;
    }
}


/*****************************************************************************
 *
 * FILTER_WAVE_TABLE()
 *
 * Apply lowpass filter to wavetable for bandlimiting oscillators.
 *
 *****************************************************************************/
void
filter_wave_table(int wave_num, int num_cycles, double octaves) {
    int		j;
    int		cycle;
    int		sample;
    int		oversample	= 60;
    sample_t	f;
    sample_t	q;
    sample_t	hp		= 0.0;
    sample_t	bp		= 0.0;
    sample_t	lp		= 0.0;
    sample_t	last_lp		= 0.0;

    /* set cutoff at N ocatves above the principal */
    f = (sample_t)sin (M_PI * 2.0 * (pow (2.0, octaves)) / (F_WAVEFORM_SIZE * (double)oversample));

    /* zero resonance for simple bandlimiting */
    q = 1.0;

    /* run seven cycles of the waveform through the filter. */
    /* dump the output of the filter into the wave table on seventh run */
    for (cycle = 1; cycle <= num_cycles; cycle++) {
	for (sample = 0; sample < F_WAVEFORM_SIZE; sample++) {

	    /* oversample the filter */
	    for (j = 0; j < oversample; j++) {

		/* highpass */
		hp = wave_table[wave_num][sample] - lp - (bp * q);

		/* bandpass */
		bp += (f * hp);

		/* lowpass */
		lp += (f * bp);
	    }

	    /* ignore filter output on the first cycles */
	    if (cycle == num_cycles) {
		/* take the lowpass tap */
		wave_table[wave_num][sample] = lp;
#ifdef EXTRA_DEBUG
		if (debug && (((lp < 0) && (last_lp >= 0)) || ((last_lp < 0) && (lp >= 0)))) {
			fprintf (stderr, "%06d  %1.23f    %06d %1.23f\n",
				 (sample - 1), last_lp, sample, lp);
		}
#endif
	    }
	    last_lp = lp;
	}
    }
}
