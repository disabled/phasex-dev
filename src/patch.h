/*****************************************************************************
 *
 * patch.h
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
#ifndef _PHASEX_PATCH_H_
#define _PHASEX_PATCH_H_

#include "phasex.h"


/* Number of elements in an array */
#define NELEM(a)		( sizeof(a) / sizeof((a)[0]) )

#define PATCH_NAME_LEN		24


/* list of patch directories for file chooser shortcuts */
typedef struct patch_dir {
    char		*name;
    int			load_shortcut;
    int			save_shortcut;
    struct patch_dir	*next;
} PATCH_DIR_LIST;


/* all the parameters that define a patch */
/* everything marked as _cc is the MIDI controllor holding place for the actual floating point values	*/
typedef struct patch {
    /* supporting data */
    char	*name;				/* Patch name						*/
    char	*filename;			/* filename of patch currently loaded			*/
    char	*directory;			/* directory, stripped from patch filename		*/
    int		modified;			/* flag to track modification for quit confirmation	*/

    /* general parameters */
    sample_t	volume;				/* patch volume						*/
    short	volume_cc;
    sample_t	bpm;				/* beats per minute					*/
    short	bpm_cc;
    short	master_tune;			/* master tuning, in 1/128th halfsteps off A2FREQ	*/
    short	master_tune_cc;
    short	keymode;			/* ket to osc mode (mono_smooth or mono_multikey)	*/
    short	keyfollow_vol;			/* keyfollow volume ctlr (0 weights low freq, 127 high) */
    short	portamento;			/* portamento level					*/
    short	transpose;			/* halfsteps transpose, +/-				*/
    short	transpose_cc;			/* transpose midi cc [0=-64, 64=0, 127=63]		*/
    sample_t	input_boost;			/* sixteenth boost in input amplitude			*/
    short	input_boost_cc;
    short	input_follow;			/* apply input envelope follower			*/
    short	pan_cc;				/* panning adjustment (0=right, 64=center, 127=left)	*/
    sample_t	stereo_width;			/* width of stereo image (0=mono 127=stereo)		*/
    short	stereo_width_cc;
    sample_t	amp_velocity;			/* amp velocity sensitivity				*/
    short	amp_velocity_cc;

    /* amplifier envelope parameters */
    short	amp_attack;			/* amp envelope attack time				*/
    short	amp_decay;			/* amp envelope decay time				*/
    sample_t	amp_sustain;			/* amp envelope sustain level				*/
    short	amp_sustain_cc;
    short	amp_release;			/* amp envelope release time				*/

    /* delay parameters */
    short	delay_flip;			/* flip channels each time through delay ?		*/
    sample_t	delay_mix;			/* delay input to output mixing ratio			*/
    short	delay_mix_cc;
    sample_t	delay_feed;			/* delay feedback					*/
    short	delay_feed_cc;
    sample_t	delay_time;			/* length of delay buffer in bars			*/
    short	delay_time_cc;
    short	delay_lfo;			/* lfo to apply to delay				*/
    short	delay_lfo_cc;

    /* chorus parameters */
    short	chorus_flip;			/* flip channels each time through chorus ?		*/
    sample_t	chorus_mix;			/* chorus input to output mixing ratio			*/
    short	chorus_mix_cc;
    sample_t	chorus_feed;			/* chorus feedback					*/
    short	chorus_feed_cc;
    sample_t	chorus_time;			/* length of chorus buffer in samples			*/
    short	chorus_time_cc;
    sample_t	chorus_amount;			/* amount of chorus delay buffer for LFO to span	*/
    short	chorus_amount_cc;
    sample_t	chorus_phase_rate;		/* chorus phase offset lfo rate				*/
    short	chorus_phase_rate_cc;
    sample_t	chorus_phase_balance;		/* amount for LFO to set stereo crossover phase offset	*/
    short	chorus_phase_balance_cc;
    short	chorus_lfo_wave;		/* waveform to use for chorus read lfo			*/
    sample_t	chorus_lfo_rate;		/* chorus lfo rate					*/
    short	chorus_lfo_rate_cc;

    /* filter parameters */
    sample_t	filter_lfo_rate;		/* rate for filter lfo in bars per oscillation		*/
    short	filter_lfo_rate_cc;
    sample_t	filter_cutoff;			/* filter cutoff frequency (midi note number)		*/
    short	filter_cutoff_cc;
    sample_t	filter_resonance;		/* filter resonance (sorry... no self-oscillation)	*/
    short	filter_resonance_cc;
    short	filter_smoothing;		/* amount of smoothing between param value changes	*/
    short	filter_keyfollow;		/* filter cutoff follows note frequency ?		*/
    short	filter_mode;			/* filter mode (0 = lp, 1 = hp, 2 = bp, 3 = bs, 4 = pk)	*/
    short	filter_type;			/* filter type (0 - 3; all Chamberlin variations)	*/
    sample_t	filter_gain;			/* filter gain at input (in cents: 100=1.0, 127=1.27)	*/
    short	filter_gain_cc;
    sample_t	filter_env_amount;		/* filter envelope coefficient (uses amp envelope)	*/
    short	filter_env_amount_cc;
    sample_t	filter_env_sign;		/* filter envelope sign (+1 or -1)			*/
    short	filter_env_sign_cc;
    short	filter_attack;			/* filter envelope attack time				*/
    short	filter_decay;			/* filter envelope decay time				*/
    sample_t	filter_sustain;			/* filter envelope sustain level			*/
    short	filter_sustain_cc;
    short	filter_release;			/* filter envelope release time				*/
    short	filter_lfo;			/* lfo to apply to filter				*/
    short	filter_lfo_cc;
    sample_t	filter_lfo_cutoff;		/* amount of lfo modulation for filter cutoff		*/
    short	filter_lfo_cutoff_cc;
    sample_t	filter_lfo_resonance;		/* amount of lfo modulation for filter resonance	*/
    short	filter_lfo_resonance_cc;

    /* oscillator paramaters */
    short	osc_wave[NUM_OSCS];		/* waveform number for each oscillator			*/
    short	osc_freq_base[NUM_OSCS];	/* frequency basis (0=wave; 1=env; 2=cc)		*/
    sample_t	osc_rate[NUM_OSCS];		/* base oscillator frequency (before pitch/phase shift)	*/
    short	osc_rate_cc[NUM_OSCS];
    sample_t	osc_init_phase[NUM_OSCS];      	/* initial phase position for oscillator wave		*/
    short	osc_init_phase_cc[NUM_OSCS];
    short	osc_polarity_cc[NUM_OSCS];	/* scale wave from [-1,1] or [0,1] ?			*/
    short	osc_channel[NUM_OSCS];		/* midi channel for this oscillator			*/
    short	osc_modulation[NUM_OSCS];	/* modulation type (0=AM; 1=Mix)			*/
    sample_t	osc_transpose[NUM_OSCS];	/* number of halfsteps to transpose each oscillator	*/
    short	osc_transpose_cc[NUM_OSCS];
    sample_t	osc_fine_tune[NUM_OSCS];	/* number of 1/120th halfsteps to fine tune oscillator	*/
    short	osc_fine_tune_cc[NUM_OSCS];
    sample_t	osc_pitchbend[NUM_OSCS];	/* per-osc amount (halfsteps) to bend in each direction	*/
    short	osc_pitchbend_cc[NUM_OSCS];
    short	am_mod_type[NUM_OSCS];		/* type of modulator for amplitude modulation		*/
    short	am_lfo[NUM_OSCS];		/* lfo to use for amplitude modulation			*/
    short	am_lfo_cc[NUM_OSCS];
    sample_t	am_lfo_amount[NUM_OSCS];	/* range of am lfo					*/
    short	am_lfo_amount_cc[NUM_OSCS];
    short	freq_mod_type[NUM_OSCS];	/* type of modulator for frequency shift		*/
    short	freq_lfo[NUM_OSCS+1];		/* lfo to use for frequency shift			*/
    short	freq_lfo_cc[NUM_OSCS];
    sample_t	freq_lfo_amount[NUM_OSCS];	/* range of freq shift lfo in half steps		*/
    short	freq_lfo_amount_cc[NUM_OSCS];
    sample_t	freq_lfo_fine[NUM_OSCS];	/* range of freq shift lfo in 1/120th half steps	*/
    short	freq_lfo_fine_cc[NUM_OSCS];
    short	phase_mod_type[NUM_OSCS];	/* modulator type for phase shift			*/
    short	phase_lfo[NUM_OSCS];		/* lfo to use for phase shift				*/
    short	phase_lfo_cc[NUM_OSCS];
    sample_t	phase_lfo_amount[NUM_OSCS];	/* range of phase shift lfo in degrees			*/
    short	phase_lfo_amount_cc[NUM_OSCS];
    short	wave_lfo[NUM_OSCS];		/* lfo to use for wave selector lfo			*/
    short	wave_lfo_cc[NUM_OSCS];
    sample_t	wave_lfo_amount[NUM_OSCS];	/* range of wave selector lfo				*/
    short	wave_lfo_amount_cc[NUM_OSCS];

    /* lfo parameters */
    short	lfo_wave[NUM_LFOS+1];		/* waveform to use for lfo				*/
    short	lfo_freq_base[NUM_LFOS+1];	/* freq_base for lfo (0=wave; 1=env; 2=cc)		*/
    sample_t	lfo_init_phase[NUM_LFOS+1];	/* initial phase position for lfo			*/
    short	lfo_init_phase_cc[NUM_LFOS+1];
    sample_t	lfo_polarity[NUM_LFOS+1];	/* polarity values for lfo (0=bipolar; 1=unipolar)	*/
    short	lfo_polarity_cc[NUM_LFOS+1];
    sample_t	lfo_rate[NUM_LFOS+1];		/* rate of lfo, in (96ths - 1)				*/
    short	lfo_rate_cc[NUM_LFOS+1];
    short	lfo_transpose[NUM_LFOS+1];	/* number of halfsteps to transpose each lfo		*/
    short	lfo_transpose_cc[NUM_LFOS+1];
    sample_t	lfo_pitchbend[NUM_LFOS+1];	/* per-lfo amount (halfsteps) to bend in each direction	*/
    short	lfo_pitchbend_cc[NUM_LFOS+1];
} PATCH;


/* states for state machine */
/* numbering is important (state+=8 and state+=256 used in parser) */

#define PARSE_STATE_DEF_TYPE				0

#define PARSE_STATE_PATCH_START				1
#define PARSE_STATE_PATCH_END				2

#define PARSE_STATE_SUBSECT_ENTRY			3


#define PARSE_STATE_OSC_START				16
#define PARSE_STATE_LFO_START				17
#define PARSE_STATE_ENV_START				18
#define PARSE_STATE_FILTER_START			19
#define PARSE_STATE_DELAY_START				20
#define PARSE_STATE_GENERAL_START			21
#define PARSE_STATE_CHORUS_START			22
#define PARSE_STATE_UNKNOWN_START			23

#define PARSE_STATE_OSC_ATTR				32
#define PARSE_STATE_LFO_ATTR				33
#define PARSE_STATE_ENV_ATTR				34
#define PARSE_STATE_FILTER_ATTR				35
#define PARSE_STATE_DELAY_ATTR				36
#define PARSE_STATE_GENERAL_ATTR			37
#define PARSE_STATE_CHORUS_ATTR				38
#define PARSE_STATE_UNKNOWN_ATTR			39

#define PARSE_STATE_OSC_ATTR_END			48
#define PARSE_STATE_LFO_ATTR_END			49
#define PARSE_STATE_ENV_ATTR_END			50
#define PARSE_STATE_FILTER_ATTR_END			51
#define PARSE_STATE_DELAY_ATTR_END			52
#define PARSE_STATE_GENERAL_ATTR_END			53
#define PARSE_STATE_CHORUS_ATTR_END			54
#define PARSE_STATE_UNKNOWN_ATTR_END			55


#define PARSE_STATE_OSC_WAVE_EQ				256
#define PARSE_STATE_OSC_INIT_PHASE_EQ			257
#define PARSE_STATE_OSC_POLARITY_EQ			258
#define PARSE_STATE_OSC_FREQ_BASE_EQ			259
#define PARSE_STATE_OSC_MODULATION_EQ			260
#define PARSE_STATE_OSC_RATE_EQ				261
#define PARSE_STATE_OSC_TRANSPOSE_EQ			262
#define PARSE_STATE_OSC_FINE_TUNE_EQ			263
#define PARSE_STATE_OSC_PITCHBEND_AMOUNT_EQ		264
#define PARSE_STATE_OSC_MIDI_CHANNEL_EQ			265
#define PARSE_STATE_OSC_AM_LFO_EQ			266
#define PARSE_STATE_OSC_AM_LFO_AMOUNT_EQ		267
#define PARSE_STATE_OSC_FREQ_LFO_EQ			268
#define PARSE_STATE_OSC_FREQ_LFO_AMOUNT_EQ		269
#define PARSE_STATE_OSC_FREQ_LFO_FINE_EQ		270
#define PARSE_STATE_OSC_PHASE_LFO_EQ			271
#define PARSE_STATE_OSC_PHASE_LFO_AMOUNT_EQ		272
#define PARSE_STATE_OSC_WAVE_LFO_EQ			273
#define PARSE_STATE_OSC_WAVE_LFO_AMOUNT_EQ		274

#define PARSE_STATE_LFO_WAVE_EQ				275
#define PARSE_STATE_LFO_POLARITY_EQ			276
#define PARSE_STATE_LFO_FREQ_BASE_EQ			277
#define PARSE_STATE_LFO_RATE_EQ				278
#define PARSE_STATE_LFO_INIT_PHASE_EQ			279
#define PARSE_STATE_LFO_TRANSPOSE_EQ			280
#define PARSE_STATE_LFO_PITCHBEND_AMOUNT_EQ		281

#define PARSE_STATE_ENV_ATTACK_EQ			282
#define PARSE_STATE_ENV_DECAY_EQ			283
#define PARSE_STATE_ENV_SUSTAIN_EQ			284
#define PARSE_STATE_ENV_RELEASE_EQ			285

#define PARSE_STATE_FILTER_CUTOFF_EQ			286
#define PARSE_STATE_FILTER_RESONANCE_EQ			287
#define PARSE_STATE_FILTER_SMOOTHING_EQ			288
#define PARSE_STATE_FILTER_KEYFOLLOW_EQ			289
#define PARSE_STATE_FILTER_MODE_EQ			290
#define PARSE_STATE_FILTER_TYPE_EQ			291
#define PARSE_STATE_FILTER_GAIN_EQ			292
#define PARSE_STATE_FILTER_ENV_AMOUNT_EQ		293
#define PARSE_STATE_FILTER_ENV_SIGN_EQ			294
#define PARSE_STATE_FILTER_ENV_ATTACK_EQ		295
#define PARSE_STATE_FILTER_ENV_DECAY_EQ			296
#define PARSE_STATE_FILTER_ENV_SUSTAIN_EQ		297
#define PARSE_STATE_FILTER_ENV_RELEASE_EQ		298
#define PARSE_STATE_FILTER_LFO_EQ			299
#define PARSE_STATE_FILTER_LFO_CUTOFF_EQ		300
#define PARSE_STATE_FILTER_LFO_RESONANCE_EQ		301

#define PARSE_STATE_DELAY_MIX_EQ			304
#define PARSE_STATE_DELAY_FEED_EQ			305
#define PARSE_STATE_DELAY_FLIP_EQ			306
#define PARSE_STATE_DELAY_TIME_EQ			307
#define PARSE_STATE_DELAY_LFO_EQ			308

#define PARSE_STATE_GENERAL_BPM_EQ			311
#define PARSE_STATE_GENERAL_MASTER_TUNE_EQ		312
#define PARSE_STATE_GENERAL_PORTAMENTO_EQ		313
#define PARSE_STATE_GENERAL_MIDI_CHANNEL_EQ		314
#define PARSE_STATE_GENERAL_KEYMODE_EQ			315
#define PARSE_STATE_GENERAL_KEYFOLLOW_VOL_EQ		316
#define PARSE_STATE_GENERAL_VOLUME_EQ			317
#define PARSE_STATE_GENERAL_TRANSPOSE_EQ		318
#define PARSE_STATE_GENERAL_INPUT_BOOST_EQ		319
#define PARSE_STATE_GENERAL_INPUT_FOLLOW_EQ		320
#define PARSE_STATE_GENERAL_PAN_EQ			321
#define PARSE_STATE_GENERAL_STEREO_WIDTH_EQ		322
#define PARSE_STATE_GENERAL_AMP_VELOCITY_EQ		323

#define PARSE_STATE_CHORUS_MIX_EQ			324
#define PARSE_STATE_CHORUS_FEED_EQ			325
#define PARSE_STATE_CHORUS_FLIP_EQ    			326
#define PARSE_STATE_CHORUS_TIME_EQ			327
#define PARSE_STATE_CHORUS_AMOUNT_EQ			328
#define PARSE_STATE_CHORUS_PHASE_RATE_EQ	       	329
#define PARSE_STATE_CHORUS_PHASE_BALANCE_EQ	       	330
#define PARSE_STATE_CHORUS_LFO_WAVE_EQ			331
#define PARSE_STATE_CHORUS_LFO_RATE_EQ			332

#define PARSE_STATE_UNKNOWN_EQ				333


#define PARSE_STATE_OSC_WAVE_VAL			512
#define PARSE_STATE_OSC_INIT_PHASE_VAL			513
#define PARSE_STATE_OSC_POLARITY_VAL			514
#define PARSE_STATE_OSC_FREQ_BASE_VAL			515
#define PARSE_STATE_OSC_MODULATION_VAL			516
#define PARSE_STATE_OSC_RATE_VAL			517
#define PARSE_STATE_OSC_TRANSPOSE_VAL			518
#define PARSE_STATE_OSC_FINE_TUNE_VAL			519
#define PARSE_STATE_OSC_PITCHBEND_AMOUNT_VAL		520
#define PARSE_STATE_OSC_MIDI_CHANNEL_VAL		521
#define PARSE_STATE_OSC_AM_LFO_VAL			522
#define PARSE_STATE_OSC_AM_LFO_AMOUNT_VAL		523
#define PARSE_STATE_OSC_FREQ_LFO_VAL			524
#define PARSE_STATE_OSC_FREQ_LFO_AMOUNT_VAL		525
#define PARSE_STATE_OSC_FREQ_LFO_FINE_VAL		526
#define PARSE_STATE_OSC_PHASE_LFO_VAL			527
#define PARSE_STATE_OSC_PHASE_LFO_AMOUNT_VAL		528
#define PARSE_STATE_OSC_WAVE_LFO_VAL			529
#define PARSE_STATE_OSC_WAVE_LFO_AMOUNT_VAL		530

#define PARSE_STATE_LFO_WAVE_VAL			531
#define PARSE_STATE_LFO_POLARITY_VAL			532
#define PARSE_STATE_LFO_FREQ_BASE_VAL			533
#define PARSE_STATE_LFO_RATE_VAL			534
#define PARSE_STATE_LFO_INIT_PHASE_VAL			535
#define PARSE_STATE_LFO_TRANSPOSE_VAL			536
#define PARSE_STATE_LFO_PITCHBEND_AMOUNT_VAL		537

#define PARSE_STATE_ENV_ATTACK_VAL			538
#define PARSE_STATE_ENV_DECAY_VAL			539
#define PARSE_STATE_ENV_SUSTAIN_VAL			540
#define PARSE_STATE_ENV_RELEASE_VAL			541

#define PARSE_STATE_FILTER_CUTOFF_VAL			542
#define PARSE_STATE_FILTER_RESONANCE_VAL		543
#define PARSE_STATE_FILTER_SMOOTHING_VAL		544
#define PARSE_STATE_FILTER_KEYFOLLOW_VAL		545
#define PARSE_STATE_FILTER_MODE_VAL			546
#define PARSE_STATE_FILTER_TYPE_VAL			547
#define PARSE_STATE_FILTER_GAIN_VAL			548
#define PARSE_STATE_FILTER_ENV_AMOUNT_VAL		549
#define PARSE_STATE_FILTER_ENV_SIGN_VAL			550
#define PARSE_STATE_FILTER_ENV_ATTACK_VAL		551
#define PARSE_STATE_FILTER_ENV_DECAY_VAL		552
#define PARSE_STATE_FILTER_ENV_SUSTAIN_VAL		553
#define PARSE_STATE_FILTER_ENV_RELEASE_VAL		554
#define PARSE_STATE_FILTER_LFO_VAL			555
#define PARSE_STATE_FILTER_LFO_CUTOFF_VAL		556
#define PARSE_STATE_FILTER_LFO_RESONANCE_VAL		557

#define PARSE_STATE_DELAY_MIX_VAL			560
#define PARSE_STATE_DELAY_FEED_VAL			561
#define PARSE_STATE_DELAY_FLIP_VAL			562
#define PARSE_STATE_DELAY_TIME_VAL			563
#define PARSE_STATE_DELAY_LFO_VAL			564

#define PARSE_STATE_GENERAL_BPM_VAL			567
#define PARSE_STATE_GENERAL_MASTER_TUNE_VAL		568
#define PARSE_STATE_GENERAL_PORTAMENTO_VAL		569
#define PARSE_STATE_GENERAL_MIDI_CHANNEL_VAL		570
#define PARSE_STATE_GENERAL_KEYMODE_VAL			571
#define PARSE_STATE_GENERAL_KEYFOLLOW_VOL_VAL		572
#define PARSE_STATE_GENERAL_VOLUME_VAL			573
#define PARSE_STATE_GENERAL_TRANSPOSE_VAL		574
#define PARSE_STATE_GENERAL_INPUT_BOOST_VAL		575
#define PARSE_STATE_GENERAL_INPUT_FOLLOW_VAL		576
#define PARSE_STATE_GENERAL_PAN_VAL			577
#define PARSE_STATE_GENERAL_STEREO_WIDTH_VAL		578
#define PARSE_STATE_GENERAL_AMP_VELOCITY_VAL		579

#define PARSE_STATE_CHORUS_MIX_VAL			580
#define PARSE_STATE_CHORUS_FEED_VAL			581
#define PARSE_STATE_CHORUS_FLIP_VAL			582
#define PARSE_STATE_CHORUS_TIME_VAL			583
#define PARSE_STATE_CHORUS_AMOUNT_VAL			584
#define PARSE_STATE_CHORUS_PHASE_RATE_VAL		585
#define PARSE_STATE_CHORUS_PHASE_BALANCE_VAL	       	586
#define PARSE_STATE_CHORUS_LFO_WAVE_VAL			587
#define PARSE_STATE_CHORUS_LFO_RATE_VAL			588

#define PARSE_STATE_UNKNOWN_VAL				589


extern PATCH		*patch;

extern PATCH_DIR_LIST	*patch_dir_list;

extern char		*wave_names[];
extern char		*boolean_names[];


sample_t get_rational_float(char *token);
int get_boolean(char *token, char *filename, int line);
sample_t get_rate_val(int ctlr);
int get_rate_ctlr(char *token, char *filename, int line);
char *get_next_token(char *inbuf);
void init_patch(PATCH *patch);
int read_patch(char *filename, int program_number);
int save_patch(char *filename, int program_number);


#endif /* _PHASEX_PATCH_H_ */
