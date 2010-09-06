/*****************************************************************************
 *
 * jack.c
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
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jack/jack.h>
#include "phasex.h"
#include "jack.h"
#include "engine.h"
#include "settings.h"
#include "gtkui.h"
#include "lash.h"

jack_client_t			*client;

jack_port_t			*input_port1;
jack_port_t			*input_port2;

jack_port_t			*output_port1;
jack_port_t			*output_port2;

pthread_mutex_t			buffer_mutex;
pthread_mutex_t			sample_rate_mutex;

pthread_cond_t			buffer_has_data		= PTHREAD_COND_INITIALIZER;
pthread_cond_t			buffer_has_space	= PTHREAD_COND_INITIALIZER;
pthread_cond_t			sample_rate_cond	= PTHREAD_COND_INITIALIZER;
pthread_cond_t			jack_client_cond	= PTHREAD_COND_INITIALIZER;

jack_default_audio_sample_t	input_buffer1[PHASEX_MAX_BUFSIZE];
jack_default_audio_sample_t	input_buffer2[PHASEX_MAX_BUFSIZE];

jack_default_audio_sample_t	output_buffer1[PHASEX_MAX_BUFSIZE];
jack_default_audio_sample_t	output_buffer2[PHASEX_MAX_BUFSIZE];

int				jack_running		= 0;

int				buffer_full		= 0;
int				buffer_size		= PHASEX_MAX_BUFSIZE;
int				buffer_read_index	= 0;
int				buffer_write_index	= 0;

int				sample_rate		= 0;
int				sample_rate_mode	= SAMPLE_RATE_NORMAL;
sample_t			f_sample_rate		= 0.0;
sample_t			nyquist_freq		= 22050.0;
sample_t			wave_period		= F_WAVEFORM_SIZE / 44100.0;

char				*jack_inputs		= NULL;
char				*jack_outputs		= NULL;

jack_position_t jack_pos;
jack_transport_state_t  jack_state = -1;
jack_transport_state_t  jack_prev_state = -1;

int             current_frame = 0;
jack_nframes_t  prev_frame = 0;

int             frames_per_tick;
int             frames_per_beat;

int phase_correction;
int need_resync;

sample_t tmp_1;

/*****************************************************************************
 *
 * PROCESS_BUFFER()
 *
 * callback for jack to read audio data from ringbuffer
 * jack uses a realtime priority thread for this
 *
 *****************************************************************************/
int 
process_buffer(jack_nframes_t nframes, void *arg) {
    jack_default_audio_sample_t *in1;
    jack_default_audio_sample_t *in2;
    jack_default_audio_sample_t *out1;
    jack_default_audio_sample_t *out2;
    int				portion1;
    int				portion2;
    int             j, osc, lfo;

    /* Get the input and output buffers for audio data */
    in1  = jack_port_get_buffer (input_port1,  nframes);
    in2  = jack_port_get_buffer (input_port2,  nframes);
    out1 = jack_port_get_buffer (output_port1, nframes);
    out2 = jack_port_get_buffer (output_port2, nframes);

    /* check for jack not running or global shutdown so we don't get hung on mutex */
    if (!jack_running || shutdown) {
	return 0;
    }

    /* copy circular buffers and update index */
    /* WARNING:  needs to handle circular buffer properly */
    if ((buffer_read_index + nframes) > buffer_size) {
	portion1 = buffer_size - buffer_read_index;
	portion2 = nframes - portion1;
	memcpy (&(input_buffer1[buffer_read_index]), in1,   sizeof (jack_default_audio_sample_t) * portion1);
	memcpy (&(input_buffer2[buffer_read_index]), in2,   sizeof (jack_default_audio_sample_t) * portion1);
	memcpy (input_buffer1, &(in1[portion1]), sizeof (jack_default_audio_sample_t) * portion2);
	memcpy (input_buffer2, &(in2[portion1]), sizeof (jack_default_audio_sample_t) * portion2);

	memcpy (out1, &(output_buffer1[buffer_read_index]), sizeof (jack_default_audio_sample_t) * portion1);
	memcpy (out2, &(output_buffer2[buffer_read_index]), sizeof (jack_default_audio_sample_t) * portion1);
	memcpy (&(out1[portion1]), output_buffer1, sizeof (jack_default_audio_sample_t) * portion2);
	memcpy (&(out2[portion1]), output_buffer2, sizeof (jack_default_audio_sample_t) * portion2);
    }
    else {
	memcpy (&(input_buffer1[buffer_read_index]), in1,   sizeof (jack_default_audio_sample_t) * nframes);
	memcpy (&(input_buffer2[buffer_read_index]), in2,   sizeof (jack_default_audio_sample_t) * nframes);
	memcpy (out1, &(output_buffer1[buffer_read_index]), sizeof (jack_default_audio_sample_t) * nframes);
	memcpy (out2, &(output_buffer2[buffer_read_index]), sizeof (jack_default_audio_sample_t) * nframes);
    }
    buffer_read_index += nframes;
    buffer_read_index %= buffer_size;

    /* Update buffer full and buffer needed counters */
    buffer_full   -= nframes;
    
    /* jack transport sync */
    jack_state = jack_transport_query (client, &jack_pos);
    /* reinit sync vars if transport is just started */
    if((jack_prev_state == JackTransportStopped) && (jack_state == JackTransportStarting))
    {
        if (debug)
	    {
	        fprintf (stderr, "Starting sync.\n");
        }
        need_resync = 1;
    }
    if(jack_state == JackTransportRolling)
    {
        if(need_resync)
        {
            prev_frame = jack_pos.frame - nframes;
            frames_per_beat = sample_rate / global.bps;
            frames_per_tick = sample_rate / (jack_pos.ticks_per_beat * global.bps);
            current_frame = 0;
            need_resync = 0;
        }
    
        /* Handle BPM change */
        if(jack_pos.beats_per_minute && (global.bps != jack_pos.beats_per_minute / 60.0))
        {
            global.bps = jack_pos.beats_per_minute / 60.0;
            frames_per_beat = sample_rate / global.bps;
            frames_per_tick = sample_rate / (jack_pos.ticks_per_beat * global.bps);

            /* Update delay params */
            part.delay_size   = patch->delay_time * f_sample_rate / global.bps;
            part.delay_length = (int)(part.delay_size);
            
            /* Update chorus params */
            part.chorus_lfo_freq     = global.bps * patch->chorus_lfo_rate;
            part.chorus_lfo_adjust   = part.chorus_lfo_freq * wave_period;
            part.chorus_phase_freq   = global.bps * patch->chorus_phase_rate;
            part.chorus_phase_adjust = part.chorus_phase_freq * wave_period;
            
            /* Update oscillators */
            for (j = 0; j < setting_polyphony; j++) {
			    if (voice[j].active) {
				    for (osc = 0; osc < NUM_OSCS; osc++) {
				        if (patch->osc_freq_base[osc] >= FREQ_BASE_TEMPO) {
					    voice[j].osc_freq[osc] = patch->osc_rate[osc] * global.bps;
				        }
				    }
			    }
			}
			
			/* Update LFOs */
	        for (lfo = 0; lfo < NUM_LFOS; lfo++) {
			    if (patch->lfo_freq_base[lfo] >= FREQ_BASE_TEMPO) {
				    part.lfo_freq[lfo]   = patch->lfo_rate[lfo] * global.bps;
			    }
		    }
	    }
	    
	    /* frame-based sync */
	    current_frame = (jack_pos.frame - prev_frame);
	    prev_frame = jack_pos.frame;
	    
	    if(current_frame != nframes)
	    {
	        /* whoooaaaa, we're traveling through time! */
	        phase_correction = current_frame - nframes;
	    }
	    
	    /* BBT sync */
	    /*if(prev_beat != jack_pos.beat)
	    {
	        prev_beat = jack_pos.beat;
	        current_beat++;
	        phase_correction = jack_pos.tick * frames_per_tick;
	    }*/
	    
	    /* Plain time sync */
	    //current_frame += (jack_pos.frame - prev_frame);
	    //prev_frame = jack_pos.frame; 
	    /* resync if clock is lost */
	    //if((current_frame >= 2*frames_per_beat) || (current_frame < 0))
	    //    current_frame = 0;

	    //if(current_frame >= frames_per_beat)
	    //{
	    //    current_beat++;
	    //    current_frame = current_frame - frames_per_beat;
	    //    phase_correction = current_frame;
	    //} 
	    
	    /* do the actual sync */
	    if(phase_correction)
	    {
	        if (debug)
	        {
	            fprintf (stderr, "Out of sync. Phase correction:  %d\n", phase_correction);
            }
            
	        part.delay_write_index += phase_correction;
	        while(part.delay_write_index < 0.0)
		        part.delay_write_index += part.delay_bufsize;
	        while(part.delay_write_index >= part.delay_bufsize)
		        part.delay_write_index -= part.delay_bufsize;
	    
	        part.chorus_lfo_index_a += phase_correction * part.chorus_lfo_adjust;
	        while(part.chorus_lfo_index_a < 0.0)
		        part.chorus_lfo_index_a += F_WAVEFORM_SIZE;
	        while(part.chorus_lfo_index_a >= F_WAVEFORM_SIZE)
		        part.chorus_lfo_index_a -= F_WAVEFORM_SIZE;

		    part.chorus_lfo_index_b = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.25);
	        while(part.chorus_lfo_index_b < 0.0)
		        part.chorus_lfo_index_b += F_WAVEFORM_SIZE;
	        while(part.chorus_lfo_index_b >= F_WAVEFORM_SIZE)
		        part.chorus_lfo_index_b -= F_WAVEFORM_SIZE;

		    part.chorus_lfo_index_c = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.5);
	        while(part.chorus_lfo_index_c < 0.0)
		        part.chorus_lfo_index_c += F_WAVEFORM_SIZE;
	        while(part.chorus_lfo_index_c >= F_WAVEFORM_SIZE)
		        part.chorus_lfo_index_c -= F_WAVEFORM_SIZE;

		    part.chorus_lfo_index_d = part.chorus_lfo_index_a + (F_WAVEFORM_SIZE * 0.75);
	        while(part.chorus_lfo_index_d < 0.0)
		        part.chorus_lfo_index_d += F_WAVEFORM_SIZE;
	        while(part.chorus_lfo_index_d >= F_WAVEFORM_SIZE)
		        part.chorus_lfo_index_d -= F_WAVEFORM_SIZE;
	    
	        for (osc = 0; osc < NUM_OSCS; osc++) {
				for (j = 0; j < setting_polyphony; j++) {
				    if (patch->osc_freq_base[osc] >= FREQ_BASE_TEMPO) 
				    {
				        switch (patch->freq_mod_type[osc]) {
		                case MOD_TYPE_LFO:
			            tmp_1 = part.lfo_out[patch->freq_lfo[osc]];
			            break;
		                case MOD_TYPE_OSC:
			            tmp_1 = ( voice[j].osc_out1[part.osc_freq_mod[osc]] + voice[j].osc_out2[part.osc_freq_mod[osc]] ) * 0.5;
			            break;
		                case MOD_TYPE_VELOCITY:
			            tmp_1 = voice[j].velocity_coef_linear;
			            break;
		                default:
			            tmp_1 = 0.0;
			            break;
		                }
		                
				        voice[j].index[osc] += phase_correction
				            * halfsteps_to_freq_mult ( ( tmp_1
					        * patch->freq_lfo_amount[osc] )
							   + part.osc_pitch_bend[osc]
							   + patch->osc_transpose[osc] )
			                * voice[j].osc_freq[osc] * wave_period;
				        
				        while (voice[j].index[osc] < 0.0)
			                voice[j].index[osc] += F_WAVEFORM_SIZE;
		                while (voice[j].index[osc] >= F_WAVEFORM_SIZE)
			                voice[j].index[osc] -= F_WAVEFORM_SIZE;
				    }
				}
			}
			for (lfo = 0; lfo < NUM_LFOS; lfo++) {
			    if (patch->lfo_freq_base[lfo] >= FREQ_BASE_TEMPO) 
			    {
				    part.lfo_index[lfo] += phase_correction
				        * part.lfo_freq[lfo]
		                * halfsteps_to_freq_mult (patch->lfo_transpose[lfo] + part.lfo_pitch_bend[lfo])
		                * wave_period;
				    
				    while (part.lfo_index[lfo] < 0.0)
		                part.lfo_index[lfo] += F_WAVEFORM_SIZE;
		            while (part.lfo_index[lfo] >= F_WAVEFORM_SIZE)
		                part.lfo_index[lfo] -= F_WAVEFORM_SIZE;
				}
			}
			phase_correction = 0;
	    }
    }
    else if ((jack_state == JackTransportStopped) && (jack_prev_state == JackTransportRolling))
    {
        //current_beat = 1;
        //prev_beat = 1;
        
        /* send NOTE_OFFs on transport stop */
        engine_notes_off();
    }
    
    jack_prev_state = jack_state;

    /* Signal the engine that there's space again */
    if (pthread_mutex_trylock (&buffer_mutex) == 0) {
	pthread_cond_broadcast (&buffer_has_space);
	pthread_mutex_unlock (&buffer_mutex);
    }

    return 0;
}


/*****************************************************************************
 *
 * JACK_BUFSIZE_HANDLER()
 *
 * Gets called when jack sets or changes buffer size.
 *
 *****************************************************************************/
int 
jack_bufsize_handler(jack_nframes_t nframes, void *arg) {
    /* Begin buffer critical section */
    pthread_mutex_lock (&buffer_mutex);

    /* Make sure buffer doesn't get overrun */
    if (nframes > (PHASEX_MAX_BUFSIZE / 2)) {
	fprintf (stderr, "JACK requested buffer size:  %d.  Max is:  %d.\n",
		 nframes, (PHASEX_MAX_BUFSIZE / 2));
	phasex_shutdown ("Buffer size exceeded.  Exiting...\n");
    }
    buffer_size   = nframes * 2;
    buffer_full   = buffer_size;

    pthread_mutex_unlock (&buffer_mutex);
    /* End buffer critical section */

    if (debug) {
	fprintf (stderr, "JACK requested buffer size:  %d\n", nframes);
    }

    return 0;
}


/*****************************************************************************
 *
 * JACK_SAMPLERATE_HANDLER()
 *
 * Gets called when jack sets or changes sample rate.
 *
 *****************************************************************************/
int 
jack_samplerate_handler(jack_nframes_t nframes, void *arg) {

    if (debug) {
	fprintf (stderr, "JACK requested sample rate:  %d\n", nframes);
    }

    /* if jack requests a zero value, just use what we already have */
    if (nframes == 0) {
	return 0;
    }

    /* scale sample rate depending on mode */
    switch (sample_rate_mode) {
    case SAMPLE_RATE_UNDERSAMPLE:
	nframes /= 2;
	break;
    case SAMPLE_RATE_NORMAL:
	break;
    case SAMPLE_RATE_OVERSAMPLE:
	nframes *= 2;
	break;
    }

    pthread_mutex_lock (&sample_rate_mutex);
    if (nframes == sample_rate) {
	pthread_mutex_unlock (&sample_rate_mutex);
	return 0;
    }

    /* Changing sample rate currently not supported */
    if ((sample_rate > 0) && (sample_rate != nframes)) {
	/* TODO: rebuild _all_ samplerate dependent tables and vars here */
	phasex_shutdown ("Unable to change sample rate.  Exiting...\n");
    }

    /* First time setting sample rate */
    if (sample_rate == 0) {
	sample_rate    = nframes;
	f_sample_rate  = (sample_t)sample_rate;
	nyquist_freq   = (sample_t)(sample_rate / 2);
	wave_period    = (sample_t)(F_WAVEFORM_SIZE / (double)sample_rate);

	if (debug) {
	    fprintf (stderr, "Internal sample rate:  %d\n", nframes);
	}

	/* now that we have the sample rate, signal anyone else who needs to know */
	pthread_cond_broadcast (&sample_rate_cond);
    }

    pthread_mutex_unlock (&sample_rate_mutex);
    return 0;
}


/*****************************************************************************
 *
 * JACK_SHUTDOWN_HANDLER()
 *
 * Gets called when JACK shuts down or closes client.
 *
 *****************************************************************************/
void 
jack_shutdown_handler(void *arg) {

    /* set state so client can be restarted */
    jack_running = 0;
    jack_thread_p = 0;
    client = NULL;

    if (debug) {
	fprintf (stderr, "JACK shutdown handler called in client thread 0x%lx\n", pthread_self());
    }
}


/*****************************************************************************
 *
 * JACK_XRUN_HANDLER()
 *
 * Gets called when jack detects an xrun event.
 *
 *****************************************************************************/
int 
jack_xrun_handler(void *arg) {
    if (debug) {
	fprintf (stderr, "JACK xrun detected.\n");
    }
    return 0;
}


/*****************************************************************************
 *
 * JACK_RESTART()
 *
 * Closes the JACK client, which will cause the watchdog loop to restart it.
 * To be called from other threads.
 *
 *****************************************************************************/
void
jack_restart(void) {
    jack_client_t *tmp_client = client;

    if ((client != NULL) && jack_running && (jack_thread_p) != 0) {
	client = NULL;
	jack_running = 0;
	jack_thread_p = 0;

#ifdef JACK_DEACTIVATE_BEFORE_CLOSE
	jack_deactivate (tmp_client);
#endif
	//jack_thread_wait (tmp_client, 0);
	jack_client_close (tmp_client);

	/* zero out buffers */
	memset (part.delay_buf, 0, DELAY_MAX * 2 * sizeof (sample_t));
#ifdef INTERPOLATE_CHORUS
	memset (part.chorus_buf_1, 0, CHORUS_MAX * sizeof (sample_t));
	memset (part.chorus_buf_2, 0, CHORUS_MAX * sizeof (sample_t));
#else
	memset (part.chorus_buf, 0, CHORUS_MAX * 2 * sizeof (sample_t));
#endif
	pthread_mutex_lock (&buffer_mutex);

	memset (input_buffer1,  0, PHASEX_MAX_BUFSIZE * sizeof (jack_default_audio_sample_t));
	memset (input_buffer2,  0, PHASEX_MAX_BUFSIZE * sizeof (jack_default_audio_sample_t));
	memset (output_buffer1, 0, PHASEX_MAX_BUFSIZE * sizeof (jack_default_audio_sample_t));
	memset (output_buffer2, 0, PHASEX_MAX_BUFSIZE * sizeof (jack_default_audio_sample_t));

	buffer_full        = buffer_size;
	buffer_read_index  = 0;
	buffer_write_index = 0;

	pthread_mutex_unlock (&buffer_mutex);
    }
}


/*****************************************************************************
 *
 * JACK_INIT()
 *
 * Setup the jack client.
 *
 *****************************************************************************/
int
jack_init(void) {
    const char		*server_name = NULL;
    static char		client_name[32];
    jack_options_t	options = JackNoStartServer | JackUseExactName;
    jack_status_t	status = 0;

    if (debug) {
	fprintf (stderr, "Initializing JACK client from thread 0x%lx\n", pthread_self());
    }

    if (client != NULL) {
	if (debug) {
	    fprintf (stderr, "Warning: closing stale JACK client...\n");
	}
	jack_client_close (client);
	client = NULL;
    }

    /* open a client connection to the JACK server */
    for( phasex_instance = 0; phasex_instance != 16; phasex_instance++)
    {
    if(!phasex_instance) {
        snprintf (client_name, 32, "%s", phasex_title);
    }
    else {
        snprintf (client_name, 32, "%s-%02d", phasex_title, phasex_instance);
    }
    printf("Using client name %s\n", client_name);
    if (client = jack_client_open ((const char *)client_name, options, &status, server_name))
        break;
    }
    
    /* deal with non-unique client name */
    if (status & JackNameNotUnique) {
	fprintf (stderr, "Unable to start JACK client '%s'!\n", client_name);
	return 1;
    }

    /* deal with jack server problems */
    if (status & (JackServerFailed | JackServerError)) {
	if (debug) {
	    fprintf (stderr, "Unable to connect to JACK server.  Status = 0x%2.0x\n", status);
	}
	return 1;
    }

    /* deal with missing client */
    if (client == NULL) {
	if (debug) {
	    fprintf (stderr, "Unable to open JACK client.  Status = 0x%2.0x\n", status);
	}
	return 1;
    }

    /* callback for when jack shuts down (needs to be set as early as possible) */
    jack_on_shutdown (client, jack_shutdown_handler, 0);

    if (debug) {
	fprintf (stderr, "Unique JACK client name '%s' assigned.\n", client_name);
    }

    /* notify if we started jack server */
    if (status & JackServerStarted) {
	fprintf (stderr, "JACK server started.\n");
    }

    /* report realtime scheduling in JACK */
    if (debug) {
	if (jack_is_realtime (client)) {
	    fprintf (stderr, "JACK is running with realtime scheduling.\n");
	}
	else {
	    fprintf (stderr, "JACK is running without realtime scheduling.\n");
	}
    }

    /* get sample rate early and notify other threads */
    sample_rate = jack_get_sample_rate (client);

    if (debug) {
	fprintf (stderr, "JACK sample rate:  %d\n", sample_rate);
    }

    /* scale sample rate depending on mode */
    switch (sample_rate_mode) {
    case SAMPLE_RATE_UNDERSAMPLE:
	sample_rate /= 2;
	break;
    case SAMPLE_RATE_OVERSAMPLE:
	sample_rate *= 2;
	break;
    }

    /* set samplerate related vars */
    f_sample_rate = (sample_t)sample_rate;
    nyquist_freq  = (sample_t)(sample_rate / 2);
    wave_period   = (sample_t)(F_WAVEFORM_SIZE / (double)sample_rate);

    if (debug) {
	fprintf (stderr, "Internal sample rate:  %d\n", sample_rate);
    }

    /* callback for setting our sample rate when jack tells us to */
    jack_set_sample_rate_callback (client, jack_samplerate_handler, 0);

    /* now that we have the sample rate, signal anyone else who needs to know */
    pthread_mutex_lock (&sample_rate_mutex);
    pthread_cond_broadcast (&sample_rate_cond);
    pthread_mutex_unlock (&sample_rate_mutex);

    /* get buffer size */
    buffer_size = jack_get_buffer_size (client) * 2;
    if (buffer_size > (PHASEX_MAX_BUFSIZE / 2)) {
    	fprintf (stderr, "JACK requested buffer size:  %d.  Max is:  %d.\n",
		 buffer_size, (PHASEX_MAX_BUFSIZE / 2));
    	fprintf (stderr, "JACK buffer size exceeded.  Closing client...\n");
	jack_client_close (client);
	client = NULL;
	jack_running = 0;
	return 1;
    }
    buffer_full = buffer_size;
    if (debug) {
	fprintf (stderr, "JACK output buffer size:  %d.\n", buffer_size);
    }

    /* callback for setting our buffer size when jack tells us to */
    jack_set_buffer_size_callback (client, jack_bufsize_handler, 0);

    /* callback for dealing with xruns */
    jack_set_xrun_callback (client, jack_xrun_handler, 0);

    /* create ports */
    input_port1 = jack_port_register (client, "in_1",
				      JACK_DEFAULT_AUDIO_TYPE,
				      JackPortIsInput, 0);
    input_port2 = jack_port_register (client, "in_2",
				      JACK_DEFAULT_AUDIO_TYPE,
				      JackPortIsInput, 0);

    output_port1 = jack_port_register (client, "out_1",
				       JACK_DEFAULT_AUDIO_TYPE,
				       JackPortIsOutput, 0);
    output_port2 = jack_port_register (client, "out_2",
				       JACK_DEFAULT_AUDIO_TYPE,
				       JackPortIsOutput, 0);

    if ( (output_port1 == NULL) || (output_port2 == NULL) ||
	 (input_port1 == NULL) || (input_port2 == NULL) )
    {
	fprintf (stderr, "JACK has no output ports available.\n");
	jack_client_close (client);
	client = NULL;
	jack_running = 0;
	return 0;
    }

    /* set callback for jack to grab audio data */
    jack_set_process_callback (client, process_buffer, 0);

    return 0;
}


/*****************************************************************************
 *
 * JACK_START()
 *
 * Start jack client and attach to playback ports.
 *
 *****************************************************************************/
int
jack_start(void) {
    const char		**ports;
    char		*p;
    char		*q;
    int			j;

    /* activate client (callbacks start, so everything else needs to be ready) */
    if (jack_activate (client)) {
	fprintf (stderr, "Unable to activate JACK client.\n");
	jack_client_close (client);
	jack_running  = 0;
	jack_thread_p = 0;
	client        = NULL;
	return 1;
    }

    /* all up and running now */
    jack_running = 1;
    jack_thread_p = jack_client_thread_id (client);

    /* connect ports.  in/out is from server perspective */

    /* By default, connect PHASEX output to first two JACK hardware playback ports found */
    if (setting_jack_autoconnect) {
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical | JackPortIsInput);
	if ((ports == NULL) || (ports[0] == NULL)) {
	    fprintf (stderr, "Warning:  PHASEX output not connected!\n");
	    fprintf (stderr, "          (No physical JACK playback ports available.)\n");
	}
	else {
	    if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
		fprintf (stderr, "Unable to connect PHASEX output ports to JACK input.\n");
	    }
	    if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
		fprintf (stderr, "Unable to connect PHASEX output ports to JACK input.\n");
	    }
	}
    }

    /* otherwise, connect to ports that match -o command line flag */
    else {
	ports = jack_get_ports (client, NULL, NULL, JackPortIsInput);
	if ((ports == NULL) || (ports[0] == NULL)) {
	    fprintf (stderr, "Warning:  PHASEX output not connected!\n");
	    fprintf (stderr, "          (No JACK playback ports available.)\n");
	}
	else {
	    p = jack_outputs;
	    if (p != NULL) {
		if ((q = index (p, ',')) != NULL) {
		    *q = '\0';
		    q++;
		}
		else {
		    q = p;
		}
		for (j = 0; ports[j] != NULL; j++) {
		    if (strstr (ports[j], p) != NULL) {
			if (jack_connect (client, jack_port_name (output_port1), ports[j])) {
			    fprintf (stderr, "Unable to connect PHASEX output ports to JACK input.\n");
			}
			else if (debug) {
			    fprintf (stderr, "JACK connected PHASEX out_1 to %s.\n", ports[j]);
			}
			p = "_no_port_";
		    }
		    if (strstr (ports[j], q) != NULL) {
			if (jack_connect (client, jack_port_name (output_port2), ports[j])) {
			    fprintf (stderr, "Unable to connect PHASEX output ports to JACK input.\n");
			}
			else if (debug) {
			    fprintf (stderr, "JACK connected PHASEX out_2 to %s.\n", ports[j]);
			}
			q = "_no_port_";
		    }
		}
		if (p != q) {
		    q--;
		    *q = ',';
		}
	    }
	}
    }
    free (ports);

    /* By default, connect PHASEX input to first two JACK hardware capture ports found */
    if (setting_jack_autoconnect) {
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical | JackPortIsOutput);
	if ((ports == NULL) || (ports[0] == NULL) || (ports[1] == NULL)) {
	    fprintf (stderr, "Warning:  PHASEX input not connected!\n");
	    fprintf (stderr, "          (No physical JACK capture ports available.)\n");
	}
	else {
	    if (jack_connect (client, ports[0], jack_port_name (input_port1))) {
		fprintf (stderr, "Unable to connect PHASEX input ports to JACK output.\n");
	    }
	    if (jack_connect (client, ports[1], jack_port_name (input_port2))) {
		fprintf (stderr, "Unable to connect PHASEX input ports to JACK output.\n");
	    }
	}
    }

    /* otherwise, connect to ports that match -i command line flag */
    else {
	ports = jack_get_ports (client, NULL, NULL, JackPortIsOutput);
	if ((ports == NULL) || (ports[0] == NULL) || (ports[1] == NULL)) {
	    fprintf (stderr, "Warning:  PHASEX input not connected!\n");
	    fprintf (stderr, "          (No JACK capture ports available.)\n");
	}
	else {
	    p = jack_inputs;
	    if (p != NULL) {
		if ((q = index (p, ',')) != NULL) {
		    *q = '\0';
		    q++;
		}
		else {
		    q = p;
		}
		for (j = 0; ports[j] != NULL; j++) {
		    if (strstr (ports[j], p) != NULL) {
			if (jack_connect (client, ports[j], jack_port_name (input_port1))) {
			    fprintf (stderr, "Unable to connect PHASEX input ports to JACK output.\n");
			}
			else if (debug) {
			    fprintf (stderr, "JACK connected %s to PHASEX in_1.\n", ports[j]);
			}
			p = "_no_port_";
		    }
		    if (strstr (ports[j], q) != NULL) {
			if (jack_connect (client, ports[j], jack_port_name (input_port2))) {
			    fprintf (stderr, "Unable to connect PHASEX input ports to JACK output.\n");
			}
			else if (debug) {
			    fprintf (stderr, "JACK connected %s to PHASEX in_2.\n", ports[j]);
			}
			q = "_no_port_";
		    }
		}
	    }
	}
    }
    free (ports);

    /* report latency and total latency */
    if (debug) {
	fprintf (stderr, "JACK latency:  %d\n",
		 jack_port_get_latency (output_port1));
	fprintf (stderr, "JACK total latency:  %d\n",
		 jack_port_get_total_latency (client, output_port1));
    }

    return 0;
}


/*****************************************************************************
 *
 * JACK_WATCHDOG()
 *
 * Watch jack client thread and restart client when needed.
 *
 *****************************************************************************/
#define WATCHDOG_STATE_NO_CLIENT     0
#define WATCHDOG_STATE_CLIENT_INIT   1
#define WATCHDOG_STATE_CLIENT_ACTIVE 2
int
jack_watchdog(void) {
    int		watchdog_state = WATCHDOG_STATE_CLIENT_ACTIVE;

    while (!shutdown) {
#ifdef EXTRA_DEBUG
	if (debug) {
	    fprintf (stderr, "JACK Watchdog loop...\n");
	}
#endif

	if (gtkui_restarting) {
	    pthread_join (gtkui_thread_p, NULL);
	    start_gtkui_thread ();
	}

    /* poll lash for events */
    lash_pollevent();

	/* everybody sleeps */
	usleep (250000);

	switch (watchdog_state) {
	case WATCHDOG_STATE_CLIENT_ACTIVE:
	    /* make sure the thread didn't go away */
	    if (client == NULL) {
		if (debug && !shutdown) {
		    fprintf (stderr, "JACK Watchdog: JACK client went away!  Attempting to recover.\n");
		}
		watchdog_state = WATCHDOG_STATE_NO_CLIENT;
		usleep (250000);
	    }
	    else {
#ifdef JOIN_JACK_CLIENT_THREAD
		/* if there's a jack client thread, wait for it to die */
		if (debug) {
		    fprintf (stderr, "JACK Watchdog: Waiting for JACK client thread 0x%lx to exit.\n", jack_thread_p);
		}
		if (pthread_join (jack_thread_p, NULL) == 0) {
		    if (debug) {
			fprintf (stderr, "JACK Watchdog: pthread_join() successful!\n");
		    }
		    /* client is invalid at this point, so don't call jack_client_close() */
		    client = NULL;
		    jack_thread_p = 0;
		    jack_running = 0;
		    watchdog_state = WATCHDOG_STATE_NO_CLIENT;
		    usleep (250000);
		}
		else {
		    if (debug) {
			fprintf (stderr, "JACK Watchdog: pthread_join() on JACK client thread 0x%lx failed!\n", jack_thread_p);
		    }
		}
#else
		if (jack_thread_p == 0) {
		    if (debug) {
			fprintf (stderr, "JACK Watchdog: JACK thread went away!  Attempting to recover.\n");
		    }
		    client       = NULL;
		    jack_running = 0;
		    watchdog_state = WATCHDOG_STATE_NO_CLIENT;
		    usleep (250000);
		}
#endif
	    }
	    usleep (250000);
	    break;

	case WATCHDOG_STATE_NO_CLIENT:
	    /* open a new client */
	    if (jack_init () == 0) {
		if (debug) {
		    fprintf (stderr, "JACK Watchdog: Initialized JACK client.\n");
		}
		jack_thread_p = 0;
		jack_running = 0;
		watchdog_state = WATCHDOG_STATE_CLIENT_INIT;
	    }
	    else {
		if (debug) {
		    fprintf (stderr, "JACK Watchdog: Waiting for JACK server...\n");
		}
	    }
	    usleep (250000);
	    break;

	case WATCHDOG_STATE_CLIENT_INIT:
	    /* start client */
	    if (jack_start () == 0) {
		if (debug) {
		    fprintf (stderr, "JACK Watchdog: Started JACK with client thread 0x%lx\n", jack_thread_p);
		}
		jack_running = 1;
		watchdog_state = WATCHDOG_STATE_CLIENT_ACTIVE;
	    }
	    else {
		if (debug) {
		    fprintf (stderr, "JACK Watchdog: Unable to start JACK client.\n");
		}
		jack_thread_p = 0;
		if (client != NULL) {
		    /* no thread to wait for unless jack client actually starts */
		    jack_client_close (client);
		    client = NULL;
		}
		watchdog_state = WATCHDOG_STATE_NO_CLIENT;
	    }
	    usleep (250000);
	    break;
	}
    }

    usleep (250000);

    if ((client != NULL) && (jack_thread_p != 0)) {
	jack_client_close (client);
	client = NULL;
	jack_thread_p = 0;
    }

    /* not reached */
    return 0;
}

