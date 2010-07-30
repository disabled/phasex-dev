/*****************************************************************************
 *
 * callback.h
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
#ifndef _PHASEX_CALLBACK_H_
#define _PHASEX_CALLBACK_H_

#include <gtk/gtk.h>
#include "param.h"


/* helper functions */
int get_widget_val(GtkWidget *widget, PARAM *param);
void update_widget_val(GtkWidget *widget, PARAM *param);
int update_param_generic(GtkWidget *widget, PARAM *param);

/* update callback funtions, callable from anywhere */
void update_bpm(GtkWidget *widget, gpointer *data);
void update_master_tune(GtkWidget *widget, gpointer *data);
void update_portamento(GtkWidget *widget, gpointer *data);
void update_keymode(GtkWidget *widget, gpointer *data);
void update_keyfollow_vol(GtkWidget *widget, gpointer *data);
void update_volume(GtkWidget *widget, gpointer *data);
void update_transpose(GtkWidget *widget, gpointer *data);
void update_input_boost(GtkWidget *widget, gpointer *data);
void update_input_follow(GtkWidget *widget, gpointer *data);
void update_pan(GtkWidget *widget, gpointer *data);
void update_stereo_width(GtkWidget *widget, gpointer *data);
void update_amp_velocity(GtkWidget *widget, gpointer *data);
void update_filter_cutoff(GtkWidget *widget, gpointer *data);
void update_filter_resonance(GtkWidget *widget, gpointer *data);
void update_filter_smoothing(GtkWidget *widget, gpointer *data);
void update_filter_keyfollow(GtkWidget *widget, gpointer *data);
void update_filter_mode(GtkWidget *widget, gpointer *data);
void update_filter_type(GtkWidget *widget, gpointer *data);
void update_filter_gain(GtkWidget *widget, gpointer *data);
void update_filter_env_amount(GtkWidget *widget, gpointer *data);
void update_filter_env_sign(GtkWidget *widget, gpointer *data);
void update_filter_attack(GtkWidget *widget, gpointer *data);
void update_filter_decay(GtkWidget *widget, gpointer *data);
void update_filter_sustain(GtkWidget *widget, gpointer *data);
void update_filter_release(GtkWidget *widget, gpointer *data);
void update_filter_lfo(GtkWidget *widget, gpointer *data);
void update_filter_lfo_cutoff(GtkWidget *widget, gpointer *data);
void update_filter_lfo_resonance(GtkWidget *widget, gpointer *data);
void update_amp_attack(GtkWidget *widget, gpointer *data);
void update_amp_decay(GtkWidget *widget, gpointer *data);
void update_amp_sustain(GtkWidget *widget, gpointer *data);
void update_amp_release(GtkWidget *widget, gpointer *data);
void update_delay_mix(GtkWidget *widget, gpointer *data);
void update_delay_feed(GtkWidget *widget, gpointer *data);
void update_delay_flip(GtkWidget *widget, gpointer *data);
void update_delay_time(GtkWidget *widget, gpointer *data);
void update_delay_lfo(GtkWidget *widget, gpointer *data);
void update_chorus_mix(GtkWidget *widget, gpointer *data);
void update_chorus_feed(GtkWidget *widget, gpointer *data);
void update_chorus_flip(GtkWidget *widget, gpointer *data);
void update_chorus_time(GtkWidget *widget, gpointer *data);
void update_chorus_amount(GtkWidget *widget, gpointer *data);
void update_chorus_phase_rate(GtkWidget *widget, gpointer *data);
void update_chorus_phase_balance(GtkWidget *widget, gpointer *data);
void update_chorus_lfo_wave(GtkWidget *widget, gpointer *data);
void update_chorus_lfo_rate(GtkWidget *widget, gpointer *data);
void update_osc_modulation(GtkWidget *widget, gpointer *data);
void update_osc_wave(GtkWidget *widget, gpointer *data);
void update_osc_freq_base(GtkWidget *widget, gpointer *data);
void update_osc_rate(GtkWidget *widget, gpointer *data);
void update_osc_polarity(GtkWidget *widget, gpointer *data);
void update_osc_init_phase(GtkWidget *widget, gpointer *data);
void update_osc_transpose(GtkWidget *widget, gpointer *data);
void update_osc_fine_tune(GtkWidget *widget, gpointer *data);
void update_osc_pitchbend(GtkWidget *widget, gpointer *data);
void update_osc_am_lfo(GtkWidget *widget, gpointer *data);
void update_osc_am_lfo_amount(GtkWidget *widget, gpointer *data);
void update_osc_freq_lfo(GtkWidget *widget, gpointer *data);
void update_osc_freq_lfo_amount(GtkWidget *widget, gpointer *data);
void update_osc_freq_lfo_fine(GtkWidget *widget, gpointer *data);
void update_osc_phase_lfo(GtkWidget *widget, gpointer *data);
void update_osc_phase_lfo_amount(GtkWidget *widget, gpointer *data);
void update_osc_wave_lfo(GtkWidget *widget, gpointer *data);
void update_osc_wave_lfo_amount(GtkWidget *widget, gpointer *data);
void update_lfo_wave(GtkWidget *widget, gpointer *data);
void update_lfo_freq_base(GtkWidget *widget, gpointer *data);
void update_lfo_rate(GtkWidget *widget, gpointer *data);
void update_lfo_polarity(GtkWidget *widget, gpointer *data);
void update_lfo_init_phase(GtkWidget *widget, gpointer *data);
void update_lfo_transpose(GtkWidget *widget, gpointer *data);
void update_lfo_pitchbend(GtkWidget *widget, gpointer *data);


#endif /* _PHASEX_CALLBACK_H_ */

