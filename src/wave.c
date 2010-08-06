/*****************************************************************************
 *
 * wave.c
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <samplerate.h>
#include "phasex.h"
#include "config.h"
#include "wave.h"
#include "filter.h"
#include "jack.h"
#include "patch.h"


/* base tuning frequency -- can be overridden on command line */
double		a4freq = A4FREQ;

/* this is _the_ wave table */
sample_t	wave_table[NUM_WAVEFORMS][WAVEFORM_SIZE];

/* frequency table for midi notes */
sample_t	freq_table[128][648];

/* lookup table for exponential frequency scaling */
sample_t	freq_shift_table[FREQ_SHIFT_TABLE_SIZE];

/* env table for envelope durations in samples */
int		env_table[128];

/* env curve for envelope values to actual amplitudes */
sample_t	env_curve[ENV_CURVE_SIZE+1];

/* table for mixing gain, w/ unity at cc=127 */
sample_t	mix_table[128];

/* table for stereo panner, centered at 64 */
sample_t	pan_table[128];

/* table for general purpose gain, w/ unity at cc=120 */
sample_t	gain_table[128];

/* gain table for velocity[sensitivity][velicity], w/ unity at velocity=100 */
sample_t	velocity_gain_table[128][128];

/* keyfollow table for ctlr/key combo --> amplitude coefficient */
sample_t	keyfollow_table[128][128];


/*****************************************************************************
 *
 * BUILD_FREQ_TABLE()
 *
 * Generates the (midi_note - 128) to frequency lookup table
 *
 *****************************************************************************/
void
build_freq_table(void) {
    double	basefreq;
    double	freq;
    double	halfstep;
    double	tunestep;
    int		octave;
    int		note;
    int		j;

    /* build table with 53 1/3 octaves, w/ MIDI range in center */

    /* normal halfstep is 12th of an even tempered octave */
    halfstep = exp (log (2.0) / 12.0);

    /* tuning works in 120th of a halfstep increments */
    tunestep = exp (log (2.0) / (12.0 * F_TUNING_RESOLUTION));

    /* for every master tune adjustment */
    for (j = 0; j < 128; j++) {
	basefreq = a4freq * pow (tunestep, (double)(j - 64));

	/* build the table with a freshly calculated base freq every octave */
	for (octave = 0; octave < 54; octave++) {

	    /* A4 down 27 octaves and 1 half-step */
	    /* (table starts at 256 half-steps below midi range) */
	    freq = basefreq * pow(2.0, (double)(octave - 27)) / halfstep;

	    /* calculate notes for each octave with successive multiplications */
	    for (note = octave * 12; note < (octave + 1) * 12; note++) {
		freq_table[j][note] = (sample_t)freq;
		freq                *= halfstep;
	    }
	}
    }
}


/*****************************************************************************
 *
 * BUILD_FREQ_SHIFT_TABLE()
 *
 * Generates lookup table for exponential function used for
 * fine frequency adjustments.
 *
 *****************************************************************************/
void
build_freq_shift_table(void) {
    double	halfstep;
    double	tunestep;
    int		index;
    int		octaves;
    int		halfsteps;
    int		tunesteps;

    halfstep  = exp (log (2.0) / 12.0);
    tunestep = exp (log (2.0) / (12.0 * F_TUNING_RESOLUTION));

    for (index = 0; index < FREQ_SHIFT_TABLE_SIZE / 2; index++) {
	tunesteps = (FREQ_SHIFT_TABLE_SIZE / 2) - index;
	halfsteps = tunesteps / TUNING_RESOLUTION;
	tunesteps %= TUNING_RESOLUTION;
	octaves   = halfsteps / 12;
	halfsteps %= 12;
	freq_shift_table[index] = (sample_t)(1.0 /
	    (pow (2.0,      (double)octaves) *
	     pow (halfstep, (double)halfsteps) *
	     pow (tunestep, (double)tunesteps)));
    }
    freq_shift_table[(FREQ_SHIFT_TABLE_SIZE / 2)] = 1.0;
    for (index = (FREQ_SHIFT_TABLE_SIZE / 2) + 1; index < FREQ_SHIFT_TABLE_SIZE; index++) {
	tunesteps = index - (FREQ_SHIFT_TABLE_SIZE / 2);
	halfsteps = tunesteps / TUNING_RESOLUTION;
	tunesteps %= TUNING_RESOLUTION;
	octaves   = halfsteps / 12;
	halfsteps %= 12;
	freq_shift_table[index] = (sample_t)
	    (pow (2.0,      (double)octaves) *
	     pow (halfstep, (double)halfsteps) *
	     pow (tunestep, (double)tunesteps));
    }
}


/*****************************************************************************
 *
 * BUILD_ENV_TABLES()
 *
 * Generates lookup tables for cc to envelope duration and
 * amplitude curve.
 *
 *****************************************************************************/
void
build_env_tables(void) {
    sample_t	env;
    sample_t	logval;
    sample_t	logstep;
    int		j;

    /* envelope duration table */
    for (j = 0; j < 128; j++) {
	env_table[j] = ((j + 0) * (j + 0) * (sample_rate / 6000)) + (sample_rate / 2000);
    }

    /* envelope logarithmic volume curve */
    logval  = logf (5.0 * F_ENV_CURVE_SIZE);
    logstep = 5.0 * F_ENV_CURVE_SIZE;
    env     = 1.0;
    for (j = ENV_CURVE_SIZE; j > 0; j--) {
	env_curve[j] = (sample_t)(env);
	env          /= expf (logval / logstep);
	logstep      -= 5.0;
#ifdef EXTRA_DEBUG
	if (debug && ((j % 96) == 0)) {
	    fprintf (stderr, "Envelope %05d: %012d, %.23f\n", j, (unsigned int)(1.0 / env_curve[j]), env_curve[j]);
	}
#endif
    }
#ifdef EXTRA_DEBUG
    if (debug) {
	fprintf (stderr, "Envelope %05d: %012d, %.23f\n", 1, (unsigned int)(1.0 / env_curve[1]), env_curve[1]);
    }
#endif
    env_curve[0] = 0.0;
}


/*****************************************************************************
 *
 * BUILD_KEYFOLLOW_TABLE()
 *
 * Generates lookup tables for cc to keyfollow volume adjustment
 *
 *****************************************************************************/
void
build_keyfollow_table(void) {
    int		ctlr;
    int		key;
    sample_t	slope;

    /* first the low freq weighted slope */
    for (ctlr = 0; ctlr < 64; ctlr++) {
	slope = (sample_t)(64 - ctlr) / 64.0;
	for (key = 0; key < 128; key++) {
	    keyfollow_table[ctlr][key] = 1.0 - (slope * (((sample_t)(key)) / 127.0));
	}
    }

    /* next the median */
    for (key = 0; key < 128; key++) {
	keyfollow_table[64][key] = 1.0;
    }

    /* last the high freq weighted slope */
    for (ctlr = 65; ctlr < 128; ctlr++) {
	slope = (sample_t)(ctlr - 64) / 63.0;
	for (key = 0; key < 128; key++) {
	    keyfollow_table[ctlr][key] = 1.0 - (slope * (((sample_t)(127 - key)) / 127.0));
	}
    }
}


/*****************************************************************************
 *
 * BUILD_MIX_TABLE()
 *
 * Generates lookup tables for cc to mix scalar mappings.
 *
 *****************************************************************************/
void
build_mix_table(void) {
    int		j;

    /* a simple parabolic curve works well, but still isn't perfect */
    mix_table[0]   = 0.0;
    mix_table[127] = 1.0;
    for (j = 1; j < 127; j++) {
    	mix_table[j] = ((sample_t)j) / 127.0;
    	mix_table[j] += sqrt (((sample_t)j) / 127.0);
	mix_table[j] *= 0.5;
    }
}


/*****************************************************************************
 *
 * BUILD_PAN_TABLE()
 *
 * Generates lookup tables for cc to mix scalar mappings.
 *
 *****************************************************************************/
void
build_pan_table(void) {
    int		j;
    sample_t	scale = 1.0 / (0.5 + sqrt (0.5));

    /* average between parabolic and linear curves */
    pan_table[0]   = 0.0;
    pan_table[127] = 1.0;
    for (j = 1; j < 127; j++) {
    	pan_table[j] = ((sample_t)j) / 128.0;
    	pan_table[j] += sqrt (((sample_t)j) / 128.0);
	pan_table[j] *= scale;
    }
}


/*****************************************************************************
 *
 * BUILD_GAIN_TABLE()
 *
 * Generates lookup tables for cc to gain scalar mappings.
 *
 *****************************************************************************/
void
build_gain_table(void) {
    int		j;
    sample_t	gain;
    sample_t	step;

    /* 1/2 dB steps */
    step = exp (log (10.0) / 40.0);

    gain = 1.0;
    for (j = 119; j >= 0; j--) {
	gain /= step;
	gain_table[j] = gain;
    }
    gain_table[120] = 1.0;
    gain = 1.0;
    for (j = 121; j < 128; j++) {
	gain *= step;
	gain_table[j] = gain;
    }
}


/*****************************************************************************
 *
 * BUILD_VELOCITY_GAIN_TABLE()
 *
 * Generates lookup tables for cc to gain scalar mappings.
 *
 *****************************************************************************/
void
build_velocity_gain_table(void) {
    int		j;
    int		k;
    sample_t	gain;
    sample_t	step;

    /* sensitivity 0 gets no gain adjustment */
    for (k = 0; k < 128; k++) {
	velocity_gain_table[0][k] = 1.0;
    }

    /* step through the sensitivity levels */
    for (j = 1; j < 128; j++) {

	/* tiny up to 1/2 dB steps */
	step = exp (log (10.0) / (40.0 + 254.0 - (2.0 * (sample_t)j)));

	/* calculate gain for this sensitivity, centered at 100 */
	gain = 1.0;
	for (k = 99; k >= 0; k--) {
	    gain /= step;
	    velocity_gain_table[j][k] = gain;
	}
	velocity_gain_table[j][100] = 1.0;
	gain = 1.0;
	for (k = 101; k < 128; k++) {
	    gain *= step;
	    velocity_gain_table[j][k] = gain;
	}
    }
}


/*****************************************************************************
 *
 * BUILD_WAVE_TABLES()
 *
 * Generates high res wave tables for use by all oscillators and lfos
 *
 *****************************************************************************/
void
build_waveform_tables(void) {
    int			sample_num;
    sample_t		angle;

    for (sample_num = 0; sample_num < WAVEFORM_SIZE; sample_num++) {

	angle = 2 * M_PI * (sample_t)sample_num / F_WAVEFORM_SIZE;

	wave_table[WAVE_SINE][sample_num]	= sin (angle);
	wave_table[WAVE_TRIANGLE][sample_num]	= func_triangle (sample_num);
	wave_table[WAVE_SAW][sample_num]	= func_saw (sample_num);
	wave_table[WAVE_REVSAW][sample_num]	= func_revsaw (sample_num);
	wave_table[WAVE_SQUARE][sample_num]	= func_square (sample_num);
	wave_table[WAVE_STAIR][sample_num]	= func_stair (sample_num);
	wave_table[WAVE_SAW_S][sample_num]	= func_saw (sample_num);
	wave_table[WAVE_REVSAW_S][sample_num]	= func_revsaw (sample_num);
	wave_table[WAVE_SQUARE_S][sample_num]	= func_square (sample_num);
	wave_table[WAVE_STAIR_S][sample_num]	= func_stair (sample_num);
	wave_table[WAVE_IDENTITY][sample_num]	= 1.0;
	wave_table[WAVE_NULL][sample_num]	= 0.0;
    }

    /* bandlimit the _S (or BL) waveshapes */
    filter_wave_table (WAVE_SAW_S,    7, 5.0);
    filter_wave_table (WAVE_REVSAW_S, 7, 5.0);
    filter_wave_table (WAVE_SQUARE_S, 7, 5.0);
    filter_wave_table (WAVE_STAIR_S,  7, 5.0);

    /* load and bandlimit sampled waveforms */
#ifdef IGNORE_SAMPLES
    int j;
    for (j = 12; j < NUM_WAVEFORMS; j++) {
	for (sample_num = 0; sample_num < WAVEFORM_SIZE; sample_num++) {
	    wave_table[j][sample_num] = wave_table[WAVE_SINE][sample_num];
	}
    }
#else
    load_sampled_waveform (WAVE_JUNO_OSC,    7, 3.0);
    load_sampled_waveform (WAVE_JUNO_SAW,    7, 4.0);
    load_sampled_waveform (WAVE_JUNO_SQUARE, 7, 4.0);
    load_sampled_waveform (WAVE_JUNO_POLY,   7, 4.0);
    load_sampled_waveform (WAVE_VOX_1,       7, 3.0);
    load_sampled_waveform (WAVE_VOX_2,       7, 3.0);
#endif
}


/*****************************************************************************
 *
 * LOAD_SAMPLED_WAVEFORM()
 *
 * Loads sampled oscillator waveforms into the high-res wavetable.
 *
 *****************************************************************************/
int load_sampled_waveform(int wavenum, int num_cycles, double octaves) {
    float	buffer[32400];
    char	filename[PATH_MAX];
    struct stat statbuf;
    SRC_DATA	src_data;
    FILE	*rawfile;
    int		sample;
    int		input_len;
    float	pos_max;
    float	neg_max;
    float	offset;
    float	scalar;

    /* get file size */
    snprintf (filename, PATH_MAX, "%s/%s.raw", SAMPLE_DIR, wave_names[wavenum]);
    if (stat (filename, &statbuf) != 0) {
	if (debug) {
	    fprintf (stderr, "Unable to load raw sample file '%s'\n", filename);
	}
    }
    input_len = statbuf.st_size / sizeof (float);
    if (debug) {
	fprintf (stderr, "Raw Waveform '%s': %d samples\n", filename, input_len);
    }

    /* open raw sample file */
    if ((rawfile = fopen (filename, "rb")) != 0) {

	/* read sample data */
	if (fread (buffer, sizeof (float), input_len, rawfile) == input_len) {
	    fclose (rawfile);

	    /* normalize sample to [-1,1] */
	    pos_max = neg_max = 0;
	    for (sample = 0; sample < input_len; sample++) {
		if (buffer[sample] > pos_max) {
		    pos_max = buffer[sample];
		}
		if (buffer[sample] < neg_max) {
		    neg_max = buffer[sample];
		}
	    }
	    offset = (pos_max + neg_max) / -2.0;
	    scalar = 1.0 / (float)((pos_max - neg_max) / 2.0);
	    for (sample = 0; sample < input_len; sample++) {
		buffer[sample] = (buffer[sample] + offset) * scalar;
	    }

	    /* resample to fit WAVEFORM_SIZE */
	    src_data.data_in       = buffer;
	    src_data.data_out      = wave_table[wavenum];
	    src_data.input_frames  = input_len;
	    src_data.output_frames = WAVEFORM_SIZE;
	    src_data.src_ratio     = F_WAVEFORM_SIZE / (double)input_len;

	    src_simple (&src_data, SRC_SINC_BEST_QUALITY, 1);

	    /* filter resampled data */
	    filter_wave_table (wavenum, num_cycles, octaves);

	    /* all done */
	    return 0;
	}
	fclose (rawfile);
    }

    /* copy sine data on failure */
    fprintf (stderr, "Error reading '%s'!\n", filename);
    for (sample = 0; sample < WAVEFORM_SIZE; sample++) {
	wave_table[wavenum][sample] = wave_table[WAVE_SINE][sample];
    }

    return -1;
}


/*****************************************************************************
 *
 * HERMITE()
 *
 * Read from a wavetable or sample buffer using hermite interpolation.
 *
 *****************************************************************************/
sample_t 
hermite(sample_t *buf, int max, sample_t index) {
    sample_t	tension	= -0.5;
    sample_t	bias	= 0.0;
    sample_t	mu;
    sample_t	mu2;
    sample_t	mu3;
    sample_t	m0;
    sample_t	m1;
    sample_t	a0;
    sample_t	a1;
    sample_t	a2;
    sample_t	a3;
    sample_t	y0;
    sample_t	y1;
    sample_t	y2;
    sample_t	y3;
    sample_t	index_floor;
    int		index_int;

    /* integer value of index */
    index_floor = floor (index);
    index_int = (int)index_floor;

    /* fractional portion of index */
    mu = index - index_floor;
    mu2 = mu * mu;
    mu3 = mu2 * mu;

    /* four adjacent samples, with higher precision index in the middle */
    y0 = buf[(index_int - 1) % max];
    y1 = buf[(index_int)     % max];
    y2 = buf[(index_int + 1) % max];
    y3 = buf[(index_int + 2) % max];

    /* slope of first and second segments */
    m0  = ((y1 - y0) * (1.0 + bias) * (1.0 - tension) * 0.5) + ((y2 - y1) * (1.0 - bias) * (1.0 - tension) * 0.5);
    m1  = ((y2 - y1) * (1.0 + bias) * (1.0 - tension) * 0.5) + ((y3 - y2) * (1.0 - bias) * (1.0 - tension) * 0.5);

    /* setup the first part of the hermite polynomial */
    a0 = ( 2 * mu3) - (3 * mu2) + 1;
    a1 = (     mu3) - (2 * mu2) + mu;
    a2 = (     mu3) - (    mu2);
    a3 = (-2 * mu3) + (3 * mu2);

    /* return the interpolated sample on the hermite curve */
    return ((a0 * y1) + (a1 * m0) + (a2 * m1) + (a3 * y2));
}


/*****************************************************************************
 *
 * Functions for the generating the waveform samples
 * The synth engine should never use these directly due to overhead.
 * Use the wave table instead!
 *
 *****************************************************************************/


/*****************************************************************************
 *
 * FUNC_SQUARE()
 *
 * Square function for building wave table
 *
 *****************************************************************************/
sample_t
func_square(unsigned int sample_num) {
    if (sample_num < (WAVEFORM_SIZE / 2)) {
	return 1.0;
    }
    return -1.0;
}


/*****************************************************************************
 *
 * FUNC_SAW()
 *
 * Saw function for building wave table
 *
 *****************************************************************************/
sample_t
func_saw(unsigned int sample_num) {
    return (2.0 * (sample_t)sample_num / F_WAVEFORM_SIZE) - 1.0;
}


/*****************************************************************************
 *
 * FUNC_REVSAW()
 *
 * Reverse saw wave function for building wave table.
 *
 *****************************************************************************/
sample_t
func_revsaw(unsigned int sample_num) {
    return (2.0 * (F_WAVEFORM_SIZE - (sample_t)sample_num) / F_WAVEFORM_SIZE) - 1.0;
}


/*****************************************************************************
 *
 * FUNC_TRIANGLE()
 *
 * Triangle wave function for building wave table
 *
 *****************************************************************************/
sample_t
func_triangle(unsigned int sample_num) {
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return 4.0 * (sample_t)sample_num / F_WAVEFORM_SIZE;
    }
    sample_num -= (WAVEFORM_SIZE / 4);
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return 4.0 * ((F_WAVEFORM_SIZE / 4.0) - (sample_t)sample_num) / F_WAVEFORM_SIZE;
    }
    sample_num -= (WAVEFORM_SIZE / 4);
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return -4.0 * (sample_t)sample_num / F_WAVEFORM_SIZE;
    }
    sample_num -= (WAVEFORM_SIZE / 4);
    return -4.0 * ((F_WAVEFORM_SIZE / 4.0) - (sample_t)sample_num) / F_WAVEFORM_SIZE;
}


/*****************************************************************************
 *
 * FUNC_STAIR()
 *
 * Stairstep wave function for building wave table
 *
 *****************************************************************************/
sample_t
func_stair(unsigned int sample_num) {
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return 0.0;
    }
    sample_num -= (WAVEFORM_SIZE / 4);
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return 1.0;
    }
    sample_num -= (WAVEFORM_SIZE / 4);
    if (sample_num < (WAVEFORM_SIZE / 4)) {
	return 0.0;
    }
    return -1.0;
}
