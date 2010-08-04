/*****************************************************************************
 *
 * engine.c
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
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include "phasex.h"
#include "wave.h"
#include "filter.h"
#include "engine.h"
#include "patch.h"
#include "midi.h"
#include "jack.h"
#include "settings.h"


VOICE		voice[MAX_VOICES];
PART		part;
GLOBAL		global;
int         hold_pedal = 0;

pthread_mutex_t	engine_ready_mutex;
pthread_cond_t	engine_ready_cond	= PTHREAD_COND_INITIALIZER;
int		engine_ready		= 0;


/*****************************************************************************
 *
 * INIT_PARAMETERS()
 *
 * Initializes parameters used by engine.
 * Run once before engine enters its main loop.
 *
 *****************************************************************************/
void 
init_parameters(void) {
    int		osc;
    int		lfo;
    int		j;
    int		once = 1;

    if (once) {
	/* clear static mem */
	memset (&global, 0, sizeof (GLOBAL));
	memset (&part,   0, sizeof (PART));
	memset (&voice,  0, MAX_VOICES * sizeof (VOICE));

	/* no midi keys in play yet */
	part.head     = NULL;
	part.cur      = NULL;
	part.prev     = NULL;
	part.midi_key = -1;
	part.prev_key = -1;

	/* set buffer sizes and zero buffers */
	part.delay_bufsize = DELAY_MAX;
	memset (part.delay_buf, 0, part.delay_bufsize * 2 * sizeof (sample_t));
	part.chorus_bufsize = CHORUS_MAX;
#ifdef INTERPOLATE_CHORUS
	memset (part.chorus_buf_1, 0, part.chorus_bufsize * sizeof (sample_t));
	memset (part.chorus_buf_2, 0, part.chorus_bufsize * sizeof (sample_t));
#else
	memset (part.chorus_buf, 0, part.chorus_bufsize * 2 * sizeof (sample_t));
#endif

	/* initialize keylist nodes */
	for (j = 0; j < 128; j++) {
	    part.keylist[j].midi_key = j;
	    part.keylist[j].next     = NULL;
	}

	once = 0;
    }

    /* set initial bps from command line bpm */
    global.bps = patch->bpm / 60.0;
    midi->tick_sample = f_sample_rate / (global.bps * 24.0);

    /* init portamento */
    part.portamento_samples = env_table[patch->portamento];

    /* initialize delay */
    part.delay_size        = patch->delay_time * f_sample_rate / global.bps;
    part.delay_length	   = (int)(part.delay_size);
    part.delay_write_index = 0;

    /* initialize chorus */
    part.chorus_lfo_freq     = global.bps * patch->chorus_lfo_rate;
    part.chorus_size         = patch->chorus_time;
    part.chorus_len          = (int)(patch->chorus_time);
    part.chorus_lfo_adjust   = part.chorus_lfo_freq * wave_period;
    part.chorus_write_index  = 0;
    part.chorus_delay_index  = part.chorus_bufsize - part.chorus_len - 1;
    part.chorus_phase_freq   = global.bps * patch->chorus_phase_rate;
    part.chorus_phase_adjust = part.chorus_phase_freq * wave_period;

    /* set chorus phase lfo indices */
    part.chorus_phase_index_a = 0.0;

    part.chorus_phase_index_b = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.25);
    if (part.chorus_phase_index_b >= F_WAVEFORM_SIZE) {
	part.chorus_phase_index_b -= F_WAVEFORM_SIZE;
    }

    part.chorus_phase_index_c = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.5);
    if (part.chorus_phase_index_c >= F_WAVEFORM_SIZE) {
	part.chorus_phase_index_c -= F_WAVEFORM_SIZE;
    }

    part.chorus_phase_index_d = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.75);
    if (part.chorus_phase_index_d >= F_WAVEFORM_SIZE) {
	part.chorus_phase_index_d -= F_WAVEFORM_SIZE;
    }

    /* set amount used for mix weight for the chorus lfo positions at right angles */
    part.chorus_phase_amount_a = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_a])
	* 0.5 * (mix_table[127 - patch->chorus_phase_balance_cc]);

    part.chorus_phase_amount_b = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_b])
	* 0.5 * (mix_table[patch->chorus_phase_balance_cc]);

    part.chorus_phase_amount_c = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_c])
	* 0.5 * (mix_table[127 - patch->chorus_phase_balance_cc]);

    part.chorus_phase_amount_d = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_d])
	* 0.5 * (mix_table[patch->chorus_phase_balance_cc]);

    /* set chorus lfo indices */
    part.chorus_lfo_index_a = 0.0;

    if ((part.chorus_lfo_index_b = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.25)) >= F_WAVEFORM_SIZE) {
	part.chorus_lfo_index_b -= F_WAVEFORM_SIZE;
    }

    if ((part.chorus_lfo_index_c = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.5)) >= F_WAVEFORM_SIZE) {
	part.chorus_lfo_index_c -= F_WAVEFORM_SIZE;
    }

    if ((part.chorus_lfo_index_d = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.75)) >= F_WAVEFORM_SIZE) {
	part.chorus_lfo_index_d -= F_WAVEFORM_SIZE;
    }

    /* initialize pitch bend attrs */
    part.pitch_bend_target = part.pitch_bend_base = 0.0;

    /* initialize filter attrs */
    patch->filter_env_amount  = (sample_t)patch->filter_env_amount_cc;
    patch->filter_env_amount  *= patch->filter_env_sign;
    part.filter_cutoff_target = patch->filter_cutoff;

    /* init velocity */
    part.velocity        = 0;
    part.velocity_coef   = 1.0;
    part.velocity_target = 1.0;

    /* per-lfo setup (including LFO_OFF/LFO_VELOCITY) */
    for (lfo = 0; lfo <= NUM_LFOS; lfo++) {

	/* calculate initial indices from initial phases */
	part.lfo_init_index[lfo] = patch->lfo_init_phase[lfo] * F_WAVEFORM_SIZE;
	part.lfo_index[lfo]      = part.lfo_init_index[lfo];

	/* calculate frequency and corresponding index adjustment */
	part.lfo_freq[lfo]   = global.bps * patch->lfo_rate[lfo];
	part.lfo_adjust[lfo] = part.lfo_freq[lfo] * wave_period;

	/* zero out remaining floating point params */
	part.lfo_pitch_bend[lfo] = 0.0;
	part.lfo_portamento[lfo] = 0.0;
	part.lfo_out[lfo]        = 0.0;
    }

    /* per-oscillator setup */
    for (osc = 0; osc < NUM_OSCS; osc++) {
	/* calculate initial indices from initial phases */
	part.osc_init_index[osc] = patch->osc_init_phase[osc] * F_WAVEFORM_SIZE;

	/* zero out floating point params */
	part.osc_pitch_bend[osc] = 0.0;
    }

    /* now handle voice specific inits */
    for (j = 0; j < MAX_VOICES; j++) {
	/* init portamento and velocity */
	voice[j].portamento_samples     = env_table[patch->portamento];
	voice[j].velocity               = 0;
	voice[j].velocity_coef_linear   = 1.0;
	voice[j].velocity_target_linear = 1.0;
	voice[j].velocity_coef_log      = 1.0;
	voice[j].velocity_target_log    = 1.0;

	/* generate the size in samples and value deltas for our envelope intervals */
	voice[j].amp_env_dur[ENV_INTERVAL_ATTACK]    = env_table[patch->amp_attack];
	voice[j].amp_env_delta[ENV_INTERVAL_ATTACK]  = 1.0 / (sample_t)voice[j].amp_env_dur[ENV_INTERVAL_ATTACK];
	voice[j].amp_env_dur[ENV_INTERVAL_DECAY]     = env_table[patch->amp_decay];
	voice[j].amp_env_delta[ENV_INTERVAL_DECAY]   = (patch->amp_sustain - 1.0) / (sample_t)voice[j].amp_env_dur[ENV_INTERVAL_DECAY];
	voice[j].amp_env_dur[ENV_INTERVAL_SUSTAIN]   = 1;
	voice[j].amp_env_delta[ENV_INTERVAL_SUSTAIN] = 0.0;
	voice[j].amp_env_dur[ENV_INTERVAL_RELEASE]   = env_table[patch->amp_release];
	voice[j].amp_env_delta[ENV_INTERVAL_RELEASE] = (0.0 - patch->amp_sustain) / (sample_t)voice[j].amp_env_dur[ENV_INTERVAL_RELEASE];
	voice[j].amp_env_dur[ENV_INTERVAL_DONE]      = 44100;
	voice[j].amp_env_delta[ENV_INTERVAL_DONE]    = 0.0;

	/* initialize envelope state (after release, waiting for attack) and amplitude */
	voice[j].cur_amp_interval = 4;
	voice[j].cur_amp_sample   = voice[j].amp_env_dur[ENV_INTERVAL_DONE];
	voice[j].amp_env          = 0.0;
	voice[j].amp_env_raw      = 0.0;

	/* generate the size in samples and value deltas for our envelope intervals */
	voice[j].filter_env_dur[ENV_INTERVAL_ATTACK]    = env_table[patch->filter_attack];
	voice[j].filter_env_delta[ENV_INTERVAL_ATTACK]  = 1.0 / (sample_t)voice[j].filter_env_dur[ENV_INTERVAL_ATTACK];
	voice[j].filter_env_dur[ENV_INTERVAL_DECAY]     = env_table[patch->filter_decay];
	voice[j].filter_env_delta[ENV_INTERVAL_DECAY]   = (patch->filter_sustain - 1.0) / (sample_t)voice[j].filter_env_dur[ENV_INTERVAL_DECAY];
	voice[j].filter_env_dur[ENV_INTERVAL_SUSTAIN]   = 1;
	voice[j].filter_env_delta[ENV_INTERVAL_SUSTAIN] = 0.0;
	voice[j].filter_env_dur[ENV_INTERVAL_RELEASE]   = env_table[patch->filter_release];
	voice[j].filter_env_delta[ENV_INTERVAL_RELEASE] = (0.0 - patch->filter_sustain) / (sample_t)voice[j].filter_env_dur[ENV_INTERVAL_RELEASE];
	voice[j].filter_env_dur[ENV_INTERVAL_DONE]      = 44100;
	voice[j].filter_env_delta[ENV_INTERVAL_DONE]    = 0.0;

	/* initialize envelope state (after release, waiting for attack) and amplitude */
	voice[j].cur_filter_interval = 4;
	voice[j].cur_filter_sample   = voice[j].filter_env_dur[4];
	voice[j].filter_env          = 0.0;
	voice[j].filter_env_raw      = 0.0;

	/* filter outputs */
	voice[j].filter_hp1 = voice[j].filter_hp2 = 0.0;
	voice[j].filter_lp1 = voice[j].filter_lp2 = 0.0;
	voice[j].filter_bp1 = voice[j].filter_bp2 = 0.0;

	/* signify that we're not in a portamento slide */
	voice[j].portamento_sample = -1;

	/* per-oscillator-per-voice setup */
	for (osc = 0; osc < NUM_OSCS; osc++) {

	    /* handle tempo based initial frequencies */
	    if (patch->osc_freq_base[osc] >= FREQ_BASE_TEMPO) {
		voice[j].osc_freq[osc] = patch->osc_rate[osc] * global.bps;
	    }

	    /* zero out floating point params */
	    voice[j].osc_portamento[osc] = 0.0;
	    voice[j].index[osc]          = 0.0;
	}

	/* initialize the MOD_OFF dummy-voice */
	voice[j].osc_out1[MOD_OFF] = 0.0;
	voice[j].osc_out2[MOD_OFF] = 0.0;

	/* set inactive and ageless */
	voice[j].active     = 0;
	voice[j].allocated  = 0;
	voice[j].age        = 0;
	voice[j].midi_key   = -1;
	voice[j].keypressed = -1;
    }

    /* mono gets voice 0 */
    if (patch->keymode != KEYMODE_POLY) {
	voice[0].active    = 1;
	voice[0].allocated = 1;
    }
}


/*****************************************************************************
 *
 * ENGINE_THREAD()
 *
 * Main sound synthesis thread.
 * This is where all the magick happens  ;-}
 *
 *****************************************************************************/
void *
engine_thread(void *arg) {
    struct sched_param	schedparam;
    pthread_t		thread_id;
    short		osc;			/* current oscillator				*/
    short		lfo;			/* current LFO					*/
    short		j;
    short		v;			/* current voice				*/
    short		odd_sample		= 1;
    sample_t		last_out1		= 0;
    sample_t		last_out2		= 0;
    sample_t		denormal_offset		= (sample_t)(1e-23);
    sample_t		freq_adjust;		/* num samples to adj for cur freq w/ shift	*/
    sample_t		phase_adjust1;		/* num samples adjustment for phase shift	*/
    sample_t		phase_adjust2;		/* num samples adjustment for phase shift	*/
    sample_t		amp_env_max;		/* max of amp env for all active voices		*/
    sample_t		filter_f;		/* filter F-value				*/
    sample_t		filter_q;		/* filter Q-factor (1 - ~resonance)		*/
    int			filter_index;		/* index into table of f-coefficients		*/
    sample_t		filter_env_max;		/* max of filter env for all active voices	*/
    sample_t		input_env_raw;		/* input envelope raw value			*/
    sample_t		input_env_attack;	/* input envelope follower attack in samples	*/
    sample_t		input_env_release;	/* input envelope follower release in samples	*/
    sample_t		tmp_1,tmp_2,tmp_3,tmp_4;/* stereo pairs of floating point temp vars	*/
    sample_t		tmp_1_a			= 0;
    sample_t		tmp_1_b			= 0;
    sample_t		tmp_1_c			= 0;
    sample_t		tmp_1_d			= 0;
    sample_t		tmp_2_a			= 0;
    sample_t		tmp_2_b			= 0;
    sample_t		tmp_2_c			= 0;
    sample_t		tmp_2_d			= 0;
#ifdef DEBUG_SAMPLE_VALUES
    int			debug_sample_counter	= 0;
#endif
    sample_t		aftertouch_smooth_len	= f_sample_rate * 0.25;		/* 1/4 second	*/
    sample_t		aftertouch_smooth_factor= 1.0 / (aftertouch_smooth_len + 1.0);

    if (debug) {
	fprintf (stderr, "Denormal Offset: %.37f\n", denormal_offset);
    }

    /* initialize input envelope follower */
    input_env_raw     = 0.0;
    input_env_attack  = exp (log (0.01) / (12));
    input_env_release = exp (log (0.01) / (24000));

    /* set realtime scheduling and priority */
    thread_id = pthread_self ();
    memset (&schedparam, 0, sizeof (struct sched_param));
    schedparam.sched_priority = setting_engine_priority;
    pthread_setschedparam (thread_id, setting_sched_policy, &schedparam);

    /* broadcast the engine ready condition */
    engine_ready = 1;
    pthread_cond_broadcast (&engine_ready_cond);

    /* MAIN LOOP: one time through for each sample */
    while (1) {

	/* generate envelopes for all voices */
	amp_env_max    = 0.0;
	filter_env_max = 0.0;
	for (v = 0; v < setting_polyphony; v++) {

	    /* skip over inactive voices */
	    if ((voice[v].allocated == 0)) {
		continue;
	    }

	    /* mark voice as active, since we know it's allocated */
	    voice[v].active = 1;

	    /* has the end of an envelope interval been reached? */
	    if (voice[v].cur_amp_sample < -1) {

		/* deal with envelope if not in key sustain */
		if (((voice[v].cur_amp_interval != ENV_INTERVAL_SUSTAIN) || (voice[v].keypressed == -1)) 
		    && (!hold_pedal || (voice[v].cur_amp_interval == ENV_INTERVAL_ATTACK))) {

		    /* move on to the next envelope interval */
		    if (voice[v].cur_amp_interval < ENV_INTERVAL_DONE) {
			voice[v].cur_amp_interval++;
			/* decay and release delta and duration get set here */
			switch (voice[v].cur_amp_interval) {
			case ENV_INTERVAL_DECAY:
			    voice[v].amp_env_dur[ENV_INTERVAL_DECAY]   = env_table[patch->amp_decay];
			    voice[v].amp_env_delta[ENV_INTERVAL_DECAY] =
				(patch->amp_sustain - 1.0) / (sample_t)voice[v].amp_env_dur[ENV_INTERVAL_DECAY];
			    break;
			case ENV_INTERVAL_RELEASE:
			    voice[v].amp_env_dur[ENV_INTERVAL_RELEASE]   = env_table[patch->amp_release];
			    voice[v].amp_env_delta[ENV_INTERVAL_RELEASE] =
				(0.0 - voice[v].amp_env_raw) / (sample_t)voice[v].amp_env_dur[ENV_INTERVAL_RELEASE];
			    break;
			}
		    }

		    /* unless the end of the envelope has been reached */
		    else {
			/* deactivate voice in poly and retrigger modes */
			if ((patch->keymode == KEYMODE_POLY) || (patch->keymode == KEYMODE_MONO_RETRIGGER)) {
			    voice[v].active    = 0;
			    voice[v].allocated = 0;
			    voice[v].age       = 0;
			    voice[v].midi_key  = -1;
			}
			/* for all modes, set envelope to zero */
			voice[v].amp_env_raw = 0.0;
		    }
		    voice[v].cur_amp_sample = voice[v].amp_env_dur[voice[v].cur_amp_interval];
		}
	    }

	    /* still inside the amp envelope interval */
	    else {

		/* decrement our sample number for this envelope interval */
		voice[v].cur_amp_sample--;

		/* add the per-sample delta to the envelope */
		if ((voice[v].amp_env_raw += voice[v].amp_env_delta[voice[v].cur_amp_interval]) < 0.0) {
		    voice[v].amp_env_raw = 0.0;
		}
		else if (voice[v].amp_env_raw > 1.0) {
		    voice[v].amp_env_raw = 1.0;
		}
		/* switch to release interval on NOTE_OFF when pedal is not held */
		if((voice[v].keypressed == -1) && (voice[v].cur_amp_interval < ENV_INTERVAL_SUSTAIN) && !hold_pedal)
		{
		    voice[v].cur_amp_sample = -2; 
		    voice[v].cur_amp_interval = ENV_INTERVAL_SUSTAIN;
		} 
	    }

	    /* now do the exact same thing for filter envelope */
	    if (voice[v].cur_filter_sample < 0) {

		/* deal with envelope if not in key sustain (or moving out of sustain) */
		if (((voice[v].cur_filter_interval != ENV_INTERVAL_SUSTAIN) || (voice[v].keypressed == -1)) 
		    && (!hold_pedal || (voice[v].cur_filter_interval == ENV_INTERVAL_ATTACK))) {

		    /* move on to the next envelope interval */
		    if (voice[v].cur_filter_interval < ENV_INTERVAL_DONE) {
			voice[v].cur_filter_interval++;
			/* decay and release delta and duration get set here */
			switch (voice[v].cur_filter_interval) {
			case ENV_INTERVAL_DECAY:
			    voice[v].filter_env_dur[ENV_INTERVAL_DECAY]   = env_table[patch->filter_decay];
			    voice[v].filter_env_delta[ENV_INTERVAL_DECAY] =
				(patch->filter_sustain - 1.0) / (sample_t)voice[v].filter_env_dur[ENV_INTERVAL_DECAY];
			    break;
			case ENV_INTERVAL_RELEASE:
			    voice[v].filter_env_dur[ENV_INTERVAL_RELEASE]   = env_table[patch->filter_release];
			    voice[v].filter_env_delta[ENV_INTERVAL_RELEASE] =
				(0.0 - voice[v].filter_env_raw) / (sample_t)voice[v].filter_env_dur[ENV_INTERVAL_RELEASE];
			    break;
			}
		    }

		    /* unless the end of the envelope has been reached */
		    else {
			voice[v].filter_env_raw = 0.0;
		    }
		    voice[v].cur_filter_sample = voice[v].filter_env_dur[voice[v].cur_filter_interval];
		}
	    }

	    /* still inside the filter envelope interval */
	    else {

		/* increment our sample number for this envelope interval */
		voice[v].cur_filter_sample--;

		/* add the per-sample delta to the envelope */
		if ((voice[v].filter_env_raw += voice[v].filter_env_delta[voice[v].cur_filter_interval]) < 0.0) {
		    voice[v].filter_env_raw = 0.0;
		}
		else if (voice[v].filter_env_raw > 1.0) {
		    voice[v].filter_env_raw = 1.0;
		}
		if((voice[v].keypressed == -1) && (voice[v].cur_filter_interval < ENV_INTERVAL_SUSTAIN) && !hold_pedal)
		{
		    voice[v].cur_filter_sample = -2; 
		    voice[v].cur_filter_interval = ENV_INTERVAL_SUSTAIN;
		}
	    }

	    /* find max of per-voice envelopes in case per-part lfo needs it */
	    if (voice[v].amp_env_raw > amp_env_max) {
		amp_env_max    = voice[v].amp_env_raw;
	    }
	    if (voice[v].filter_env_raw > filter_env_max) {
		filter_env_max = voice[v].filter_env_raw;
	    }
	}

	/* handle input envelope follower (boost is handled while copying from ringbuffer) */
	tmp_1 = fabs (global.in1) + fabs (global.in2);
	if (tmp_1 > 2.0) {
	    tmp_1 = 1.0;
	}
	else {
	    tmp_1 *= 0.5;
	}
	if (tmp_1 > input_env_raw) {
	    input_env_raw = input_env_attack  * (input_env_raw - tmp_1) + tmp_1;
	}
	else {
	    input_env_raw = input_env_release * (input_env_raw - tmp_1) + tmp_1;
	}

	/* pitch bender smoothing */
	part.pitch_bend_base =
	    ((aftertouch_smooth_len * part.pitch_bend_base)
	     + part.pitch_bend_target) * aftertouch_smooth_factor;

	/* generate output from lfos */
	for (lfo = 0; lfo < NUM_LFOS; lfo++) {

	    /* current pitch bend for this lfo */
	    part.lfo_pitch_bend[lfo] = part.pitch_bend_base * patch->lfo_pitchbend[lfo];

	    /* grab the generic waveform data */
	    switch (patch->lfo_freq_base[lfo]) {

	    case FREQ_BASE_MIDI_KEY:
		/* handle portamento if necessary */
		if (part.portamento_sample > 0) {
		    part.lfo_freq[lfo] += part.lfo_portamento[lfo];
		}
		/* otherwise set frequency directly */
		else {
		    part.lfo_freq[lfo] = freq_table[patch->master_tune_cc][256 + part.lfo_key[lfo]];
		}

		/* intentional fall-through */
	    case FREQ_BASE_TEMPO_KEYTRIG:
	    case FREQ_BASE_TEMPO:
		/* calculate lfo index adjustment based on current freq and pitch bend */
		part.lfo_adjust[lfo] = part.lfo_freq[lfo]
		    * halfsteps_to_freq_mult (patch->lfo_transpose[lfo] + part.lfo_pitch_bend[lfo])
		    * wave_period;

		/* grab LFO output from wave table */
		part.lfo_out[lfo]   = wave_table[patch->lfo_wave[lfo]][(int)part.lfo_index[lfo]];
		part.lfo_index[lfo] += part.lfo_adjust[lfo];
		while (part.lfo_index[lfo] < 0.0) {
		    part.lfo_index[lfo] += F_WAVEFORM_SIZE;
		}
		while (part.lfo_index[lfo] >= F_WAVEFORM_SIZE) {
		    part.lfo_index[lfo] -= F_WAVEFORM_SIZE;
		}
		break;

	    case FREQ_BASE_AMP_ENVELOPE:
		part.lfo_out[lfo] = (2.0 * env_curve[(int)(amp_env_max * F_ENV_CURVE_SIZE)]) - 1.0;
		break;

	    case FREQ_BASE_FILTER_ENVELOPE:
		part.lfo_out[lfo] = (2.0 * env_curve[(int)(filter_env_max * F_ENV_CURVE_SIZE)]) - 1.0;
		break;

	    case FREQ_BASE_INPUT_1:
		part.lfo_out[lfo] = global.in1;
		break;

	    case FREQ_BASE_INPUT_2:
		part.lfo_out[lfo] = global.in2;
		break;

	    case FREQ_BASE_INPUT_STEREO:
		part.lfo_out[lfo] = (global.in1 + global.in2) * 0.5;
		break;

	    case FREQ_BASE_VELOCITY:
		part.lfo_out[lfo] = part.velocity_coef;
		break;
	    }

	    /* resacle for unipolar lfo, if necessary */
	    if (patch->lfo_polarity_cc[lfo] == POLARITY_UNIPOLAR) {
		part.lfo_out[lfo] += 1.0;
		part.lfo_out[lfo] *= 0.5;
	    }
	}

	/* parts get mixed at end of voice loop, so init now */
	part.out1 = part.out2 = 0.0;

	/* update number of samples left in portamento */
	if (part.portamento_sample > 0) {
	    part.portamento_sample--;
	}

	/* per-part-per-osc calculations */
	for (osc = 0; osc < NUM_OSCS; osc++) {

	    /* handle wave selector lfo */
	    part.osc_wave[osc] =
		(patch->osc_wave[osc]
		 + (short)(part.lfo_out[patch->wave_lfo[osc]] * patch->wave_lfo_amount[osc])
		 + (NUM_WAVEFORMS << 4)) % NUM_WAVEFORMS;
	}

	/* filter cutoff and channel aftertouch smoothing */
	patch->filter_cutoff =
	    ((part.filter_smooth_len * patch->filter_cutoff)
	     + part.filter_cutoff_target) * part.filter_smooth_factor;
	part.velocity_coef =
	    ((aftertouch_smooth_len * part.velocity_coef)
	     + part.velocity_target) * aftertouch_smooth_factor;

	/* cycle through voices in play */
	for (v = 0; v < setting_polyphony; v++) {

	    /* skip over inactive voices */
	    if (voice[v].active == 0) {
		continue;
	    }

	    /* set voice output to zero.  oscs will be mixed in */
	    voice[v].out1 = voice[v].out2 = 0.0;

	    /* velocity smoothing (needed for smooth aftertouch) */
	    voice[v].velocity_coef_linear =
		((aftertouch_smooth_len * voice[v].velocity_coef_linear)
		 + voice[v].velocity_target_linear) * aftertouch_smooth_factor;
	    voice[v].velocity_coef_log =
		((aftertouch_smooth_len * voice[v].velocity_coef_log)
		 + voice[v].velocity_target_log) * aftertouch_smooth_factor;

	    /* cycle through all of the oscillators */
	    for (osc = 0; osc < NUM_OSCS; osc++) {

		/* skip over inactive oscillators */
		if (patch->osc_modulation[osc] == MOD_TYPE_OFF) {
		    continue;
		}

		/* current pitch bend for this osc */
		part.osc_pitch_bend[osc] = part.pitch_bend_base * patch->osc_pitchbend[osc];

		/* this is the output of this oscillator with optional polarity value scaling */
		switch (patch->osc_freq_base[osc]) {

		case FREQ_BASE_MIDI_KEY:
		    /* handle portamento if necessary */
		    if (voice[v].portamento_sample > 0) {
			voice[v].osc_freq[osc] += voice[v].osc_portamento[osc];
			voice[v].portamento_sample--;
		    }
		    /* otherwise set frequency directly */
		    else {
			voice[v].osc_freq[osc] = freq_table[patch->master_tune_cc][256 + voice[v].osc_key[osc] + patch->transpose];
		    }

		    /* intentional fall-through */
		case FREQ_BASE_TEMPO:
		case FREQ_BASE_TEMPO_KEYTRIG:
		    /* get frequency modulator */
		    switch (patch->freq_mod_type[osc]) {
		    case MOD_TYPE_LFO:
			tmp_1 = part.lfo_out[patch->freq_lfo[osc]];
			break;
		    case MOD_TYPE_OSC:
			tmp_1 = ( voice[v].osc_out1[part.osc_freq_mod[osc]] + voice[v].osc_out2[part.osc_freq_mod[osc]] ) * 0.5;
			break;
		    case MOD_TYPE_VELOCITY:
			tmp_1 = voice[v].velocity_coef_linear;
			break;
		    default:
			tmp_1 = 0.0;
			break;
		    }

		    /* calculate current note frequency with current freq lfo, */
		    /* pitch bender, etc, and come up with adjustment to current index */
		    freq_adjust = halfsteps_to_freq_mult ( ( tmp_1
							     * patch->freq_lfo_amount[osc] )
							   + part.osc_pitch_bend[osc]
							   + patch->osc_transpose[osc] )
			* voice[v].osc_freq[osc] * wave_period;

		    /* shift the wavetable index by amounts determined above */
		    voice[v].index[osc] += freq_adjust;
		    while (voice[v].index[osc] < 0.0) {
			voice[v].index[osc] += F_WAVEFORM_SIZE;
		    }
		    while (voice[v].index[osc] >= F_WAVEFORM_SIZE) {
			voice[v].index[osc] -= F_WAVEFORM_SIZE;
		    }

		    /* calculate current phase shift */

		    /* get data from modulation source */
		    switch (patch->phase_mod_type[osc]) {
		    case MOD_TYPE_LFO:
			tmp_1 = tmp_2 = part.lfo_out[patch->phase_lfo[osc]];
			break;
		    case MOD_TYPE_OSC:
			tmp_1 = voice[v].osc_out1[part.osc_phase_mod[osc]];
			tmp_2 = voice[v].osc_out2[part.osc_phase_mod[osc]];
			break;
		    case MOD_TYPE_VELOCITY:
			tmp_1 = tmp_2 = voice[v].velocity_coef_linear;
			break;
		    default:
			tmp_1 = tmp_2 = 0.0;
			break;
		    }

		    /* calculate phase adjustment */
		    phase_adjust1 = tmp_1
			* patch->phase_lfo_amount[osc] * F_WAVEFORM_SIZE;
		    phase_adjust2 = tmp_2
			* patch->phase_lfo_amount[osc] * F_WAVEFORM_SIZE;

		    /* grab osc output from wave table, applying phase adjustments to right and left */
		    voice[v].osc_out1[osc] =
			(wave_table[part.osc_wave[osc]][(((int)(voice[v].index[osc] - phase_adjust1)
							  + WAVEFORM_SIZE) % WAVEFORM_SIZE)]);
		    voice[v].osc_out2[osc] =
			(wave_table[part.osc_wave[osc]][(((int)(voice[v].index[osc] + phase_adjust2)
							  + WAVEFORM_SIZE) % WAVEFORM_SIZE)]);
		    break;

		case FREQ_BASE_INPUT_1:
		    voice[v].osc_out1[osc] = global.in1;
		    voice[v].osc_out2[osc] = global.in1;
		    break;

		case FREQ_BASE_INPUT_2:
		    voice[v].osc_out1[osc] = global.in2;
		    voice[v].osc_out2[osc] = global.in2;
		    break;

		case FREQ_BASE_INPUT_STEREO:
		    voice[v].osc_out1[osc] = global.in1;
		    voice[v].osc_out2[osc] = global.in2;
		    break;

		case FREQ_BASE_AMP_ENVELOPE:
		    tmp_1 = (2.0 * env_curve[(int)(voice[v].amp_env_raw * F_ENV_CURVE_SIZE)]) - 1.0;
		    voice[v].osc_out1[osc] = tmp_1;
		    voice[v].osc_out2[osc] = tmp_1;
		    break;

		case FREQ_BASE_FILTER_ENVELOPE:
		    tmp_1 = (2.0 * env_curve[(int)(voice[v].filter_env_raw * F_ENV_CURVE_SIZE)]) - 1.0;
		    voice[v].osc_out1[osc] = tmp_1;
		    voice[v].osc_out2[osc] = tmp_1;
		    break;

		case FREQ_BASE_VELOCITY:
		    voice[v].osc_out1[osc] = voice[v].velocity_coef_linear;
		    voice[v].osc_out2[osc] = voice[v].velocity_coef_linear;
		    break;
		}

		/* rescale if osc is unipolar */
		if (patch->osc_polarity_cc[osc] == POLARITY_UNIPOLAR) {
		    voice[v].osc_out1[osc] += 1.0;
		    voice[v].osc_out1[osc] *= 0.5;
		    voice[v].osc_out2[osc] += 1.0;
		    voice[v].osc_out2[osc] *= 0.5;
		}

		/* last modulation to apply is AM */
		switch (patch->am_mod_type[osc]) {
		case MOD_TYPE_OSC:
		    if (patch->am_lfo_amount[osc] > 0.0) {
			voice[v].osc_out1[osc] *= ((voice[v].osc_out1[part.osc_am_mod[osc]] * patch->am_lfo_amount[osc]) + 1.0) * 0.5;
			voice[v].osc_out2[osc] *= ((voice[v].osc_out2[part.osc_am_mod[osc]] * patch->am_lfo_amount[osc]) + 1.0) * 0.5;
		    }
		    else if (patch->am_lfo_amount[osc] < 0.0) {
			voice[v].osc_out1[osc] *= ((voice[v].osc_out1[part.osc_am_mod[osc]] * patch->am_lfo_amount[osc]) - 1.0) * 0.5;
			voice[v].osc_out2[osc] *= ((voice[v].osc_out2[part.osc_am_mod[osc]] * patch->am_lfo_amount[osc]) - 1.0) * 0.5;
		    }
		    break;
		case MOD_TYPE_LFO:
		    if (patch->am_lfo_amount[osc] > 0.0) {
			tmp_1 = ((part.lfo_out[patch->am_lfo[osc]] * patch->am_lfo_amount[osc]) + 1.0) * 0.5;
			voice[v].osc_out1[osc] *= tmp_1;
			voice[v].osc_out2[osc] *= tmp_1;
		    }
		    else if (patch->am_lfo_amount[osc] < 0.0) {
			tmp_1 = ((part.lfo_out[patch->am_lfo[osc]] * patch->am_lfo_amount[osc]) - 1.0) * 0.5;
			voice[v].osc_out1[osc] *= tmp_1;
			voice[v].osc_out2[osc] *= tmp_1;
		    }
		    break;
		case MOD_TYPE_VELOCITY:
		    tmp_1 = 1.0 - ( ( 1.0 - voice[v].velocity_coef_linear ) * patch->am_lfo_amount[osc]);
		    voice[v].osc_out1[osc] *= tmp_1;
		    voice[v].osc_out2[osc] *= tmp_1;
		    break;
		}

		/* add oscillator to voice mix, if necessary */
		if (patch->osc_modulation[osc] == MOD_TYPE_MIX) {
		    voice[v].out1 += voice[v].osc_out1[osc];
		    voice[v].out2 += voice[v].osc_out2[osc];
		}

	    }

	    /* oscs are mixed.  now apply AM oscs. */ 
	    for (osc = 0; osc < NUM_OSCS; osc++) {
		if (patch->osc_modulation[osc] == MOD_TYPE_AM) {
		    voice[v].out1 *= voice[v].osc_out1[osc];
		    voice[v].out2 *= voice[v].osc_out2[osc];
		}
	    }

	    /* apply keyfollow volume and filter gain at filter input */
	    tmp_1 = ( patch->filter_gain * keyfollow_table[patch->keyfollow_vol][voice[v].vol_key] ) * 0.25;
	    voice[v].out1 *= tmp_1;
	    voice[v].out2 *= tmp_1;

	    /* Experimental modified Chamberlin filter */
	    /* uses a large lookup table for cutoff coeff */
	    /* oversampling for stabliity and harmonic characteristics */

	    tmp_2 = (patch->filter_lfo == LFO_VELOCITY) ? voice[v].velocity_coef_linear : part.lfo_out[patch->filter_lfo];
	    filter_q = 1.0 - (patch->filter_resonance + (tmp_2 * patch->filter_lfo_resonance));
	    if (filter_q < 0.00390625) {  // (1/256... was 1/128)
		filter_q = 0.00390625;
	    }
	    else if (filter_q > 1.0) {
		filter_q = 1.0;
	    }

	    /* set the freq coef table index based on cutoff, lfo, envelope, and keyfollow */
	    filter_index = ((int)( ( ( tmp_2 * patch->filter_lfo_cutoff )
				     + ( patch->filter_env_amount * voice[v].filter_env_raw )
				     + patch->filter_cutoff + voice[v].filter_key_adj + 256.0 ) * F_TUNING_RESOLUTION ))
		+ patch->master_tune;

	    /* now look up the f coefficient from the table */
	    /* use hard clipping (top midi note + 2 octaves) for filter cutoff */
	    if (filter_index < 0) {
		filter_f = filter_table[0];
		if (debug) {
		    fprintf (stderr, "Filter Index = %d\n", filter_index);
		}
	    }
	    else if (filter_index > filter_limit) {
		filter_f = filter_table[filter_limit];
	    }
	    else {
		filter_f = filter_table[filter_index];
	    }

	    /* Two variations of the Chamberlin filter */
	    switch (patch->filter_type) {

	    case FILTER_TYPE_DIST:
		/* "Dist" - LP distortion */
		for (j = 0; j < FILTER_OVERSAMPLE; j++) {
		    /* highpass */
		    voice[v].filter_hp1 = voice[v].out1 - voice[v].filter_lp1 - (voice[v].filter_bp1 * filter_q);
		    voice[v].filter_hp2 = voice[v].out2 - voice[v].filter_lp2 - (voice[v].filter_bp2 * filter_q);

		    /* bandpass */
		    voice[v].filter_bp1 += filter_f * voice[v].filter_hp1;
		    voice[v].filter_bp2 += filter_f * voice[v].filter_hp2;

		    /* lowpass */
		    voice[v].filter_lp1 += filter_f * voice[v].filter_bp1;
		    voice[v].filter_lp2 += filter_f * voice[v].filter_bp2;

		    /* lowpass distortion */
		    tmp_1 = (filter_dist_1[j] - filter_f) * filter_dist_2[j];
		    voice[v].filter_lp1 *= tmp_1;
		    voice[v].filter_lp2 *= tmp_1;
		}
		break;

	    case FILTER_TYPE_RETRO:
		/* "Retro" - No distortion */
		for (j = 0; j < FILTER_OVERSAMPLE; j++) {
		    /* highpass */
		    voice[v].filter_hp1 = voice[v].out1 - voice[v].filter_lp1 - (voice[v].filter_bp1 * filter_q);
		    voice[v].filter_hp2 = voice[v].out2 - voice[v].filter_lp2 - (voice[v].filter_bp2 * filter_q);

		    /* bandpass */
		    voice[v].filter_bp1 += filter_f * voice[v].filter_hp1;
		    voice[v].filter_bp2 += filter_f * voice[v].filter_hp2;

		    /* lowpass */
		    voice[v].filter_lp1 += filter_f * voice[v].filter_bp1;
		    voice[v].filter_lp2 += filter_f * voice[v].filter_bp2;
		}
		break;
	    }

	    /* select the filter output we want */
	    switch (patch->filter_mode) {
	    case FILTER_MODE_LP:
		voice[v].out1 = voice[v].filter_lp1;
		voice[v].out2 = voice[v].filter_lp2;
		break;
	    case FILTER_MODE_HP:
		voice[v].out1 = voice[v].filter_hp1;
		voice[v].out2 = voice[v].filter_hp2;
		break;
	    case FILTER_MODE_BP:
		voice[v].out1 = voice[v].filter_bp1;
		voice[v].out2 = voice[v].filter_bp2;
		break;
	    case FILTER_MODE_BS:
		voice[v].out1 = (voice[v].filter_lp1 - voice[v].filter_bp1 + voice[v].filter_hp1);
		voice[v].out2 = (voice[v].filter_lp2 - voice[v].filter_bp2 + voice[v].filter_hp2);
		break;
	    case FILTER_MODE_LP_PLUS:
		voice[v].out1 = (voice[v].filter_lp1 + voice[v].filter_bp1);
		voice[v].out2 = (voice[v].filter_lp2 + voice[v].filter_bp2);
		break;
	    case FILTER_MODE_HP_PLUS:
		voice[v].out1 = (voice[v].filter_bp1 + voice[v].filter_hp1);
		voice[v].out2 = (voice[v].filter_bp2 + voice[v].filter_hp2);
		break;
	    case FILTER_MODE_BP_PLUS:
		voice[v].out1 = (voice[v].filter_lp1 - voice[v].filter_hp1);
		voice[v].out2 = (voice[v].filter_lp2 - voice[v].filter_hp2);
		break;
	    case FILTER_MODE_BS_PLUS:
		voice[v].out1 = (voice[v].filter_lp1 + voice[v].filter_bp1 + voice[v].filter_hp1);
		voice[v].out2 = (voice[v].filter_lp2 + voice[v].filter_bp2 + voice[v].filter_hp2);
		break;
	    }

	    /* Apply the amp velocity and amp envelope for this voice */
	    tmp_1 = voice[v].velocity_coef_log * env_curve[(int)(voice[v].amp_env_raw * F_ENV_CURVE_SIZE)];
	    voice[v].out1 *= tmp_1;
	    voice[v].out2 *= tmp_1;

	    /* end of per voice parameters.  mix voices */
	    part.out1 += ((voice[v].out1 * patch->stereo_width) + (voice[v].out2 * (1.0 - patch->stereo_width)));
	    part.out2 += ((voice[v].out2 * patch->stereo_width) + (voice[v].out1 * (1.0 - patch->stereo_width)));

	    /* keep track of voice's age for note stealing */
	    voice[v].age++;
	}

	/* apply input envelope, if needed */
	if (patch->input_follow) {
	    part.out1 *= input_env_raw;
	    part.out2 *= input_env_raw;
	}

	/* now apply patch volume and panning */
	part.out1 *= patch->volume * pan_table[127 - patch->pan_cc];
	part.out2 *= patch->volume * pan_table[patch->pan_cc];

	/* phasing stereo crossover chorus effect */
	/* clock-sync still needs to be implemented in midi.c */
	if (patch->chorus_mix_cc > 0) {

#ifdef INTERPOLATE_CHORUS
	    /* with interpolation, chorus buffer must be two separate mono buffers */

	    /* set phase offset read indices into chorus delay buffer */
	    part.chorus_read_index_a =
		((sample_t)(part.chorus_bufsize + part.chorus_write_index - part.chorus_len - 1) 
		 + ((wave_table[patch->chorus_lfo_wave][(int)part.chorus_lfo_index_a] + 1.0)
		    * part.chorus_size * patch->chorus_amount * 0.5));

	    part.chorus_read_index_b =
		((sample_t)(part.chorus_bufsize + part.chorus_write_index - part.chorus_len - 1) 
		 + ((wave_table[patch->chorus_lfo_wave][(int)part.chorus_lfo_index_b] + 1.0)
		    * part.chorus_size * patch->chorus_amount * 0.5));

	    part.chorus_read_index_c =
		((sample_t)(part.chorus_bufsize + part.chorus_write_index - part.chorus_len - 1) 
		 + ((wave_table[patch->chorus_lfo_wave][(int)part.chorus_lfo_index_c] + 1.0)
		    * part.chorus_size * patch->chorus_amount * 0.5));

	    part.chorus_read_index_d =
		((sample_t)(part.chorus_bufsize + part.chorus_write_index - part.chorus_len - 1) 
		 + ((wave_table[patch->chorus_lfo_wave][(int)part.chorus_lfo_index_d] + 1.0)
		    * part.chorus_size * patch->chorus_amount * 0.5));

	    /* grab values from phase offset positions within chorus delay buffer */
	    tmp_1_a = hermite (part.chorus_buf_1, part.chorus_bufsize, part.chorus_read_index_a);
	    tmp_1_a = hermite (part.chorus_buf_2, part.chorus_bufsize, part.chorus_read_index_a);

	    tmp_1_b = hermite (part.chorus_buf_1, part.chorus_bufsize, part.chorus_read_index_b);
	    tmp_1_b = hermite (part.chorus_buf_2, part.chorus_bufsize, part.chorus_read_index_b);

	    tmp_1_c = hermite (part.chorus_buf_1, part.chorus_bufsize, part.chorus_read_index_c);
	    tmp_1_c = hermite (part.chorus_buf_2, part.chorus_bufsize, part.chorus_read_index_c);

	    tmp_1_d = hermite (part.chorus_buf_1, part.chorus_bufsize, part.chorus_read_index_d);
	    tmp_1_d = hermite (part.chorus_buf_2, part.chorus_bufsize, part.chorus_read_index_d);
#else
	    /* chorus_buf MUST be a single stereo width buffer, not separate buffers! */
	    /* set phase offset read indices into chorus delay buffer */
	    part.chorus_read_index_a =
		(part.chorus_bufsize + part.chorus_write_index 
		 + (int)(((wave_table[patch->chorus_lfo_wave][(int)part.chorus_lfo_index_a] + 1.0)
			  * part.chorus_size * patch->chorus_amount * 0.5))
		 - part.chorus_len - 1) % part.chorus_bufsize;

	    part.chorus_read_index_b =
		(part.chorus_bufsize + part.chorus_write_index
		 + (int)(((wave_table[patch->chorus_lfo_wave][(int)(part.chorus_lfo_index_b)] + 1.0)
			  * part.chorus_size * patch->chorus_amount * 0.5))
		 - part.chorus_len - 1) % part.chorus_bufsize;

	    part.chorus_read_index_c =
		(part.chorus_bufsize + part.chorus_write_index
		 + (int)(((wave_table[patch->chorus_lfo_wave][(int)(part.chorus_lfo_index_c)] * 1.0)
			  * part.chorus_size * patch->chorus_amount * 0.5))
		 - part.chorus_len - 1) % part.chorus_bufsize;

	    part.chorus_read_index_d =
		(part.chorus_bufsize + part.chorus_write_index
		 + (int)(((wave_table[patch->chorus_lfo_wave][(int)(part.chorus_lfo_index_d)] * 1.0)
			  * part.chorus_size * patch->chorus_amount * 0.5))
		 - part.chorus_len - 1) % part.chorus_bufsize;

	    /* grab values from phase offset positions within chorus delay buffer */
	    tmp_1_a = part.chorus_buf[2 * part.chorus_read_index_a];
	    tmp_2_a = part.chorus_buf[2 * part.chorus_read_index_a + 1];

	    tmp_1_b = part.chorus_buf[2 * part.chorus_read_index_b];
	    tmp_2_b = part.chorus_buf[2 * part.chorus_read_index_b + 1];

	    tmp_1_c = part.chorus_buf[2 * part.chorus_read_index_c];
	    tmp_2_c = part.chorus_buf[2 * part.chorus_read_index_c + 1];

	    tmp_1_d = part.chorus_buf[2 * part.chorus_read_index_d];
	    tmp_2_d = part.chorus_buf[2 * part.chorus_read_index_d + 1];
#endif

	    /* add them together, with channel crossing */
	    tmp_1 = ( (tmp_1_a * part.chorus_phase_amount_a) + (tmp_2_b * part.chorus_phase_amount_b) +
		      (tmp_1_c * part.chorus_phase_amount_c) + (tmp_2_d * part.chorus_phase_amount_d) );
	    tmp_2 = ( (tmp_2_a * part.chorus_phase_amount_a) + (tmp_1_b * part.chorus_phase_amount_b) +
		      (tmp_2_c * part.chorus_phase_amount_c) + (tmp_1_d * part.chorus_phase_amount_d) );

	    /* keep dry signal around for chorus delay buffer mixing */
	    tmp_3 = part.out1;
	    tmp_4 = part.out2;

	    /* combine dry/wet for final output */
	    part.out1 = (tmp_3 * (mix_table[127 - patch->chorus_mix_cc])) + (tmp_1 * mix_table[patch->chorus_mix_cc]);
	    part.out2 = (tmp_4 * (mix_table[127 - patch->chorus_mix_cc])) + (tmp_2 * mix_table[patch->chorus_mix_cc]);

#ifdef INTERPOLATE_CHORUS
	    /* write to chorus delay buffer with feedback */
	    tmp_1 = ((part.chorus_buf_1[part.chorus_delay_index] * mix_table[patch->chorus_feed_cc])
		     + (tmp_3 * mix_table[127 - patch->chorus_feed_cc])) - denormal_offset;

	    tmp_2 = ((part.chorus_buf_2[part.chorus_delay_index] * mix_table[patch->chorus_feed_cc])
		     + (tmp_4 * mix_table[127 - patch->chorus_feed_cc])) - denormal_offset;

	    if (patch->chorus_flip) {
		part.chorus_buf_1[part.chorus_write_index] = tmp_2;
		part.chorus_buf_2[part.chorus_write_index] = tmp_1;
	    }
	    else {
		part.chorus_buf_1[part.chorus_write_index] = tmp_1;
		part.chorus_buf_2[part.chorus_write_index] = tmp_2;
	    }
#else
	    /* write to chorus delay buffer with feedback */
	    part.chorus_buf[2 * part.chorus_write_index + patch->chorus_flip] =
	    	((part.chorus_buf[2 * part.chorus_delay_index]     * mix_table[patch->chorus_feed_cc])
	    	 + (tmp_3 * mix_table[127 - patch->chorus_feed_cc])) - denormal_offset;

	    part.chorus_buf[2 * part.chorus_write_index + (1 - patch->chorus_flip)] =
	    	((part.chorus_buf[2 * part.chorus_delay_index + 1] * mix_table[patch->chorus_feed_cc])
	    	 + (tmp_4 * mix_table[127 - patch->chorus_feed_cc])) - denormal_offset;
#endif

	    /* set phase lfo indices */
	    part.chorus_phase_index_a += part.chorus_phase_adjust;
	    if (part.chorus_phase_index_a >= F_WAVEFORM_SIZE) {
		part.chorus_phase_index_a -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_phase_index_b = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.25);
	    if (part.chorus_phase_index_b >= F_WAVEFORM_SIZE) {
		part.chorus_phase_index_b -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_phase_index_c = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.5);
	    if (part.chorus_phase_index_c >= F_WAVEFORM_SIZE) {
		part.chorus_phase_index_c -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_phase_index_d = part.chorus_phase_index_a + (F_WAVEFORM_SIZE * 0.75);
	    if (part.chorus_phase_index_d >= F_WAVEFORM_SIZE) {
		part.chorus_phase_index_d -= F_WAVEFORM_SIZE;
	    }

	    /* set amount used for mix weight for the LFO positions at right angles */
	    part.chorus_phase_amount_a = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_a])
		* 0.5 * (mix_table[127 - patch->chorus_phase_balance_cc]);

	    part.chorus_phase_amount_b = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_b])
		* 0.5 * (mix_table[patch->chorus_phase_balance_cc]);

	    part.chorus_phase_amount_c = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_c])
		* 0.5 * (mix_table[127 - patch->chorus_phase_balance_cc]);

	    part.chorus_phase_amount_d = (1.0 + wave_table[WAVE_SINE][(int)part.chorus_phase_index_d])
		* 0.5 * (mix_table[patch->chorus_phase_balance_cc]);

	    /* set lfo indices */
	    part.chorus_lfo_index_a += part.chorus_lfo_adjust;
	    if (part.chorus_lfo_index_a >= F_WAVEFORM_SIZE) {
		part.chorus_lfo_index_a -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_lfo_index_b = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.25);
	    if (part.chorus_lfo_index_b >= F_WAVEFORM_SIZE) {
		part.chorus_lfo_index_b -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_lfo_index_c = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.5);
	    if (part.chorus_lfo_index_c >= F_WAVEFORM_SIZE) {
		part.chorus_lfo_index_c -= F_WAVEFORM_SIZE;
	    }

	    part.chorus_lfo_index_d = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.75);
	    if (part.chorus_lfo_index_d >= F_WAVEFORM_SIZE) {
		part.chorus_lfo_index_d -= F_WAVEFORM_SIZE;
	    }

	    /* increment chorus write index */
	    part.chorus_write_index++;
	    part.chorus_write_index %= part.chorus_bufsize;

	    /* increment delayed position into chorus buffer */
	    part.chorus_delay_index++;
	    part.chorus_delay_index %= part.chorus_bufsize;
	}

	/* delay effect (also works great for flange or simple chorus) */
	/* this algo really needs a more rigorous clock-sync */
	/* delay_buf MUST be a single stereo width buffer, not separate buffers! */
	if (patch->delay_mix_cc > 0) {

	    /* set read position into delay buffer */
	    if (patch->delay_lfo == LFO_OFF) {
		part.delay_read_index =
		    (part.delay_bufsize + part.delay_write_index
		     - part.delay_length - 1) % part.delay_bufsize;
	    }

	    /* set read position into delay buffer based on delay lfo */
	    else {
		part.delay_read_index =
		    (part.delay_bufsize + part.delay_write_index
		     - (int)(((part.lfo_out[patch->delay_lfo] + 1.0) * part.delay_size * 0.5)) - 1) % part.delay_bufsize;
	    }

	    /* read delayed signal from delay buffer */
	    tmp_1 = part.delay_buf[2 * part.delay_read_index];
	    tmp_2 = part.delay_buf[2 * part.delay_read_index + 1];

	    /* keep original input signal around for buffer writing */
	    tmp_3 = part.out1;
	    tmp_4 = part.out2;

	    /* mix delayed signal with input */
	    part.out1 = (part.out1 * (mix_table[127 - patch->delay_mix_cc])) + (tmp_1 * mix_table[patch->delay_mix_cc]);
	    part.out2 = (part.out2 * (mix_table[127 - patch->delay_mix_cc])) + (tmp_2 * mix_table[patch->delay_mix_cc]);

	    /* write input to delay buffer with feedback */
	    part.delay_buf[2 * part.delay_write_index + patch->delay_flip] =
	    	(tmp_1 * (mix_table[patch->delay_feed_cc])) + (tmp_3 * (mix_table[127 - patch->delay_feed_cc])) - denormal_offset;
	    part.delay_buf[2 * part.delay_write_index + (1 - patch->delay_flip)] =
	    	(tmp_2 * (mix_table[patch->delay_feed_cc])) + (tmp_4 * (mix_table[127 - patch->delay_feed_cc])) - denormal_offset;

	    /* increment delay write index */
	    part.delay_write_index++;
	    part.delay_write_index %= part.delay_bufsize;
	}

	/* part mixing should go here when multitimbral mode is supported */
	global.out1 = part.out1;
	global.out2 = part.out2;

	/* debug: keep track of max output sample */
#ifdef DEBUG_SAMPLE_VALUES
	if (debug) {
	    if (fabs (global.out1) > fabs (global.max1)) {
		global.max1 = global.out1;
	    }	    else {
		odd_sample = 1;

	    if (fabs (global.out2) > fabs (global.max2)) {
		global.max2 = global.out2;
	    }

	    if ((debug_sample_counter % 384000) == 0) {
	    	printf ("Current Max samples: (L) %05.13f     (R) %05.13f\n", global.max1, global.max2);
	    	debug_sample_counter = 0;
	    }
	    debug_sample_counter++;
	}
#endif

#ifdef MIDI_CLOCK_SYNC
	/* update number of samples in this clock tick */
	midi->tick_sample++;

	/* just in case we lose the clock */
	midi->tick_sample %= 262144;
#endif

	/* flip sign of denormal offset */
	denormal_offset *= -1.0;

	/* for oversampling, use mean of two samples */
	if ((sample_rate_mode == SAMPLE_RATE_OVERSAMPLE)) {
	    if (odd_sample) {
		last_out1 = global.out1;
		last_out2 = global.out2;
		odd_sample = 0;
		continue;
	    }

	    odd_sample = 1;

	    global.out1	+= last_out1;
	    global.out1	*= 0.5;

	    global.out2	+= last_out2;
	    global.out2	*= 0.5;
	}

	/* set thread cancellation point out outside critical section */
	pthread_testcancel ();

	/* critical section -- lock buffer mutex */
	pthread_mutex_lock (&buffer_mutex);

	/* if buffer is completely full, wait for empty space */
	if (buffer_full == buffer_size) {
	    /* this unlocks the mutex during the wait */
	    while (pthread_cond_wait (&buffer_has_space, &buffer_mutex) != 0) {
		if (debug) {
		    fprintf (stderr, "pthread_cond_wait() returned with error!\n");
		}
		usleep (20000);
	    }
	}

	/* get current input sample from buffer */
	global.in1 = (sample_t)input_buffer1[buffer_write_index] * patch->input_boost;
	global.in2 = (sample_t)input_buffer2[buffer_write_index] * patch->input_boost;

	/* for undersampling, use linear interpolation on input and output */
	if (sample_rate_mode == SAMPLE_RATE_UNDERSAMPLE) {
	    output_buffer1[buffer_write_index] = (jack_default_audio_sample_t)((global.out1 + last_out1) * 0.5);
	    output_buffer2[buffer_write_index] = (jack_default_audio_sample_t)((global.out2 + last_out2) * 0.5);

	    buffer_write_index++;
	    buffer_full++;

	    last_out1 = global.out1;
	    last_out2 = global.out2;

	    global.in1 += ((sample_t)input_buffer1[buffer_write_index] * patch->input_boost);
	    global.in1 *= 0.5;

	    global.in2 += ((sample_t)input_buffer2[buffer_write_index] * patch->input_boost);
	    global.in2 *= 0.5;
	}

	/* output this sample to the buffer */
	output_buffer1[buffer_write_index] = (jack_default_audio_sample_t)global.out1;
	output_buffer2[buffer_write_index] = (jack_default_audio_sample_t)global.out2;

	/* update buffer position */
	buffer_write_index++;
	buffer_write_index %= buffer_size;

	/* update buffer full counter */
	buffer_full++;

	/* end critical section -- unlock buffer mutex */
	pthread_mutex_unlock (&buffer_mutex);

	/* back to the top of the main while(1) loop */
    }

    /* end of engine thread */
    return NULL;
}

void
engine_panic(void)
{
    int j;
    for (j = 0; j < setting_polyphony; j++)
    {
        voice[j].allocated = 0;
        voice[j].keypressed = -1;
		part.prev = NULL;
		part.cur = part.head;
		while ((part.cur != NULL)) {
            /* if note is found, unlink it from the list */
			if (part.cur->midi_key == voice[j].midi_key) {
	        if (part.prev != NULL) {
				part.prev->next = part.cur->next;
				part.cur->next  = NULL;
				part.cur        = part.prev->next;
			}
			else {
				part.head       = part.cur->next;
				part.cur->next  = NULL;
				part.cur        = part.head;
			}
			}
			/* otherwise, on to the next key in the list */
			else {
		        part.prev = part.cur;
				part.cur  = part.cur->next;
			}
	    }
	    voice[j].midi_key   = -1;
        voice[j].age = 0;
        voice[j].amp_env_raw = 0.0;
		voice[j].cur_amp_sample = -2; 
		voice[j].cur_amp_interval = ENV_INTERVAL_DONE;
		voice[j].filter_env_raw = 0.0;
		voice[j].cur_filter_sample = -2; 
		voice[j].cur_filter_interval = ENV_INTERVAL_DONE;
    }
    memset (part.delay_buf, 0, part.delay_bufsize * 2 * sizeof (sample_t));
	part.chorus_bufsize = CHORUS_MAX;
    #ifdef INTERPOLATE_CHORUS
	memset (part.chorus_buf_1, 0, part.chorus_bufsize * sizeof (sample_t));
	memset (part.chorus_buf_2, 0, part.chorus_bufsize * sizeof (sample_t));
    #else
	memset (part.chorus_buf, 0, part.chorus_bufsize * 2 * sizeof (sample_t));
    #endif
	part.midi_key = -1;
	part.head = NULL;
    hold_pedal = 0;
}
