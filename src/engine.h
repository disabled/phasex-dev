/*****************************************************************************
 *
 * engine.h
 *
 * PHASEX:  [P]hase [H]armonic [A]dvanced [S]ynthesis [EX]periment
 *
 * Copyright (C) 1999-2009 William Weston <weston@sysex.net>
 *               2010 Anton Kormakov <assault64@gmail.com>
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
#ifndef _PHASEX_ENGINE_H_
#define _PHASEX_ENGINE_H_

#include "phasex.h"
#include "wave.h"
#include "midi.h"
#include "jack.h"


/* Basis by which oscillator frequency and phase triggering is set */
#define FREQ_BASE_MIDI_KEY		0
#define FREQ_BASE_INPUT_1		1
#define FREQ_BASE_INPUT_2		2
#define FREQ_BASE_INPUT_STEREO		3
#define FREQ_BASE_AMP_ENVELOPE		4
#define FREQ_BASE_FILTER_ENVELOPE	5
#define FREQ_BASE_VELOCITY		6
#define FREQ_BASE_TEMPO			7
#define FREQ_BASE_TEMPO_KEYTRIG		8

/* Modulation types available on a per-oscillator basis */
#define MOD_TYPE_OFF			0
#define MOD_TYPE_MIX			1
#define MOD_TYPE_AM			2
#define MOD_TYPE_NONE			3

/* Modes defining note to oscillator mapping and envelope handling */
#define KEYMODE_MONO_SMOOTH		0
#define KEYMODE_MONO_RETRIGGER		1
#define KEYMODE_MONO_MULTIKEY		2
#define KEYMODE_POLY			3

/* Filter keyfollow modes */
#define KEYFOLLOW_NONE			0
#define KEYFOLLOW_LAST			1
#define KEYFOLLOW_HIGH			2
#define KEYFOLLOW_LOW			3
#define KEYFOLLOW_MIDI			4

/* [-1,1] or [0,1] scaling on waveform */
#define POLARITY_BIPOLAR		0
#define POLARITY_UNIPOLAR		1

/* Signs */
#define SIGN_NEGATIVE			0
#define	SIGN_POSITIVE			1

/* envelope intervals */
#define ENV_INTERVAL_ATTACK		0
#define ENV_INTERVAL_DECAY		1
#define ENV_INTERVAL_SUSTAIN		2
#define ENV_INTERVAL_RELEASE		3
#define ENV_INTERVAL_DONE		4


/* internal global parameters used by synth engine */
typedef struct global {
    sample_t	bps;				/* beats per second				*/
    sample_t	in1;				/* input sample 2				*/
    sample_t	in2;				/* input sample 1				*/
    sample_t	out1;				/* output sample 2				*/
    sample_t	out2;				/* output sample 1				*/
#ifdef DEBUG_SAMPLE_VALUES
    sample_t	max1;
    sample_t	max2;
#endif
} GLOBAL;


/* internal per voice parameters used by synth engine */
typedef struct voice {
    short	active;				/* active flag, for poly mode			*/
    short	allocated;			/* only allocated voices become active		*/
    short	midi_key;			/* midi note in play				*/
    short	keypressed;			/* midi key currently held			*/
    short	need_portamento;		/* does we need portamento handling?		*/
    short	vol_key;			/* midi note to use for volume keyfollow	*/
    short	osc_wave;			/* internal value for osc wave num		*/
    short	cur_amp_interval;		/* current interval within the envelope         */
    int		cur_amp_sample;			/* sample number within current envelope peice  */
    int		portamento_sample;	       	/* sample number within portamento		*/
    int		portamento_samples;		/* portamento time in samples			*/
    int		age;				/* voice age, in samples			*/
    sample_t	out1;				/* output sample 1				*/
    sample_t	out2;				/* output sample 2				*/
    sample_t	amp_env;			/* smoothed final output of env generator	*/
    sample_t	amp_env_raw;			/* raw envelope value (before smoothing)	*/
    int		amp_env_dur[6];			/* envelope duration in samples			*/
    sample_t	amp_env_delta[6];		/* envelope value increment			*/
    sample_t	osc_out1[NUM_OSCS+1];		/* output waveform value 1			*/
    sample_t	osc_out2[NUM_OSCS+1];		/* output waveform value 2			*/
    sample_t	osc_freq[NUM_OSCS];		/* oscillator wave frequency used by engine	*/
    sample_t	osc_portamento[NUM_OSCS];	/* sample-wise freq adjust amt for portamento	*/
    sample_t	osc_phase_adjust[NUM_OSCS];	/* phase adjustment to wavetable index		*/
    sample_t	index[NUM_OSCS];		/* unconverted index into waveform lookup table	*/
    short	osc_key[NUM_OSCS];		/* current midi note for each osc		*/
    sample_t	filter_key_adj;			/* index adjustment to use for filter keyfollow	*/
    sample_t	filter_lp1;			/* filter lowpass output 1			*/
    sample_t	filter_lp2;			/* filter lowpass output 2			*/
    sample_t	filter_hp1;			/* filter highpass output 1			*/
    sample_t	filter_hp2;			/* filter highpass output 2			*/
    sample_t	filter_bp1;			/* filter bandpass output 1			*/
    sample_t	filter_bp2;			/* filter bandpass output 2			*/
    sample_t	filter_env;			/* smoothed final filter envelope output	*/
    sample_t	filter_env_raw;			/* raw envelope value (before smoothing)	*/
    sample_t	filter_env_delta[6];		/* envelope value increment			*/
    int		filter_env_dur[6];		/* envelope duration in samples			*/
    int		cur_filter_sample;		/* sample number within current envelope peice  */
    short	cur_filter_interval;		/* current interval within the envelope         */
    short	velocity;			/* per-voice velocity raw value			*/
    sample_t	velocity_coef_linear;		/* per-voice linear velocity coefficient	*/
    sample_t	velocity_target_linear;		/* target for velocity_coef_linear smoothing	*/
    sample_t	velocity_coef_log;		/* per-voice logarithmic velocity coefficient	*/
    sample_t	velocity_target_log;		/* target for velocity_coef_log smoothing	*/
} VOICE;


/* linked list of keys currently held in play */
typedef struct keylist {
    struct keylist	*next;
    short		midi_key;
} KEYLIST;


/* internal per part (per patch) parameters used by synth engine */
typedef struct part {
    KEYLIST	keylist[128];
    KEYLIST	*head;
    KEYLIST	*cur;
    KEYLIST	*prev;
    sample_t	delay_buf[(DELAY_MAX+2)*2];	/* stereo delay circular buffer			*/
#ifdef INTERPOLATE_CHORUS
    sample_t	chorus_buf_1[(CHORUS_MAX+2)];	/* left mono chorus circular buffer		*/
    sample_t	chorus_buf_2[(CHORUS_MAX+2)];	/* right mono chorus circular buffer		*/
#else
    sample_t	chorus_buf[(CHORUS_MAX+2)*2];	/* stereo chorus circular buffer		*/
#endif
    int		portamento_samples;		/* portamento time in samples			*/
    int		portamento_sample;	       	/* sample number within portamento		*/
    short	midi_key;			/* last midi key pressed			*/
    short	prev_key;			/* previous to last midi key pressed		*/
    short	last_key;			/* last key put into play			*/
    short	high_key;			/* highest oscillator key in play		*/
    short	low_key;			/* lowest oscillator key in play		*/
    short	velocity;			/* most recent note-on velocity			*/
    sample_t	velocity_coef;			/* velocity coefficient for calculations	*/
    sample_t	velocity_target;		/* target for velocity_coef smoothing		*/
    sample_t	filter_cutoff_target;		/* filter value smoothing algorithm moves to	*/
    sample_t	filter_smooth_len;		/* number of samples used for smoothing cutoff	*/
    sample_t	filter_smooth_factor;		/* 1.0 / (filter_smooth_len + 1.0)		*/
    sample_t	delay_size;			/* length of delay buffer in samples		*/
    int		delay_write_index;		/* delay_buffer write position			*/
    int		delay_read_index;		/* delay_buffer read position (lfo modulated)	*/
    int		delay_bufsize;			/* size of delay buffer in samples		*/
    int		delay_length;			/* integer length lf delay buffer in samples	*/
    int		chorus_write_index;		/* chorus_buffer write position			*/
    int		chorus_delay_index;		/* chorus_buffer feedback read position		*/
#ifdef INTERPOLATE_CHORUS
    sample_t	chorus_read_index_a;		/* chorus_buffer read position (lfo modulated)	*/
    sample_t	chorus_read_index_b;		/* chorus_buffer read position (offset 90 deg)	*/
    sample_t	chorus_read_index_c;		/* chorus_buffer read position (offset 180 deg)	*/
    sample_t	chorus_read_index_d;		/* chorus_buffer read position (offset 270 deg)	*/
#else
    int		chorus_read_index_a;		/* chorus_buffer read position (lfo modulated)	*/
    int		chorus_read_index_b;		/* chorus_buffer read position (offset 90 deg)	*/
    int		chorus_read_index_c;		/* chorus_buffer read position (offset 180 deg)	*/
    int		chorus_read_index_d;		/* chorus_buffer read position (offset 270 deg)	*/
#endif
    int		chorus_bufsize;			/* size of chorus buffer in samples		*/
    int		chorus_len;			/* int   length of chorus buffer in samples	*/
    sample_t	chorus_size;			/* float length of chorus buffer in samples	*/
    sample_t	chorus_lfo_freq;		/* chorus lfo frequency				*/
    sample_t	chorus_lfo_adjust;		/* chorus lfo index sample-by-sample adjustor	*/
    sample_t	chorus_lfo_index;		/* master index into chorus lfo			*/
    sample_t	chorus_lfo_index_a;		/* index into chorus lfo			*/
    sample_t	chorus_lfo_index_b;		/* index into chorus lfo (offset ~90 deg)	*/
    sample_t	chorus_lfo_index_c;		/* index into chorus lfo (offset 180 deg)	*/
    sample_t	chorus_lfo_index_d;		/* index into chorus lfo (offset ~270 deg)	*/
    sample_t	chorus_phase_index_a;		/* index into chorus phase lfo			*/
    sample_t	chorus_phase_index_b;		/* index into chorus phase lfo+90		*/
    sample_t	chorus_phase_index_c;		/* index into chorus phase lfo+180		*/
    sample_t	chorus_phase_index_d;		/* index into chorus phase lfo+270		*/
    sample_t	chorus_phase_freq;		/* chorus phase lfo frequency			*/
    sample_t	chorus_phase_adjust;		/* chorus phase lfo index increment size	*/
    sample_t	chorus_phase_amount_a;		/* amount to mix from lfo based position	*/
    sample_t	chorus_phase_amount_b;		/* amount to mix from lfo+90 based position	*/
    sample_t	chorus_phase_amount_c;		/* amount to mix from lfo+180 based position	*/
    sample_t	chorus_phase_amount_d;		/* amount to mix from lfo+270 based position	*/
    sample_t	pitch_bend_base;	      	/* pitch bender, scaled to [-1, 1]		*/
    sample_t	pitch_bend_target;	      	/* target for pitch bender smoothing		*/
    sample_t	out1;				/* output sample 2				*/
    sample_t	out2;				/* output sample 1				*/
    short	osc_wave[NUM_OSCS];		/* current wave for osc, including wave lfo	*/
    short	osc_am_mod[NUM_OSCS];		/* osc to use as AM modulator			*/
    short	osc_freq_mod[NUM_OSCS];		/* osc to use as FM modulator			*/
    short	osc_phase_mod[NUM_OSCS];	/* osc to use as phase modulator		*/
    sample_t	osc_init_index[NUM_OSCS];	/* initial phase index for oscillator		*/
    sample_t	osc_pitch_bend[NUM_OSCS];	/* current per-osc pitchbend amount		*/
    sample_t	osc_phase_adjust[NUM_OSCS];	/* phase adjustment to wavetable index		*/
    sample_t	lfo_pitch_bend[NUM_LFOS+1];	/* current per-LFO pitchbend amount		*/
    sample_t	lfo_freq[NUM_LFOS+1];		/* lfo frequency				*/
    sample_t	lfo_init_index[NUM_LFOS+1];	/* initial phase index for LFO waveform		*/
    sample_t	lfo_adjust[NUM_LFOS+1];		/* num samples to adjust for current lfo	*/
    sample_t	lfo_portamento[NUM_LFOS+1];	/* sample-wise freq adjust amt for portamento	*/
    sample_t	lfo_index[NUM_LFOS+1];		/* unconverted index into waveform lookup table	*/
    sample_t	lfo_out[NUM_LFOS+2];		/* raw sample output for LFOs			*/
    short	lfo_key[NUM_LFOS+1];		/* current midi note for each lfo		*/
} PART;


extern VOICE		voice[MAX_VOICES];
extern PART		part;
extern GLOBAL		global;
extern int      hold_pedal;

extern pthread_mutex_t	engine_ready_mutex;
extern pthread_cond_t	engine_ready_cond;
extern int		engine_ready;


void init_parameters(void);
void *engine_thread(void *arg);

void engine_notes_off(void);
void engine_panic(void);

#endif /* _PHASEX_ENGINE_H_ */
