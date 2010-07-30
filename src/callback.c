/*****************************************************************************
 *
 * callback.c
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
#include <math.h>
#include <gtk/gtk.h>
#include "phasex.h"
#include "settings.h"
#include "wave.h"
#include "filter.h"
#include "engine.h"
#include "patch.h"
#include "midi.h"
#include "param.h"
#include "callback.h"
#include "gtkknob.h"
#include "bank.h"
#include "gtkui.h"


#define PARAM_VAL_IGNORE	-131073


/*****************************************************************************
 *
 * GET_WIDGET_VAL()
 *
 * Return the current value of a widget, cast to int.
 *
 *****************************************************************************/
int 
get_widget_val(GtkWidget *widget, PARAM *param) {
    int		val;
    int		j;

    /* set val to int value from param in case a new val is not caught here */
    val = param->int_val[program_number];

    switch (param->type) {
    case PARAM_TYPE_INT:
    case PARAM_TYPE_REAL:
	/* get the adjustment value used for both knob and spin */
	val = (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));
	break;
    case PARAM_TYPE_RATE:
	/* get value from the entry or knob widget that was modified */
	if (widget == param->text) {
	    val = get_rate_ctlr ((char *)gtk_entry_get_text (GTK_ENTRY (param->text)), NULL, 0);
	}
	else if ((void *)widget == (void *)param->adj) {
	    val = (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));
	}
	else {
	    val = PARAM_VAL_IGNORE;
	}
	break;
    case PARAM_TYPE_DTNT:
	/* get value from adjustment for knob widget that was modified */
	val = (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));
	gtk_label_set_text (GTK_LABEL (param->text), param->list_labels[val]);
	break;
    case PARAM_TYPE_BOOL:
	/* Two radio buttons work a bit differently than three or more */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param->button[1]))) {
	    val = 1;
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param->button[0]))) {
	    val = 0;
	}
	else {
	    val = PARAM_VAL_IGNORE;
	}
	break;
    case PARAM_TYPE_BBOX:
	/* Make sure only a button toggling on actually does anything */
	/* figure out which button this is */
	/* for buttons toggling off, return out of bounds value that get caught */
	val = PARAM_VAL_IGNORE;
	for (j = 0; j <= param->cc_limit; j++) {
	    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param->button[j]))) {
		val = j;
		break;
	    }
	}
	break;
    }

    return val;
}

/*****************************************************************************
 *
 * UPDATE_PARAM_GENERIC()
 *
 * Generic handler to be called at the beginning of every update callback.
 *
 *****************************************************************************/
int 
update_param_generic(GtkWidget *widget, PARAM *param) {
    int		val = PARAM_VAL_IGNORE;

    /* ignore locked parameters except for explicit gui parameter control */
    if ((param->locked) && (widget == NULL)) {
	param->cc_val[program_number]  = param->cc_cur;
	param->int_val[program_number] = param->cc_cur + param->pre_inc;
	return PARAM_VAL_IGNORE;
    }

    /* called from midi thread or indirectly from gui */
    if ((widget == NULL) || (widget == main_window)) {
	if (param->cc_val[program_number] > param->cc_limit) {
	    param->cc_val[program_number] = param->cc_limit;
	}
	param->cc_prev                 = param->cc_cur;
	param->cc_cur                  = param->cc_val[program_number];
	param->int_val[program_number] = param->cc_cur + param->pre_inc;
	if (param->cc_cur != param->cc_prev) {
	    param->updated  = 1;
	}
//#ifdef EXTRA_DEBUG
//	if (debug) {
//	    printf ("MIDI Update ---- [%1d] -- cc_val=%03d -- int_val=%03d -- <%s>\n",
//		    param->index, param->cc_val[program_number], param->int_val[program_number], param->name);
//	}
//#endif
	val = param->cc_cur;
    }

    /* called directly from gui widget callback */
    else {
	val = get_widget_val (widget, param);
	if (val == PARAM_VAL_IGNORE) {
	    return PARAM_VAL_IGNORE;
	}
	param->int_val[program_number] = val;
	param->cc_prev                 = param->cc_cur;
	param->cc_cur                  = param->int_val[program_number] - param->pre_inc;
	param->cc_val[program_number]  = param->cc_cur;

	/* ignore when value doesn't change */
	if (param->cc_cur == param->cc_prev) {
	    return PARAM_VAL_IGNORE;
	}

	/* rate parameters still need another gui update */
	if (param->type == PARAM_TYPE_RATE) {
	    param->updated = 1;
	}

#ifdef EXTRA_DEBUG
	if (debug) {
	    printf ("GUI Update ----- [%1d] -- old ( cc_val = %03d ) -- new ( cc_val = %03d ) -- <%s>\n",
		    param->index, param->cc_prev, param->cc_val[program_number], param->name);
	}
#endif
    }

    return val;
}

/*****************************************************************************
 *
 * UPDATE_BPM()
 *
 *****************************************************************************/
void
update_bpm(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		lfo;
    int		osc;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    /* initialize all variables based on bpm */
    patch->bpm_cc     = cc_val;
    patch->bpm        = (sample_t)int_val;
    global.bps        = patch->bpm / 60.0;
    midi->tick_sample = f_sample_rate / (global.bps * 24.0);

    /* re-initialize delay size */
    part.delay_size   = patch->delay_time * f_sample_rate / global.bps;
    part.delay_length = (int)(part.delay_size);

    /* re-initialize chorus lfos */
    part.chorus_lfo_freq     = global.bps * patch->chorus_lfo_rate;
    part.chorus_lfo_adjust   = part.chorus_lfo_freq * wave_period;
    part.chorus_phase_freq   = global.bps * patch->chorus_phase_rate;
    part.chorus_phase_adjust = part.chorus_phase_freq * wave_period;

    /* per-lfo setup */
    for (lfo = 0; lfo < NUM_LFOS; lfo++) {
	/* re-calculate frequency and corresponding index adjustment */
	if (patch->lfo_freq_base[lfo] >= FREQ_BASE_TEMPO) {
	    part.lfo_freq[lfo]    = global.bps * patch->lfo_rate[lfo];
	    part.lfo_adjust[lfo]  = part.lfo_freq[lfo] * wave_period;
	}
    }

    /* per-oscillator setup */
    for (osc = 0; osc < NUM_OSCS; osc++) {
	/* re-calculate tempo based osc freq */
	if (patch->osc_freq_base[osc] >= FREQ_BASE_TEMPO) {
	    for (j = 0; j < setting_polyphony; j++) {
		voice[j].osc_freq[osc] = global.bps * patch->osc_rate[osc];
	    }
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_MASTER_TUNE()
 *
 *****************************************************************************/
void
update_master_tune(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->master_tune_cc = cc_val;
    patch->master_tune    = int_val;
}

/*****************************************************************************
 *
 * UPDATE_PORTAMENTO()
 *
 *****************************************************************************/
void
update_portamento(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->portamento        = cc_val;
    part.portamento_samples  = env_table[cc_val];
    for (j = 0; j < setting_polyphony; j++) {
	voice[j].portamento_samples = env_table[cc_val];
    }
}

/*****************************************************************************
 *
 * UPDATE_KEYMODE()
 *
 *****************************************************************************/
void
update_keymode(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->keymode = cc_val % 4;

    /* deactivate voices */
    for (j = 0; j < MAX_VOICES; j++) {
	voice[j].active    = 0;
	voice[j].allocated = 0;
	voice[j].midi_key  = -1;
    }

    /* mono keeps voice 0 active */
    if (patch->keymode != KEYMODE_POLY) {
	voice[0].active    = 1;
	voice[0].allocated = 1;
    }
}

/*****************************************************************************
 *
 * UPDATE_KEYFOLLOW_VOL()
 *
 *****************************************************************************/
void
update_keyfollow_vol(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->keyfollow_vol = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_VOLUME()
 *
 *****************************************************************************/
void
update_volume(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->volume_cc = cc_val;
    patch->volume    = gain_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_TRANSPOSE()
 *
 *****************************************************************************/
void
update_transpose(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->transpose_cc = cc_val;
    patch->transpose    = int_val;
    for (j = 0; j < setting_polyphony; j++) {
	voice[j].need_portamento = 1;
    }
}

/*****************************************************************************
 *
 * UPDATE_INPUT_BOOST()
 *
 *****************************************************************************/
void
update_input_boost(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->input_boost_cc = cc_val;
    patch->input_boost    = 1.0 + (((sample_t)int_val) / 16.0);
}

/*****************************************************************************
 *
 * UPDATE_INPUT_FOLLOW()
 *
 *****************************************************************************/
void
update_input_follow(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->input_follow = cc_val % 2;

}

/*****************************************************************************
 *
 * UPDATE_PAN()
 *
 *****************************************************************************/
void
update_pan(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->pan_cc = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_STEREO_WIDTH()
 *
 *****************************************************************************/
void
update_stereo_width(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->stereo_width_cc = cc_val;
    patch->stereo_width    = ((sample_t)(int_val) / 254.0) + 0.5;
}

/*****************************************************************************
 *
 * UPDATE_AMP_VELOCITY()
 *
 *****************************************************************************/
void
update_amp_velocity(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->amp_velocity_cc = cc_val;
    patch->amp_velocity    = ((sample_t)int_val) * 0.0078125; // / 128.0;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_CUTOFF()
 *
 *****************************************************************************/
void
update_filter_cutoff(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_cutoff_cc   = cc_val;
    part.filter_cutoff_target = (sample_t)int_val;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_RESONANCE()
 *
 *****************************************************************************/
void
update_filter_resonance(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_resonance_cc = cc_val;
    patch->filter_resonance    = ((sample_t)int_val) * 0.0078125; // / 128
}

/*****************************************************************************
 *
 * UPDATE_FILTER_SMOOTHING()
 *
 *****************************************************************************/
void
update_filter_smoothing(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_smoothing   = cc_val;
    part.filter_smooth_len    = ((sample_t)(cc_val + 1)) * 160.0;
    part.filter_smooth_factor = 1.0 / (part.filter_smooth_len + 1.0);
}

/*****************************************************************************
 *
 * UPDATE_FILTER_KEYFOLLOW()
 *
 *****************************************************************************/
void
update_filter_keyfollow(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_keyfollow = cc_val % 5;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_MODE()
 *
 *****************************************************************************/
void
update_filter_mode(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_mode = cc_val % 8;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_TYPE()
 *
 *****************************************************************************/
void
update_filter_type(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_type = cc_val % 4;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_GAIN()
 *
 *****************************************************************************/
void
update_filter_gain(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_gain_cc = cc_val;
    patch->filter_gain    = gain_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_FILTER_ENV_AMOUNT()
 *
 *****************************************************************************/
void
update_filter_env_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_env_amount_cc = cc_val;
    patch->filter_env_amount    = ((sample_t)int_val) * patch->filter_env_sign;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_ENV_SIGN()
 *
 *****************************************************************************/
void
update_filter_env_sign(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_env_sign_cc = cc_val;
    patch->filter_env_sign    = (((sample_t)cc_val) * 2.0) - 1.0;
    patch->filter_env_amount  = ((sample_t)patch->filter_env_amount_cc) * patch->filter_env_sign;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_ATTACK()
 *
 *****************************************************************************/
void
update_filter_attack(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_attack       = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_DECAY()
 *
 *****************************************************************************/
void
update_filter_decay(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_decay        = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_SUSTAIN()
 *
 *****************************************************************************/
void
update_filter_sustain(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_sustain_cc   = cc_val;
    patch->filter_sustain      = ((sample_t)int_val) / 127.0;
    for (j = 0; j < setting_polyphony; j++) {
	if (voice[j].cur_filter_interval == ENV_INTERVAL_SUSTAIN) {
	    voice[j].filter_env_raw = patch->filter_sustain;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_FILTER_RELEASE()
 *
 *****************************************************************************/
void
update_filter_release(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_release      = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_LFO()
 *
 *****************************************************************************/
void
update_filter_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_lfo_cc = cc_val;
    patch->filter_lfo    = (cc_val + NUM_LFOS) % (NUM_LFOS + 1);
}

/*****************************************************************************
 *
 * UPDATE_FILTER_LFO_CUTOFF()
 *
 *****************************************************************************/
void
update_filter_lfo_cutoff(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_lfo_cutoff_cc = cc_val;
    patch->filter_lfo_cutoff    = (sample_t)int_val;
}

/*****************************************************************************
 *
 * UPDATE_FILTER_LFO_RESONANCE()
 *
 *****************************************************************************/
void
update_filter_lfo_resonance(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->filter_lfo_resonance_cc = cc_val;
    patch->filter_lfo_resonance    = (sample_t)int_val * 0.0078125; // / 128
}

/*****************************************************************************
 *
 * UPDATE_AMP_ATTACK()
 *
 *****************************************************************************/
void
update_amp_attack(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->amp_attack       = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_AMP_DECAY()
 *
 *****************************************************************************/
void
update_amp_decay(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->amp_decay        = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_AMP_SUSTAIN()
 *
 *****************************************************************************/
void
update_amp_sustain(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->amp_sustain_cc   = cc_val;
    patch->amp_sustain      = ((sample_t)int_val) / 127.0;
    for (j = 0; j < setting_polyphony; j++) {
	if (voice[j].cur_amp_interval == ENV_INTERVAL_SUSTAIN) {
	    voice[j].amp_env_raw = patch->amp_sustain;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_AMP_RELEASE()
 *
 *****************************************************************************/
void
update_amp_release(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->amp_release      = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_DELAY_MIX()
 *
 *****************************************************************************/
void
update_delay_mix(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->delay_mix_cc = cc_val;
    patch->delay_mix    = mix_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_DELAY_FEED()
 *
 *****************************************************************************/
void
update_delay_feed(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->delay_feed_cc = cc_val;
    patch->delay_feed    = mix_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_DELAY_FLIP()
 *
 *****************************************************************************/
void
update_delay_flip(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->delay_flip = cc_val % 2;
}

/*****************************************************************************
 *
 * UPDATE_DELAY_TIME()
 *
 *****************************************************************************/
void
update_delay_time(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->delay_time_cc = cc_val;
    patch->delay_time    = 1.0 / get_rate_val(cc_val);
    part.delay_size      = patch->delay_time * f_sample_rate / global.bps;
    part.delay_length    = (int)(part.delay_size);
}

/*****************************************************************************
 *
 * UPDATE_DELAY_LFO()
 *
 *****************************************************************************/
void
update_delay_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->delay_lfo_cc = cc_val;
    patch->delay_lfo    = (cc_val + NUM_LFOS) % (NUM_LFOS + 1);
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_MIX()
 *
 *****************************************************************************/
void
update_chorus_mix(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_mix_cc = cc_val;
    patch->chorus_mix    = mix_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_FEED()
 *
 *****************************************************************************/
void
update_chorus_feed(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_feed_cc = cc_val;
    patch->chorus_feed    = mix_table[cc_val];
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_FLIP()
 *
 *****************************************************************************/
void
update_chorus_flip(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_flip = cc_val % 2;
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_TIME()
 *
 *****************************************************************************/
void
update_chorus_time(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_time_cc   = cc_val;
    //patch->chorus_time      = ((sample_t)(int_val)) * f_sample_rate / 12000.0;
    patch->chorus_time      = f_sample_rate / freq_table[patch->master_tune_cc][419 - int_val];
    part.chorus_len         = (int)(patch->chorus_time);
    part.chorus_size        = patch->chorus_time;
    part.chorus_delay_index = ( part.chorus_write_index + part.chorus_bufsize
				- part.chorus_len - 1 ) % part.chorus_bufsize;
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_AMOUNT()
 *
 *****************************************************************************/
void
update_chorus_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_amount_cc = cc_val;
    patch->chorus_amount    = ((sample_t)int_val) / 128.0;   
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_PHASE_RATE()
 *
 *****************************************************************************/
void
update_chorus_phase_rate(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_phase_rate_cc = cc_val;
    patch->chorus_phase_rate    = get_rate_val(cc_val);
    part.chorus_phase_freq      = global.bps * patch->chorus_phase_rate;
    part.chorus_phase_adjust    = part.chorus_phase_freq * wave_period;
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_PHASE_BALANCE()
 *
 *****************************************************************************/
void
update_chorus_phase_balance(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_phase_balance_cc = cc_val;
    patch->chorus_phase_balance    = ((sample_t)int_val) / 128.0;
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_LFO_WAVE()
 *
 *****************************************************************************/
void
update_chorus_lfo_wave(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_lfo_wave = cc_val % NUM_WAVEFORMS;
}

/*****************************************************************************
 *
 * UPDATE_CHORUS_LFO_RATE()
 *
 *****************************************************************************/
void
update_chorus_lfo_rate(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->chorus_lfo_rate_cc = cc_val;
    patch->chorus_lfo_rate    = get_rate_val(cc_val);
    part.chorus_lfo_freq      = global.bps * patch->chorus_lfo_rate;
    part.chorus_lfo_adjust    = part.chorus_lfo_freq * wave_period;
}

/*****************************************************************************
 *
 * UPDATE_OSC_MODULATION()
 *
 *****************************************************************************/
void
update_osc_modulation(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_modulation[param->index] = cc_val % 4;
    if (cc_val == MOD_TYPE_OFF) {
	for (j = 0; j < MAX_VOICES; j++) {
	    voice[j].osc_out1[param->index] = 0.0;
	    voice[j].osc_out2[param->index] = 0.0;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_WAVE()
 *
 *****************************************************************************/
void
update_osc_wave(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_wave[param->index] = cc_val % NUM_WAVEFORMS;
}

/*****************************************************************************
 *
 * UPDATE_OSC_FREQ_BASE()
 *
 *****************************************************************************/
void
update_osc_freq_base(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_freq_base[param->index] = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_OSC_RATE()
 *
 *****************************************************************************/
void
update_osc_rate(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_rate_cc[param->index] = cc_val;
    if (patch->osc_freq_base[param->index] >= FREQ_BASE_TEMPO) {
	patch->osc_rate[param->index] = get_rate_val(cc_val);
	for (j = 0; j < setting_polyphony; j++) {
	    voice[j].osc_freq[param->index] = global.bps * patch->osc_rate[param->index];
	}
    }

    /* TODO: simulate keypress */
}

/*****************************************************************************
 *
 * UPDATE_OSC_POLARITY()
 *
 *****************************************************************************/
void
update_osc_polarity(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_polarity_cc[param->index] = cc_val % 2;
}

/*****************************************************************************
 *
 * UPDATE_OSC_INIT_PHASE()
 *
 *****************************************************************************/
void
update_osc_init_phase(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_init_phase_cc[param->index] = cc_val;
    patch->osc_init_phase[param->index]    = ((sample_t)int_val) / 128.0;
    part.osc_init_index[param->index]      = patch->osc_init_phase[param->index] * F_WAVEFORM_SIZE;
}

/*****************************************************************************
 *
 * UPDATE_OSC_TRANSPOSE()
 *
 *****************************************************************************/
void
update_osc_transpose(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_transpose_cc[param->index] = cc_val;
    patch->osc_transpose[param->index]    = ((sample_t)int_val) + ((1.0 / 120.0) * ((sample_t)patch->osc_fine_tune[param->index]));
    for (j = 0; j < setting_polyphony; j++) {
	voice[j].need_portamento = 1;
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_FINE_TUNE()
 *
 *****************************************************************************/
void
update_osc_fine_tune(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_fine_tune_cc[param->index] = cc_val;
    patch->osc_fine_tune[param->index]    = (sample_t)int_val;
    patch->osc_transpose[param->index]    = ((sample_t)(patch->osc_transpose_cc[param->index] - 64)) + ((1.0 / 120.0) * ((sample_t)int_val));
    for (j = 0; j < setting_polyphony; j++) {
	voice[j].need_portamento = 1;
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_PITCHBEND()
 *
 *****************************************************************************/
void
update_osc_pitchbend(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->osc_pitchbend_cc[param->index] = cc_val;
    patch->osc_pitchbend[param->index]    = (sample_t)int_val;
}

/*****************************************************************************
 *
 * UPDATE_OSC_AM_LFO()
 *
 *****************************************************************************/
void
update_osc_am_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    if ((cc_val <= 0) || (cc_val > (NUM_LFOS + NUM_OSCS + 1))) {
	patch->am_lfo_cc[param->index] = 0;
	patch->am_lfo[param->index]    = LFO_OFF;
	part.osc_am_mod[param->index]  = MOD_OFF;
    }
    else {
	patch->am_lfo_cc[param->index] = cc_val;
	if (cc_val <= NUM_OSCS) {
	    patch->am_mod_type[param->index] = MOD_TYPE_OSC;
	    patch->am_lfo[param->index]      = LFO_OFF;
	    part.osc_am_mod[param->index]    = cc_val - 1;
	}
	else if (cc_val <= (NUM_LFOS + NUM_OSCS)) {
	    patch->am_mod_type[param->index] = MOD_TYPE_LFO;
	    patch->am_lfo[param->index]      = cc_val - NUM_OSCS - 1;
	    part.osc_am_mod[param->index]    = MOD_OFF;
	}
	else if (cc_val == NUM_LFOS + NUM_OSCS + 1) {
	    patch->am_mod_type[param->index] = MOD_TYPE_VELOCITY;
	    patch->am_lfo[param->index]      = LFO_OFF;
	    part.osc_am_mod[param->index]    = MOD_VELOCITY;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_AM_LFO_AMOUNT()
 *
 *****************************************************************************/
void
update_osc_am_lfo_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->am_lfo_amount_cc[param->index] = cc_val;
    patch->am_lfo_amount[param->index]    = ((sample_t)int_val) / 64.0;
}

/*****************************************************************************
 *
 * UPDATE_OSC_FREQ_LFO()
 *
 *****************************************************************************/
void
update_osc_freq_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    if ((cc_val <= 0) || (cc_val > (NUM_LFOS + NUM_OSCS + 1))) {
	patch->freq_lfo_cc[param->index] = 0;
	patch->freq_lfo[param->index]    = LFO_OFF;
	part.osc_freq_mod[param->index]  = MOD_OFF;
    }
    else {
	patch->freq_lfo_cc[param->index] = cc_val;
	if (cc_val <= NUM_OSCS) {
	    patch->freq_mod_type[param->index] = MOD_TYPE_OSC;
	    patch->freq_lfo[param->index]      = LFO_OFF;
	    part.osc_freq_mod[param->index]    = cc_val - 1;
	}
	else if (cc_val <= (NUM_LFOS + NUM_OSCS)) {
	    patch->freq_mod_type[param->index] = MOD_TYPE_LFO;
	    patch->freq_lfo[param->index]      = cc_val - NUM_OSCS - 1;
	    part.osc_freq_mod[param->index]    = MOD_OFF;
	}
	else if (cc_val == NUM_LFOS + NUM_OSCS + 1) {
	    patch->freq_mod_type[param->index] = MOD_TYPE_VELOCITY;
	    patch->freq_lfo[param->index]      = LFO_OFF;
	    part.osc_freq_mod[param->index]    = MOD_VELOCITY;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_FREQ_LFO_AMOUNT()
 *
 *****************************************************************************/
void
update_osc_freq_lfo_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->freq_lfo_amount_cc[param->index] = cc_val;
    patch->freq_lfo_amount[param->index]    = (((sample_t)int_val)) + ((sample_t)(patch->freq_lfo_fine[param->index]) * (1.0 / 120.0));
}

/*****************************************************************************
 *
 * UPDATE_OSC_FREQ_LFO_FINE()
 *
 *****************************************************************************/
void
update_osc_freq_lfo_fine(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->freq_lfo_fine_cc[param->index] = cc_val;
    patch->freq_lfo_fine[param->index]    = (sample_t)int_val;
    patch->freq_lfo_amount[param->index]  = ((sample_t)(patch->freq_lfo_amount_cc[param->index] - 64)) + (((sample_t)int_val) * (1.0 / 120.0));
}

/*****************************************************************************
 *
 * UPDATE_OSC_PHASE_LFO()
 *
 *****************************************************************************/
void
update_osc_phase_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    if ((cc_val <= 0) || (cc_val > (NUM_LFOS + NUM_OSCS + 1))) {
	patch->phase_lfo_cc[param->index] = 0;
	patch->phase_lfo[param->index]    = LFO_OFF;
	part.osc_phase_mod[param->index]  = MOD_OFF;
    }
    else {
	patch->phase_lfo_cc[param->index] = cc_val;
	if (cc_val <= NUM_OSCS) {
	    patch->phase_mod_type[param->index] = MOD_TYPE_OSC;
	    patch->phase_lfo[param->index]      = LFO_OFF;
	    part.osc_phase_mod[param->index]    = cc_val - 1;
	}
	else if (cc_val <= (NUM_LFOS + NUM_OSCS)) {
	    patch->phase_mod_type[param->index] = MOD_TYPE_LFO;
	    patch->phase_lfo[param->index]      = cc_val - NUM_OSCS - 1;
	    part.osc_phase_mod[param->index]    = MOD_OFF;
	}
	else if (cc_val == (NUM_LFOS + NUM_OSCS + 1)) {
	    patch->phase_mod_type[param->index] = MOD_TYPE_VELOCITY;
	    patch->phase_lfo[param->index]      = LFO_OFF;
	    part.osc_phase_mod[param->index]    = MOD_VELOCITY;
	}
    }
}

/*****************************************************************************
 *
 * UPDATE_OSC_PHASE_LFO_AMOUNT()
 *
 *****************************************************************************/
void
update_osc_phase_lfo_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->phase_lfo_amount_cc[param->index] = cc_val;
    patch->phase_lfo_amount[param->index]    = ((sample_t)int_val) / 120.0;
}

/*****************************************************************************
 *
 * UPDATE_OSC_WAVE_LFO()
 *
 *****************************************************************************/
void
update_osc_wave_lfo(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->wave_lfo_cc[param->index] = cc_val;
    patch->wave_lfo[param->index]    = (cc_val + NUM_LFOS) % (NUM_LFOS + 1);
}

/*****************************************************************************
 *
 * UPDATE_OSC_WAVE_LFO_AMOUNT()
 *
 *****************************************************************************/
void
update_osc_wave_lfo_amount(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->wave_lfo_amount_cc[param->index] = cc_val;
    patch->wave_lfo_amount[param->index]    = (sample_t)int_val;
}

/*****************************************************************************
 *
 * UPDATE_LFO_WAVE()
 *
 *****************************************************************************/
void
update_lfo_wave(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_wave[param->index] = cc_val % NUM_WAVEFORMS;
}

/*****************************************************************************
 *
 * UPDATE_LFO_FREQ_BASE()
 *
 *****************************************************************************/
void
update_lfo_freq_base(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_freq_base[param->index] = cc_val;
}

/*****************************************************************************
 *
 * UPDATE_LFO_RATE()
 *
 *****************************************************************************/
void
update_lfo_rate(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_rate_cc[param->index] = cc_val;
    if (patch->lfo_freq_base[param->index] >= FREQ_BASE_TEMPO) {
	patch->lfo_rate[param->index] = get_rate_val(cc_val);
	part.lfo_freq[param->index]   = global.bps * patch->lfo_rate[param->index];
	part.lfo_adjust[param->index] = part.lfo_freq[param->index] * wave_period;
    }
}

/*****************************************************************************
 *
 * UPDATE_LFO_POLARITY()
 *
 *****************************************************************************/
void
update_lfo_polarity(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_polarity_cc[param->index] = cc_val % 2;
}

/*****************************************************************************
 *
 * UPDATE_LFO_INIT_PHASE()
 *
 *****************************************************************************/
void
update_lfo_init_phase(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_init_phase_cc[param->index] = cc_val;
    patch->lfo_init_phase[param->index]    = ((sample_t)int_val) / 128.0;
    part.lfo_init_index[param->index]      = patch->lfo_init_phase[param->index] * F_WAVEFORM_SIZE;
}

/*****************************************************************************
 *
 * UPDATE_LFO_TRANSPOSE()
 *
 *****************************************************************************/
void
update_lfo_transpose(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    int		j;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_transpose_cc[param->index] = cc_val;
    patch->lfo_transpose[param->index]    = (sample_t)int_val;
    for (j = 0; j < setting_polyphony; j++) {
	voice[j].need_portamento = 1;
    }
}

/*****************************************************************************
 *
 * UPDATE_LFO_PITCHBEND()
 *
 *****************************************************************************/
void
update_lfo_pitchbend(GtkWidget *widget, gpointer *data) {
    int		cc_val;
    int		int_val;
    PARAM	*param = (PARAM *)data;

    /* use the generic updater first */
    if (update_param_generic (widget, param) == PARAM_VAL_IGNORE) {
	return;
    }
    cc_val  = param->cc_val[program_number];
    int_val = param->int_val[program_number];

    patch->lfo_pitchbend_cc[param->index] = cc_val;
    patch->lfo_pitchbend[param->index]    = (sample_t)int_val;
}
