/*****************************************************************************
 *
 * midi.c
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
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <asoundlib.h>
#include "phasex.h"
#include "midi.h"
#include "engine.h"
#include "patch.h"
#include "param.h"
#include "filter.h"
#include "gtkui.h"
#include "bank.h"
#include "settings.h"


int		ccmatrix[128][16];

MIDI		*midi;

pthread_mutex_t	midi_ready_mutex;
pthread_cond_t	midi_ready_cond		= PTHREAD_COND_INITIALIZER;

int		midi_ready		= 0;


/*****************************************************************************
 *
 * BUILD_CCMATRIX()
 *
 * Build the midi controller matrix from current param data.
 *
 *****************************************************************************/
void 
build_ccmatrix(void) {
    int		cc;
    int		id;
    int		j;

    /* wipe it clean first */
    for (cc = 0; cc < 128; cc++) {
	for (j = 0; j < 16; j++) {
	    ccmatrix[cc][j] = -1;
	}
    }
    
    /* now run through the params, one by one */
    for (id = 0; id < NUM_PARAMS; id++) {
	/* found a param that needs its cc num mapped */
	cc = param[id].cc_num;
	if ((cc  >= 0) && (cc < 128)) {
	    /* find end of current list for cc num */
	    j = 0;
	    while ((ccmatrix[cc][j] >= 0) && (j < 16)) {
		j++;
	    }
	    /* map it at the end of the array */
	    if (j < 16) {
		ccmatrix[cc][j] = id;
	    }
	}
    }
}


/*****************************************************************************
 *
 * DUMMY_MIDI_ERROR_HANDLER()
 *
 * Placeholder for a real error handling function.
 *
 *****************************************************************************/
void 
dummy_midi_error_handler(const char *file, int line, const char *func, int err, const char *fmt, ...) {
    fprintf (stderr, "Unhandled ALSA MIDI error.\n"
	     "Please make sure that the 'snd_seq_midi' and 'snd_seq_midi_event'\n"
	     "kernel modules are loaded and functioning properly.\n");
    phasex_shutdown ("PHASEX Exiting...\n");
}


/*****************************************************************************
 *
 * OPEN_ALSA_MIDI_IN()
 *
 * Creates MIDI input port and connects specified MIDI output ports to it.
 *
 *****************************************************************************/
MIDI *
open_alsa_midi_in(char *alsa_port) {
    char	client_name[32];
    char	port_name[32];
    MIDI	*new_midi;
    MIDI_PORT	*cur = NULL;
    MIDI_PORT	*prev = NULL;
    char	*o, *p, *q;
    char	*tokbuf;
    int		src_client = 0;
    int		src_port = 0;

    /* allocate our MIDI structure for returning everything */
    if ((new_midi = malloc (sizeof (MIDI))) == NULL) {
	phasex_shutdown	("Out of memory!\n");
    }
    if ((new_midi->in_port = malloc (sizeof (MIDI_PORT))) == NULL) {
	phasex_shutdown ("Out of memory!\n");
    }
    new_midi->in_port->next = NULL;
    new_midi->src_ports     = NULL;

    /*
     * get a comma separated client:port list from command line,
     *  or a '-' (or no -p arg) for open subscription
     */
    if (alsa_port != NULL) {
	if ((tokbuf = malloc (strlen (alsa_port) * 4)) == NULL) {
	    phasex_shutdown ("Out of memory!\n");
	}
	o = alsa_port;
	while ((p = strtok_r (o, ",", &tokbuf)) != NULL) {
	    o = NULL;
	    prev = cur;
	    if (*p == '-') {
		continue;
	    }
	    else if (isdigit (*p)) {
		if ((q = index (p, ':')) == NULL) {
		    fprintf (stderr, "Invalid ALSA MIDI client port '%s'.\n", alsa_port);
		    phasex_shutdown ("Unable to contiune with ALSA MIDI setup.\n");
		}
		src_client = atoi (p);
		src_port   = atoi (q + 1);
	    }
	    else {
		fprintf (stderr, "Invalid ALSA MIDI client port '%s'.\n", alsa_port);
		phasex_shutdown ("Unable to contiune with ALSA MIDI setup.\n");
	    }
	    if ((cur = malloc (sizeof (MIDI_PORT))) == NULL) {
		phasex_shutdown ("Out of memory!\n");
	    }
	    cur->client = src_client;
	    cur->port   = src_port;
	    cur->next   = NULL;
	    if (prev == NULL) {
		new_midi->src_ports = cur;
	    }
	    else {
		prev->next = cur;
	    }
	}
    }

    /* Grrr... need a better handler here */
    snd_lib_error_set_handler (dummy_midi_error_handler);

    /* open the sequencer */
    if (snd_seq_open (&new_midi->seq, "hw",
		      SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0) {
	phasex_shutdown ("Unable to open ALSA sequencer.\n");
    }

    /* extract our client id */
    new_midi->in_port->client = snd_seq_client_id (new_midi->seq);

    /* set our client name */
    snprintf (client_name, sizeof (client_name), "phasex-%02d", phasex_instance);
    if (snd_seq_set_client_name (new_midi->seq, client_name) < 0) {
	phasex_shutdown ("Unable to set ALSA sequencer client name.\n");
    }

    /* create a port */
    snprintf (port_name, sizeof (port_name), "phasex-%02d in", phasex_instance);
    new_midi->in_port->port =
	snd_seq_create_simple_port (new_midi->seq, port_name,
				    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
				    SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    /* since we opened nonblocking, we need our poll descriptors */
    if ((new_midi->npfds = snd_seq_poll_descriptors_count (new_midi->seq, POLLIN)) > 0) {
	if ((new_midi->pfd = malloc (new_midi->npfds * sizeof (struct pollfd))) == NULL) {
	    phasex_shutdown ("Out of memory!\n");
	}
	if (snd_seq_poll_descriptors (new_midi->seq, new_midi->pfd,
				      new_midi->npfds, POLLIN) <= 0) {
	    phasex_shutdown ("No ALSA sequencer descriptors to poll.\n");
	}
   }

    /* connect from list of specified source ports, if any */
    cur = new_midi->src_ports;
    while (cur != NULL) {
	if ((cur->client >= 0) && (cur->client != SND_SEQ_ADDRESS_SUBSCRIBERS))
	    snd_seq_connect_from (new_midi->seq, 0, cur->client, cur->port);
	cur = cur->next;
    }

    /* we're done, so hand over the goods */
    return new_midi;
}


/*****************************************************************************
 *
 * MIDI_CLEANUP()
 *
 * Cleanup handler for MIDI thread.
 * Closes MIDI ports.
 *
 *****************************************************************************/
void 
midi_cleanup(void *arg) {
    MIDI_PORT	*cur;

    /* disconnect from list of specified source ports, if any */
    cur = midi->src_ports;
    while (cur != NULL) {
	if ((cur->client >= 0) && (cur->client != SND_SEQ_ADDRESS_SUBSCRIBERS))
	    snd_seq_disconnect_from (midi->seq, 0, cur->client, cur->port);
	cur = cur->next;
    }

    /* close sequencer */
    snd_seq_close (midi->seq);
}


/*****************************************************************************
 *
 * MIDI_THREAD()
 *
 * MIDI input thread function.
 * Modifies patch, part, voice, and global parameters.
 *
 *****************************************************************************/
void *
midi_thread(void *arg) {
    struct sched_param	schedparam;
    pthread_t		thread_id;
    snd_seq_event_t	*ev		= NULL;	/* midi event pointer				*/
    int			keytrigger	= 0;	/* is there a new note in play this sample?	*/
    int			osc;			/* current oscillator				*/
    int			lfo;			/* current lfo					*/
    int			j;
    int			v		= 0;	/* current voice being allocated/deallocated	*/
    int			oldest_age;
    int			free_voice;
    int			steal_voice;
    int			same_key;
    int			same_key_oldest_age;
    int			cc;			/* last controller touched for midimap updates	*/
    int			id;			/* parameter id, used for finding callbacks	*/
    int			need_next_key;		/* find next key in round robin sequence?	*/
    int			staccato	= 1;
    sample_t		tmp;

    /* initialize keylist nodes */
    for (j = 0; j < 128; j++) {
	part.keylist[j].midi_key = j;
	part.keylist[j].next = NULL;
    }

    /* set realtime scheduling and priority */
    thread_id = pthread_self ();
    memset (&schedparam, 0, sizeof (struct sched_param));
    schedparam.sched_priority = setting_midi_priority;
    pthread_setschedparam (thread_id, setting_sched_policy, &schedparam);

    /* setup thread cleanup handler */
    pthread_cleanup_push (&midi_cleanup, NULL);

    /* flush MIDI input */
    if (poll (midi->pfd, midi->npfds, 0) > 0)
	while ((snd_seq_event_input (midi->seq, &ev) >= 0) && ev != NULL);

    /* broadcast the midi ready condition */
    pthread_mutex_lock (&midi_ready_mutex);
    midi_ready = 1;
    pthread_cond_broadcast (&midi_ready_cond);
    pthread_mutex_unlock (&midi_ready_mutex);

    /* MAIN LOOP: poll for midi input and process events */
    while (1) {

	/* set thread cancelation point */
	pthread_testcancel ();

	/* poll for new MIDI input */
	if (poll (midi->pfd, midi->npfds, 1000) > 0) {

	    /* cycle through all available events */
	    while ((snd_seq_event_input (midi->seq, &ev) >= 0) && ev != NULL) {

		/* make sure it's the channel we want */
		if ((ev->data.note.channel == setting_midi_channel) || (setting_midi_channel == 16)) {

		    switch (ev->type) {

			/* handle everything for note start */
		    case SND_SEQ_EVENT_NOTEON:
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "Event Note On:  note=%d  velocity=%d\n", ev->data.note.note, ev->data.note.velocity);
			}
#endif
			/* if this is velocity 0 style note off, fall through */
			if (ev->data.note.velocity > 0) {

			    /* keep track of previous to last key pressed! */
			    part.prev_key = part.midi_key;
			    part.last_key = part.midi_key = ev->data.note.note;

			    /* allocate voice for the different keymodes */
			    switch (patch->keymode) {
			    case KEYMODE_MONO_SMOOTH:
			    case KEYMODE_MONO_MULTIKEY:
				v = 0;
				break;
			    case KEYMODE_MONO_RETRIGGER:
				voice[v].keypressed = -1;
				v = (v + 1) % setting_polyphony;
				break;
			    case KEYMODE_POLY:
				v = 0;

				/* voice allocation with note stealing */
				oldest_age          = 0;
				free_voice          = -1;
				steal_voice         = setting_polyphony - 1;
				same_key            = -1;
				same_key_oldest_age = 0;
			
				/* look through all the voices */
				for (j = 0; j < setting_polyphony; j++) {

				    /* priority 1: a free voice */
				    if (voice[j].allocated == 0) {
					free_voice = steal_voice = j;
					break;
				    }
				    else {
					/* priority 2: look for the same note, keeping track of oldest */
//					if (voice[j].midi_key == part.midi_key) {
//					    same_key = j;
//					    if (voice[j].age > same_key_oldest_age) {
//						same_key_oldest_age = voice[j].age;
//						steal_voice = j;
//					    }
//					}
					/* priority 3: find the absolute oldest in play */
					if ((same_key == -1) && (voice[j].age > oldest_age)) {
					    oldest_age = voice[j].age;
					    steal_voice = j;
					}
				    }
				}

				/* priorities 1, 2, and 3 */
				if (free_voice >= 0) {
				    v = free_voice;
				}
//				else if (same_key >= 0) {
//				    v = same_key;
//				}
				else {
				    v = steal_voice;
				}

				break;
			    }

			    /* assign midi note */
			    voice[v].midi_key   = part.midi_key;
			    voice[v].keypressed = part.midi_key;

			    /* keep velocity for this note event */
			    voice[v].velocity               = part.velocity                 = ev->data.note.velocity;
			    voice[v].velocity_target_linear = voice[v].velocity_coef_linear =
				part.velocity_target        = part.velocity_coef            =
				((sample_t)part.velocity) * 0.01;
			    voice[v].velocity_target_log    = voice[v].velocity_coef_log =
				velocity_gain_table[patch->amp_velocity_cc][part.velocity];

			    /* staccato, or no previous notes in play */
			    if (part.prev_key == -1) {

				/* put this key at the start of the list */
				part.head = &part.keylist[part.midi_key];
				part.head->next = NULL;
			    }

			    /* legato, or previous notes still in play */
			    else {
				/* link this key to the end of the list, unlinking from the middle if necessary */
				part.cur = part.head;
				part.prev = NULL;
				while (part.cur != NULL) {
				    if (part.cur == &part.keylist[part.midi_key]) {
					if (part.prev) {
					    part.prev->next = part.cur->next;
					}
				    }
				    part.prev = part.cur;
				    part.cur = part.cur->next;
				}
				part.cur = &part.keylist[part.midi_key];
				part.prev->next = part.cur;
				part.cur->next = NULL;
			    }

			    /* set flag for portamento and envelope handling at bottom of loop */
			    keytrigger = 1;

			    /* break here instead of outside velocity if condition for fall through */
			    break;
			}

			/* no break, so velocity 0 style note off will pass through */

			/* only use note off for last key pressed */
		    case SND_SEQ_EVENT_NOTEOFF:
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "Event Note Off:  note=%d  velocity=%d\n", ev->data.note.note, ev->data.note.velocity);
			}
#endif
			part.prev_key = part.midi_key;
			part.midi_key = ev->data.note.note;

			switch (patch->keymode) {
			case KEYMODE_POLY:
			    /* find voice mapped to note being shut off */
			    v = -1;
			    for (j = 0; j < setting_polyphony; j++) {
				if (voice[j].midi_key == part.midi_key) {
				    voice[j].keypressed = -1;
				    v = j;
				}
			    }
			    if (v == -1) {
				v = 0;
				keytrigger = 0;
			    }
			    break;

			case KEYMODE_MONO_RETRIGGER:
			    /* find voice mapped to note being shut off, if any */
			    keytrigger = 0;
			    for (j = 0; j < setting_polyphony; j++) {
				if (voice[j].keypressed == part.midi_key) {
				    voice[j].keypressed = -1;
				    v = (j + 1) % setting_polyphony;
				}
			    }
			    break;

			case KEYMODE_MONO_SMOOTH:
			case KEYMODE_MONO_MULTIKEY:
			    /* mono modes need keytrigger activities on note off */
			    v = 0;
			    if (part.prev_key == part.midi_key) {
				keytrigger = 1;
			    }
			    break;
			}

			/* remove this key from the list and then find the last key on the list */

			/* walk the list */
			part.prev = NULL;
			part.cur = part.head;
			while (part.cur != NULL) {

			    /* if note is found, unlink it from the list */
			    if (part.cur->midi_key == part.midi_key) {
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

			/* check for keys left on the list */
			if (part.prev != NULL) {
			    /* retrigger mode needs voice allocation with keys still on list */
			    if ((patch->keymode == KEYMODE_MONO_RETRIGGER) && (part.last_key != part.prev->midi_key)) {
				voice[v].midi_key   = part.midi_key;
				voice[v].keypressed = part.midi_key;
				keytrigger = 1;
			    }

			    part.last_key = part.midi_key = part.prev->midi_key;
			    part.prev->next = NULL;

			    /* multikey always need osc remapping until all keys are done */
			    if (patch->keymode == KEYMODE_MONO_MULTIKEY) {
				keytrigger = 1;
			    }
			}
			/* re-init list if no keys */
			else {
			    //part.prev_key	= part.midi_key;
			    voice[v].midi_key   = -1;
			    voice[v].keypressed = -1;
			    part.midi_key       = -1;
			    part.head           = NULL;
			    keytrigger          = 0;
			}
			break;

		    /* key aftertouch pressure */
		    case SND_SEQ_EVENT_KEYPRESS:
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "  Event Keypress:  note=%d  velocity=%d\n", ev->data.note.note, ev->data.note.velocity);
			}
#endif
			for (v = 0; v < setting_polyphony; v++) {
			    if (ev->data.note.note == voice[v].midi_key) {
				voice[v].velocity               = ev->data.note.velocity;
				voice[v].velocity_target_linear = (sample_t)ev->data.note.velocity * 0.01;
				voice[v].velocity_target_log    = velocity_gain_table[patch->amp_velocity_cc][ev->data.note.velocity];
			    }
			}
			break;
		    case SND_SEQ_EVENT_SONGPOS:
			if (debug) {
			    fprintf (stderr, "-- Song Position Pointer: %d %d\n",
				     ev->data.control.param, ev->data.control.value);
			}
			break;
		    case SND_SEQ_EVENT_START:
			if (debug) {
			    fprintf (stderr, "-- Start Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
			break;
		    case SND_SEQ_EVENT_CONTINUE:
			if (debug) {
			    fprintf (stderr, "-- Continue Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
			break;
		    case SND_SEQ_EVENT_STOP:
			if (debug) {
			    fprintf (stderr, "-- Stop Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
			break;
		    case SND_SEQ_EVENT_SETPOS_TICK:
			if (debug) {
			    fprintf (stderr, "-- Set Position Tick Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
			break;
		    case SND_SEQ_EVENT_TICK:
			if (debug) {
			    fprintf (stderr, "-- Tick Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
			break;

		    case SND_SEQ_EVENT_PGMCHANGE:
			/* handle patch selection here */
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "-- Program Change: %d %d\n",
				   ev->data.control.param, ev->data.control.value);
			}
#endif
			program_change_request = ev->data.control.value % 128;
			break;

			/* channel pressure / polypressure */
		    case SND_SEQ_EVENT_CHANPRESS:
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "Event Channel Pressure:  %d\n", ev->data.control.value);
			}
#endif
			part.velocity_target = (sample_t)ev->data.control.value * 0.01;
			for (v = 0; v < setting_polyphony; v++) {
			    if (voice[v].active) {
				voice[v].velocity               = ev->data.control.value;
				voice[v].velocity_target_linear = (sample_t)ev->data.control.value * 0.01;
				voice[v].velocity_target_log    = velocity_gain_table[patch->amp_velocity_cc][ev->data.control.value];
			    }
			}
			break;

			/* map controllers to internal params here */
		    case SND_SEQ_EVENT_CONTROLLER:
			/* get controller number */
			cc = ev->data.control.param % 128;

			/* now walk through the params in the matrix */
			j = 0;
			while (((id = ccmatrix[cc][j]) >= 0) && (j < 16)) {
			    /* set value for parameter */
			    param[id].cc_val[program_number] = ev->data.control.value %= 128;
			    /* execute the callback */
			    param[id].callback (NULL, (gpointer)(&(param[id])));
			    /* mark as modified */
			    patch->modified = 1;
			    j++;
			}

			/* check for active cc edit */
			if (!cc_edit_ignore_midi && cc_edit_active) {
			    cc_edit_cc_num = cc;
			}
			break;

		    case SND_SEQ_EVENT_PITCHBEND:
			part.pitch_bend_target = (sample_t)(ev->data.control.value) * 0.0001220703125; /* 1 / 8192 */
			break;

#ifdef MIDI_CLOCK_SYNC
			/* Warning: this is old and partly broken code */
			/* sync tempo based oscs to MIDI clock ticks */
		    case SND_SEQ_EVENT_CLOCK:
#ifdef EXTRA_DEBUG
			if (debug) {
			    fprintf (stderr, "-- Clock Message: %d %d\n",
				     ev->data.queue.param.d32[0], ev->data.queue.param.d32[1]);
			}
#endif
			break;

			/* new beats per second is a decayed average */
			global.bps = ((global.bps * 767.0) + (f_sample_rate / (24.0 * (sample_t)midi->tick_sample))) / 768.0;

			//if (!(midi->tick % 24))
			//    fprintf (stderr, "tick: %1.9f %d %d\n", bps, (int)(f_sample_rate / bps), midi->tick_sample);

			midi->tick_sample = 0;

			/* handle tempo based oscillators and lfos */
			for (j = 0; j < setting_polyphony; j++) {
			    if (voice[j].active) {
				for (osc = 0; osc < NUM_OSCS; osc++) {
				    if (patch->osc_freq_base[osc] >= FREQ_BASE_TEMPO) {
					voice[j].osc_freq[osc] = patch->osc_rate[osc] * global.bps;
				    }
				}
			    }
			}
			for (lfo = 0; lfo < NUM_LFOS; lfo++) {
			    if (patch->lfo_freq_base[lfo] >= FREQ_BASE_TEMPO) {
				part.lfo_freq[lfo]   = patch->lfo_rate[lfo] * global.bps;
				part.lfo_adjust[lfo] = part.lfo_freq[lfo] * (F_WAVEFORM_SIZE / f_sample_rate);
			    }
			}

			/* resync LFO frequencies (not phases) once per bar */
			midi->tick++;
			if ((midi->tick % 96) == 0) {
			    part.delay_size       = patch->delay_time * f_sample_rate / global.bps;
			    for (lfo = 0; lfo < NUM_LFOS; lfo++) {
				part.lfo_freq[lfo]   = global.bps * patch->lfo_rate[lfo];
				part.lfo_adjust[lfo] = part.lfo_freq[lfo] * F_WAVEFORM_SIZE / f_sample_rate;
			    }
			}

			/* hack: reset time based oscs every 64 beats */
			midi->tick %= (64 * 24);
			if (midi->tick == 0) {
			    for (osc = 0; osc < NUM_OSCS; osc++) {
				for (j = 0; j < setting_polyphony; j++) {
				    voice[j].index[osc] = part.osc_init_index[osc];
				}
			    }
			    for (lfo = 0; lfo < NUM_LFOS; lfo++) {
				part.lfo_index[lfo] = part.lfo_init_index[lfo];
			    }
			}

			break;
#endif /* MIDI_CLOCK_SYNC */
		    } /* end switch(event type) */

		    /* handle lfo init phases and envelope triggering for all keymodes */

		    /* check for key changes (on or off) that still need to be dealt with */
		    if (keytrigger) {

			/* check for notes currently in play */
			switch (patch->keymode) {
			case KEYMODE_MONO_SMOOTH:
			case KEYMODE_MONO_RETRIGGER:
			case KEYMODE_MONO_MULTIKEY:
			    //if (voice[v].cur_amp_interval == ENV_INTERVAL_DONE)
			    if (voice[v].cur_amp_interval >= ENV_INTERVAL_RELEASE) {
				staccato = 1;
			    }
			    else {
				staccato = 0;
			    }
			    break;
			case KEYMODE_POLY:
			    staccato = 1;
			    for (j = 0; j < setting_polyphony; j++) {
				//if (voice[j].allocated || (voice[j].cur_amp_interval != ENV_INTERVAL_DONE))
				if (voice[j].cur_amp_interval < ENV_INTERVAL_RELEASE) {
				    staccato = 0;
				    break;
				}
			    }
			    break;
			}

			/* staccato playing, mono retrig mode, and poly get new envelopes and initphases */
			if ((patch->keymode == KEYMODE_POLY) || (staccato) || (patch->keymode == KEYMODE_MONO_RETRIGGER)) {

			    /* start new amp and filter envelopes */
			    voice[v].cur_amp_interval    = ENV_INTERVAL_ATTACK;
			    voice[v].cur_filter_interval = ENV_INTERVAL_ATTACK;

			    voice[v].cur_amp_sample    = voice[v].amp_env_dur[ENV_INTERVAL_ATTACK]
				                       = env_table[patch->amp_attack];
			    voice[v].cur_filter_sample = voice[v].filter_env_dur[ENV_INTERVAL_ATTACK]
				                       = env_table[patch->filter_attack];

			    voice[v].amp_env_delta[ENV_INTERVAL_ATTACK]    =
				(1.0 - voice[v].amp_env_raw)    / (sample_t)voice[v].amp_env_dur[ENV_INTERVAL_ATTACK];
			    voice[v].filter_env_delta[ENV_INTERVAL_ATTACK] =
				(1.0 - voice[v].filter_env_raw) / (sample_t)voice[v].filter_env_dur[ENV_INTERVAL_ATTACK];

			    /* init phase for keytrig oscs */
			    for (osc = 0; osc < NUM_OSCS; osc++) {

				/* init phase (unshifted by lfo) at note start */
				if ((patch->osc_freq_base[osc] == FREQ_BASE_TEMPO_KEYTRIG) || (patch->osc_freq_base[osc] == FREQ_BASE_MIDI_KEY)) {
				    voice[v].index[osc] = part.osc_init_index[osc];
				}
			    }

			    /* init phase for keytrig lfos if needed */
			    //if ((staccato) || (patch->keymode == KEYMODE_MONO_RETRIGGER)) {
			    if (staccato) {
				for (lfo = 0; lfo < NUM_LFOS; lfo++) {
				    switch (patch->lfo_freq_base[lfo]) {
				    case FREQ_BASE_MIDI_KEY:
				    case FREQ_BASE_TEMPO_KEYTRIG:
					part.lfo_index[lfo] = part.lfo_init_index[lfo];
					/* intentional fallthrough */
				    case FREQ_BASE_TEMPO:
					part.lfo_adjust[lfo] = part.lfo_freq[lfo] * wave_period;
					break;
				    }
				}
			    }
			}

			/* legato in smooth modes */
			//else {
			    /* start portamento */
			    //part.portamento_sample     = part.portamento_samples;
			    //voice[v].portamento_sample = voice[v].portamento_samples;
			//}

			/* high key and low key are set to last key and adjusted later if necessary */
			part.high_key = part.low_key = part.last_key;

			/* set highest and lowest keys in play for things that need keyfollow */
			part.cur = part.head;
			while (part.cur != NULL) {
			    if (part.cur->midi_key < part.low_key) {
				part.low_key = part.cur->midi_key;
			    }
			    if (part.cur->midi_key > part.high_key) {
				part.high_key = part.cur->midi_key;
			    }
			    part.cur = part.cur->next;
			}

			/* volume keyfollow is set by last key for poly */
			if (patch->keymode == KEYMODE_POLY) {
			    voice[v].vol_key = part.last_key;
			}
			/* volume keyfollow is set by high key for mono */
			else {
			    voice[v].vol_key = part.high_key;
			}

			/* set filter keyfollow key based on keyfollow mode */
			switch (patch->filter_keyfollow) {
			case KEYFOLLOW_LAST:
			    tmp = (sample_t)(part.last_key + patch->transpose - 64);
			    for (j = 0; j < setting_polyphony; j++) {
				voice[j].filter_key_adj = tmp;
			    }
			    break;
			case KEYFOLLOW_HIGH:
			    tmp = (sample_t)(part.high_key + patch->transpose - 64);
			    for (j = 0; j < setting_polyphony; j++) {
				voice[j].filter_key_adj = tmp;
			    }
			    break;
			case KEYFOLLOW_LOW:
			    tmp = (sample_t)(part.low_key + patch->transpose - 64);
			    for (j = 0; j < setting_polyphony; j++) {
				voice[j].filter_key_adj = tmp;
			    }
			    break;
			case KEYFOLLOW_MIDI:
			    voice[v].filter_key_adj = (sample_t)(part.last_key + patch->transpose - 64);
			    break;
			case KEYFOLLOW_NONE:
			    voice[v].filter_key_adj = 0.0;
			    break;
			}

			/* start at beginning of list of keys in play */
			part.cur = part.head;

			/* keytrigger volume only applicable to midi key based oscs */
			/* portamento applicable to midi key based oscs _and_ lfos */

			/* handle per-osc portamento for the different keymodes */
			for (osc = 0; osc < NUM_OSCS; osc++) {
			    need_next_key = 0;

			    /* portamento for midi key based main osc */
			    if (patch->osc_freq_base[osc] == FREQ_BASE_MIDI_KEY) {

				/* decide which key in play to assign to this oscillator */
				switch (patch->keymode) {
				case KEYMODE_MONO_MULTIKEY:
				    /* use notes in order in oscillators */
				    if (part.cur != NULL) {
					voice[v].osc_key[osc] = part.cur->midi_key;
					need_next_key = 1;
				    }
				    else {
					voice[v].osc_key[osc] = part.last_key;
				    }
				    break;
				case KEYMODE_MONO_SMOOTH:
				case KEYMODE_MONO_RETRIGGER:
				    /* default mono -- use key just pressed */
				    voice[v].osc_key[osc] = part.last_key;
				    break;
				case KEYMODE_POLY:
				    /* use midi key assigned to voice */
				    voice[v].osc_key[osc] = voice[v].midi_key;
				    break;
				}

				/* set oscillator frequencies based on midi note, global transpose value, and optional portamento */
				/* note: patch->osc_transpose[osc] is taken into account every sample in the engine */
				if ((patch->portamento > 0)) {
				    /* in poly, portamento always starts from previous key hit, no matter which voice */
				    if (patch->keymode == KEYMODE_POLY) {
					if (part.prev_key == -1) {
					    voice[v].osc_freq[osc] = freq_table[patch->master_tune_cc][256 + part.last_key + patch->transpose];
					}
					else {
					    voice[v].osc_freq[osc] = freq_table[patch->master_tune_cc][256 + part.prev_key + patch->transpose];
					}
				    }
				    /* portamento slide calculation works the same for all keymodes */
				    voice[v].osc_portamento[osc] = 4.0 *
					(freq_table[patch->master_tune_cc][256 + voice[v].osc_key[osc] + patch->transpose] - voice[v].osc_freq[osc])
					/ (sample_t)(voice[v].portamento_samples - 1);
				    /* start portamento now that frequency adjustment is known */
				    part.portamento_sample     = part.portamento_samples;
				    voice[v].portamento_sample = voice[v].portamento_samples;
				}

				/* if portamento is not needed, set the oscillator frequency directly */
				else {
				    voice[v].osc_freq[osc] = freq_table[patch->master_tune_cc][256 + voice[v].osc_key[osc] + patch->transpose];
				}

				if (need_next_key) {
				    if (part.cur && part.cur->next) {
					part.cur = part.cur->next;
				    }
				    else {
					part.cur = part.head;
				    }
				}
			    }
			}

			/* portamento for midi key based lfo */
			for (lfo = 0; lfo < NUM_LFOS; lfo++) {
			    need_next_key = 0;

			    if (patch->lfo_freq_base[lfo] == FREQ_BASE_MIDI_KEY) {

				/* decide which key in play to assign to this lfo */
				switch (patch->keymode) {
				case KEYMODE_MONO_MULTIKEY:
				    /* use notes in order in lfos */
				    if (part.cur != NULL) {
					part.lfo_key[lfo] = part.cur->midi_key;
					need_next_key = 1;
				    }
				    else {
					part.lfo_key[lfo] = part.last_key;
				    }
				    break;
				case KEYMODE_MONO_SMOOTH:
				case KEYMODE_MONO_RETRIGGER:
				    /* default mono -- use key just pressed */
				    part.lfo_key[lfo] = part.last_key;
				    break;
				case KEYMODE_POLY:
				    /* use midi key assigned to allocated voice */
				    part.lfo_key[lfo] = voice[v].midi_key;
				    break;
				}

				/* set lfo portamento frequencies based on midi note and transpose value */
				if ((patch->portamento > 0) && (part.portamento_samples > 0)) {
				    part.lfo_portamento[lfo] =
					(freq_table[patch->master_tune_cc][256 + part.lfo_key[lfo]] - part.lfo_freq[lfo])
					/ (sample_t)part.portamento_samples;
				}

				/* if portamento is not needed, set the lfo frequency directly */
				else {
				    part.lfo_portamento[lfo] = 0.0;
				    part.lfo_freq[lfo] = freq_table[patch->master_tune_cc][256 + part.lfo_key[lfo]];
				}

				if (need_next_key) {
				    if (part.cur && part.cur->next) {
					part.cur = part.cur->next;
				    }
				    else {
					part.cur = part.head;
				    }
				}
			    }
			}

		    } /* end portamento if condition */

		    if (keytrigger) {
			/* allocate voice (engine actually activates allocated voices) */
			voice[v].allocated = 1;
			keytrigger         = 0;
		    }
		} /* end if (channel) */
		snd_seq_free_event (ev);
		ev = NULL;
	    } /* end while(event) */

	} /* end if(poll) */

    } /* end while(shutdown == 0) */

    /* execute cleanup handler and remove it */
    pthread_cleanup_pop (1);

    /* end of MIDI thread */
    return NULL;
}
