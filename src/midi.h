/*****************************************************************************
 *
 * midi.h
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
#ifndef _PHASEX_MIDI_H_
#define _PHASEX_MIDI_H_

#include <pthread.h>
#include <asoundlib.h>

#define MIDI_HOLD_PEDAL 64
#define MIDI_ALL_SOUND_OFF 120
#define MIDI_RESET_ALL_CTRLS 121
#define MIDI_ALL_NOTES_OFF 123


typedef struct midi_port {
    int			client;
    int			port;
    struct midi_port	*next;
} MIDI_PORT;

typedef struct midi {
    snd_seq_t		*seq;		/* ALSA sequencer handle			*/
    struct pollfd	*pfd;		/* polling descriptors for checking MIDI input	*/
    int			npfds;		/* number of polling descriptors		*/
    MIDI_PORT		*in_port;	/* ALSA MIDI port opened for input		*/
    MIDI_PORT		*src_ports;	/* ALSA MIDI output ports to connect to input	*/
    int			tick_sample;	/* samples since last midi clock tick		*/
    int			tick;		/* tick number (so we can reset waves every 64) */
} MIDI;


extern int		ccmatrix[128][16];

extern MIDI		*midi;

extern pthread_mutex_t	midi_ready_mutex;
extern pthread_cond_t	midi_ready_cond;

extern int		midi_ready;


void build_ccmatrix(void);
void dummy_midi_error_handler(const char *file, int line, const char *func, int err, const char *fmt, ...);
MIDI *open_alsa_midi_in(char *alsa_port);
void midi_cleanup(void *arg);
void *midi_thread(void *arg);


#endif /* _PHASEX_MIDI_H_ */
