/*****************************************************************************
 *
 * jack.h
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
#ifndef _PHASEX_JACK_H_
#define _PHASEX_JACK_H_

#include <jack/jack.h>
#include "phasex.h"


extern jack_client_t			*client;

extern pthread_mutex_t			buffer_mutex;
extern pthread_mutex_t			sample_rate_mutex;

extern pthread_cond_t			buffer_has_data;
extern pthread_cond_t			buffer_has_space;
extern pthread_cond_t			sample_rate_cond;

extern jack_default_audio_sample_t	input_buffer1[PHASEX_MAX_BUFSIZE];
extern jack_default_audio_sample_t	input_buffer2[PHASEX_MAX_BUFSIZE];
extern jack_default_audio_sample_t	output_buffer1[PHASEX_MAX_BUFSIZE];
extern jack_default_audio_sample_t	output_buffer2[PHASEX_MAX_BUFSIZE];

extern int				jack_running;

extern int				buffer_full;
extern int				buffer_size;
extern int				buffer_read_index;
extern int				buffer_write_index;

extern int				sample_rate;
extern int				sample_rate_mode;
extern sample_t				f_sample_rate;
extern sample_t				nyquist_freq;
extern sample_t				wave_period;
extern sample_t				wave_fp_period;

extern char				*jack_inputs;
extern char				*jack_outputs;


int	process_buffer	(jack_nframes_t nframes, void *arg);
void	jack_shutdown	(void *arg);
int	jack_init	(void);
int	jack_start	(void);
void	jack_restart	(void);
int	jack_watchdog	(void);


#endif /* _PHASEX_JACK_H_ */
