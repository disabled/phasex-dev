/*****************************************************************************
 *
 * param.c
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
#include <string.h>
#include <errno.h>
#include "phasex.h"
#include "settings.h"
#include "config.h"
#include "param.h"
#include "patch.h"
#include "callback.h"
#include "midi.h"


PARAM		param[NUM_HELP_PARAMS];
PARAM_GROUP	param_group[NUM_PARAM_GROUPS];
PARAM_PAGE	param_page[NUM_PARAM_PAGES];

char		*midimap_filename	= NULL;
int		midimap_modified	= 0;


/*****************************************************************************
 * all value label lists needed for GUI
 *****************************************************************************/
const char *midi_ch_labels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "Omni",
    NULL
};

const char *on_off_labels[] = {
    "Off",
    "On",
    NULL
};

const char *freq_base_labels[] = {
    "MIDI Key",
    "Input 1",
    "Input 2",
    "Input 1&2",
    "Amp Env",
    "Filter Env",
    "Velocity",
    "Tempo",
    "Tempo+Trig",
    NULL
};

const char *mod_type_labels[] = {
    "<small>Off</small>",
    "<small>Mix</small>",
    "<small>AM</small>",
    "<small>Mod</small>",
    NULL
};

const char *keymode_labels[] = {
    "MonoSmooth",
    "MonoReTrig",
    "MonoMulti",
    "Poly",
    NULL
};

const char *keyfollow_labels[] = {
    "Off",
    "Newest",
    "Highest",
    "Lowest",
    "KeyTrig",
    NULL
};

const char *polarity_labels[] = {
    "[-1,1]",
    "[0,1]",
    NULL
};

const char *sign_labels[] = {
    "[-]",
    "[+]",
    NULL
};

const char *wave_labels[] = {
    "Sine",
    "Triangle",
    "Saw",
    "RevSaw",
    "Square",
    "Stair",
    "BL Saw",
    "BL RevSaw",
    "BL Square",
    "BL Stair",
    "Juno Osc",
    "Juno Saw",
    "Juno Sq",
    "Juno Poly",
    "Vox 1",
    "Vox 2",
    "Null",
    "Identity",
    NULL
};

const char *filter_mode_labels[] = {
    "LP",
    "HP",
    "BP",
    "BS",
    "LP+BP",
    "HP+BP",
    "LP+HP",
    "BS+BP",
    NULL
};

const char *filter_type_labels[] = {
    "Dist",
    "Retro",
    NULL
};

const char *lfo_labels[] = {
    "<small>Off</small>",
    "<small>1</small>",
    "<small>2</small>",
    "<small>3</small>",
    "<small>4</small>",
    NULL
};

const char *velo_lfo_labels[] = {
    "<small>Velo</small>",
    "<small>1</small>",
    "<small>2</small>",
    "<small>3</small>",
    "<small>4</small>",
    NULL
};

const char *mod_labels[] = {
    "Off",
    "Osc-1",
    "Osc-2",
    "Osc-3",
    "Osc-4",
    "LFO-1",
    "LFO-2",
    "LFO-3",
    "LFO-4",
    "Velocity",
    NULL
};

const char *rate_labels[] = {
    "1/128",
    "1/64",
    "1/32",
    "3/64",
    "1/16",
    "5/64",
    "3/32",
    "7/64",
    "1/8",
    "9/64",
    "5/32",
    "11/64",
    "3/16",
    "13/64",
    "7/32",
    "15/64",
    "1/4",
    "17/64",
    "9/32",
    "19/64",
    "5/16",
    "21/64",
    "11/32",
    "23/64",
    "3/8",
    "25/64",
    "13/32",
    "27/64",
    "7/16",
    "29/64",
    "15/32",
    "31/64",
    "1/2",
    "33/64",
    "17/32",
    "35/64",
    "9/16",
    "37/64",
    "19/32",
    "39/64",
    "5/8",
    "41/64",
    "21/32",
    "43/64",
    "11/16",
    "45/64",
    "23/32",
    "47/64",
    "3/4",
    "49/64",
    "25/32",
    "51/64",
    "13/16",
    "53/64",
    "27/32",
    "55/64",
    "7/8",
    "57/64",
    "29/32",
    "59/64",
    "15/16",
    "61/64",
    "31/32",
    "63/64",
    "1/1",
    "1/48",
    "1/24",
    "1/16",
    "1/12",
    "5/48",
    "1/8",
    "7/48",
    "1/6",
    "3/16",
    "5/24",
    "11/48",
    "1/4",
    "13/48",
    "7/24",
    "5/16",
    "1/3",
    "17/48",
    "3/8",
    "19/48",
    "5/12",
    "7/16",
    "11/24",
    "23/48",
    "1/2",
    "25/48",
    "13/24",
    "9/16",
    "7/12",
    "29/48",
    "5/8",
    "31/48",
    "2/3",
    "11/16",
    "17/24",
    "35/48",
    "3/4",
    "37/48",
    "19/24",
    "13/16",
    "5/6",
    "41/48",
    "7/8",
    "43/48",
    "11/12",
    "15/16",
    "23/24",
    "47/48",
    "1/1",
    "2/1",
    "3/1",
    "4/1",
    "5/1",
    "6/1",
    "7/1",
    "8/1",
    "9/1",
    "10/1",
    "11/1",
    "12/1",
    "13/1",
    "14/1",
    "15/1",
    "16/1",
    NULL
};


/*****************************************************************************
 *
 * INIT_PARAM()
 *
 * Initialize a single parameter.
 *
 *****************************************************************************/
void
init_param(int id, const char *name, const char *label, int type,
	   int cc, int lim, int ccv, int pre, int ix, int leap,
	   PHASEX_CALLBACK callback, const char *label_list[]) {
    int			j;

    param[id].id  		= id;
    param[id].name		= name;
    param[id].label		= label;
    param[id].type		= type;
    param[id].cc_num		= cc;
    param[id].cc_limit		= lim;
    param[id].pre_inc		= pre;
    param[id].index		= ix;
    param[id].leap		= leap;
    param[id].callback		= callback;
    param[id].adj		= NULL;
    param[id].knob		= NULL;
    param[id].spin		= NULL;
    param[id].combo		= NULL;
    param[id].text		= NULL;
    param[id].list_labels	= label_list;
    param[id].updated		= 0;
    param[id].locked		= 0;
    param[id].cc_default	= ccv;
    param[id].cc_prev		= ccv;

    for (j = 0; j < PATCH_BANK_SIZE; j++) {
	param[id].cc_val[j]	= ccv;
	param[id].int_val[j]	= ccv + pre;
    }

    for (j = 0; j < 12; j++) {
	param[id].button[j]	= NULL;
    }
}


/*****************************************************************************
 *
 * INIT_PARAMS()
 *
 * Map out all parameters.
 * This table is a 1:1 control mapping.
 *
 *****************************************************************************/
void
init_params(void) {
    /*         index                        name                   label          type             cc  lim  ccv  pre ix leap callback                  labels */
    init_param(PARAM_BPM,                  "bpm",                 "BPM",         PARAM_TYPE_INT,  -1, 127,  64,  64, 0,  8,update_bpm,                 NULL);
    init_param(PARAM_MASTER_TUNE,          "master_tune",         "Master Tune", PARAM_TYPE_INT,  -1, 127,  64, -64, 0, 12,update_master_tune,         NULL);
    init_param(PARAM_KEYMODE,              "keymode",             "KeyMode",     PARAM_TYPE_DTNT, -1,   3,   3,   0, 0,  1,update_keymode,             keymode_labels);
    init_param(PARAM_KEYFOLLOW_VOL,        "keyfollow_vol",       "VolKeyFollow",PARAM_TYPE_INT,  46, 127,  64, -64, 0,  8,update_keyfollow_vol,       NULL);
    init_param(PARAM_TRANSPOSE,            "transpose",           "Transpose",   PARAM_TYPE_INT,  93, 127,  64, -64, 0, 12,update_transpose,           NULL);
    init_param(PARAM_PORTAMENTO,           "portamento",          "Portamento",  PARAM_TYPE_INT,   5, 127,   0,   0, 0,  8,update_portamento,          NULL);
    init_param(PARAM_INPUT_BOOST,          "input_boost",         "Input Boost", PARAM_TYPE_REAL, -1, 127,   0,   0, 0,  8,update_input_boost,         NULL);
    init_param(PARAM_INPUT_FOLLOW,         "input_follow",        "Env Follower",PARAM_TYPE_BOOL, -1,   1,   0,   0, 0,  1,update_input_follow,        on_off_labels);
    init_param(PARAM_PAN,                  "pan",                 "Pan",         PARAM_TYPE_REAL, -1, 127,  64, -64, 0,  8,update_pan,                 NULL);
    init_param(PARAM_STEREO_WIDTH,         "stereo_width",        "Stereo Width",PARAM_TYPE_REAL, 33, 127, 127,   0, 0,  8,update_stereo_width,        NULL);
    init_param(PARAM_AMP_VELOCITY,         "amp_velocity",        "Velocity",    PARAM_TYPE_INT,  -1, 127,   0,   0, 0,  8,update_amp_velocity,        NULL);
    init_param(PARAM_AMP_ATTACK,           "amp_attack",          "Attack",      PARAM_TYPE_INT,  59, 127,  16,   0, 0,  8,update_amp_attack,          NULL);
    init_param(PARAM_AMP_DECAY,            "amp_decay",           "Decay",       PARAM_TYPE_INT,  60, 127,  32,   0, 0,  8,update_amp_decay,           NULL);
    init_param(PARAM_AMP_SUSTAIN,          "amp_sustain",         "Sustain",     PARAM_TYPE_REAL, 61, 127,  32,   0, 0,  8,update_amp_sustain,         NULL);
    init_param(PARAM_AMP_RELEASE,          "amp_release",         "Release",     PARAM_TYPE_INT,  63, 127,  32,   0, 0,  8,update_amp_release,         NULL);
    init_param(PARAM_VOLUME,               "volume",              "Patch Vol",   PARAM_TYPE_REAL, 91, 127,   0,   0, 0,  8,update_volume,              NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_FILTER_CUTOFF,        "filter_cutoff",       "Cutoff",      PARAM_TYPE_REAL, 40, 127, 127,   0, 0, 12,update_filter_cutoff,       NULL);
    init_param(PARAM_FILTER_RESONANCE,     "filter_resonance",    "Resonance",   PARAM_TYPE_INT,  42, 127,  32,   0, 0,  8,update_filter_resonance,    NULL);
    init_param(PARAM_FILTER_SMOOTHING,     "filter_smoothing",    "Smoothing",   PARAM_TYPE_INT,  -1, 127,   0,   0, 0,  8,update_filter_smoothing,    NULL);
    init_param(PARAM_FILTER_KEYFOLLOW,     "filter_keyfollow",    "KeyFollow",   PARAM_TYPE_DTNT, -1,   4,   0,   0, 0,  1,update_filter_keyfollow,    keyfollow_labels);
    init_param(PARAM_FILTER_MODE,          "filter_mode",         "Mode",        PARAM_TYPE_DTNT, -1,   7,   0,   0, 0,  1,update_filter_mode,         filter_mode_labels);
    init_param(PARAM_FILTER_TYPE,          "filter_type",         "Type",        PARAM_TYPE_BOOL, 53,   3,   0,   0, 0,  1,update_filter_type,         filter_type_labels);
    init_param(PARAM_FILTER_GAIN,          "filter_gain",         "Gain",        PARAM_TYPE_REAL, 36, 127,  64,   0, 0,  8,update_filter_gain,         NULL);
    init_param(PARAM_FILTER_ENV_AMOUNT,    "filter_env_amount",   "Env Amt",     PARAM_TYPE_REAL, 44, 127,  12,   0, 0, 12,update_filter_env_amount,   NULL);
    init_param(PARAM_FILTER_ENV_SIGN,      "filter_env_sign",     "Env Sign",    PARAM_TYPE_BOOL, -1,   1,   1,   0, 0,  1,update_filter_env_sign,     sign_labels);
    init_param(PARAM_FILTER_LFO,           "filter_lfo",          "LFO",         PARAM_TYPE_BBOX, -1,   4,   0,   0, 0,  1,update_filter_lfo,          velo_lfo_labels);
    init_param(PARAM_FILTER_LFO_CUTOFF,    "filter_lfo_cutoff",   "Cutoff",      PARAM_TYPE_REAL, 41, 127,  64, -64, 0, 12,update_filter_lfo_cutoff,   NULL);
    init_param(PARAM_FILTER_LFO_RESONANCE, "filter_lfo_resonance","Resonance",   PARAM_TYPE_REAL, -1, 127,  64, -64, 0,  8,update_filter_lfo_resonance,NULL);
    init_param(PARAM_FILTER_ATTACK,        "filter_attack",       "Attack",      PARAM_TYPE_INT,  54, 127,   0,   0, 0,  8,update_filter_attack,       NULL);
    init_param(PARAM_FILTER_DECAY,         "filter_decay",        "Decay",       PARAM_TYPE_INT,  55, 127,  48,   0, 0,  8,update_filter_decay,        NULL);
    init_param(PARAM_FILTER_SUSTAIN,       "filter_sustain",      "Sustain",     PARAM_TYPE_REAL, 56, 127,   0,   0, 0,  8,update_filter_sustain,      NULL);
    init_param(PARAM_FILTER_RELEASE,       "filter_release",      "Release",     PARAM_TYPE_INT,  58, 127,  32,   0, 0,  8,update_filter_release,      NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_CHORUS_MIX,           "chorus_mix",          "Mix",         PARAM_TYPE_REAL,105, 127,  32,   0, 0,  8,update_chorus_mix,          NULL);
    init_param(PARAM_CHORUS_AMOUNT,        "chorus_amount",       "Depth",       PARAM_TYPE_REAL, -1, 127,  32,   0, 0,  8,update_chorus_amount,       NULL);
    init_param(PARAM_CHORUS_TIME,          "chorus_time",         "Delay Time",  PARAM_TYPE_INT, 108, 127,   0,   0, 0, 12,update_chorus_time,         NULL);
    init_param(PARAM_CHORUS_PHASE_RATE,    "chorus_phase_rate",   "Phase Rate",  PARAM_TYPE_RATE, -1, 127,   0,   0, 0,  4,update_chorus_phase_rate,   rate_labels);
    init_param(PARAM_CHORUS_PHASE_BALANCE, "chorus_phase_balance","Phase Bal",   PARAM_TYPE_REAL, -1, 127,  64,   0, 0,  8,update_chorus_phase_balance,NULL);
    init_param(PARAM_CHORUS_FEED,          "chorus_feed",         "Feedback Mix",    PARAM_TYPE_REAL,109, 127,   0,   0, 0,  8,update_chorus_feed,         NULL);
    init_param(PARAM_CHORUS_LFO_WAVE,      "chorus_lfo_wave",     "LFO Wave",    PARAM_TYPE_DTNT,110,  17,   0,   0, 0,  1,update_chorus_lfo_wave,     wave_labels);
    init_param(PARAM_CHORUS_LFO_RATE,      "chorus_lfo_rate",     "LFO Rate",    PARAM_TYPE_RATE,106, 127,   0,   0, 0,  4,update_chorus_lfo_rate,     rate_labels);
    init_param(PARAM_CHORUS_FLIP,          "chorus_flip",         "Crossover",   PARAM_TYPE_BOOL, -1,   1,   0,   0, 0,  1,update_chorus_flip,         on_off_labels);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_DELAY_MIX,            "delay_mix",           "Mix",         PARAM_TYPE_REAL,113, 127,  16,   0, 0,  8,update_delay_mix,           NULL);
    init_param(PARAM_DELAY_FEED,           "delay_feed",          "Feedback Mix",    PARAM_TYPE_REAL,115, 127,  16,   0, 0,  8,update_delay_feed,          NULL);
    init_param(PARAM_DELAY_FLIP,           "delay_flip",          "Crossover",   PARAM_TYPE_BOOL, -1, 127,   0,   0, 0,  1,update_delay_flip,          on_off_labels);
    init_param(PARAM_DELAY_TIME,           "delay_time",          "Time",        PARAM_TYPE_RATE, 62, 111,  64,   0, 0,  4,update_delay_time,          rate_labels);
    init_param(PARAM_DELAY_LFO,            "delay_lfo",           "LFO",         PARAM_TYPE_BBOX, -1, 127,   0,   0, 0,  1,update_delay_lfo,           lfo_labels);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_OSC1_MODULATION,      "osc1_modulation",     "Mix Mod",     PARAM_TYPE_BBOX, -1,   3,   1,   0, 0,  1,update_osc_modulation,      mod_type_labels);
    init_param(PARAM_OSC1_WAVE,            "osc1_wave",           "Wave",        PARAM_TYPE_DTNT, 17,  17,   0,   0, 0,  1,update_osc_wave,            wave_labels);
    init_param(PARAM_OSC1_FREQ_BASE,       "osc1_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 0,  1,update_osc_freq_base,       freq_base_labels);
    init_param(PARAM_OSC1_RATE,            "osc1_rate",           "Rate",        PARAM_TYPE_RATE, -1, 127,   0,   0, 0,  4,update_osc_rate,            rate_labels);
    init_param(PARAM_OSC1_POLARITY,        "osc1_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 0,  1,update_osc_polarity,        polarity_labels);
    init_param(PARAM_OSC1_INIT_PHASE,      "osc1_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 0, 16,update_osc_init_phase,      NULL);
    init_param(PARAM_OSC1_TRANSPOSE,       "osc1_transpose",      "Transpose",   PARAM_TYPE_INT,  20, 127,  64, -64, 0, 12,update_osc_transpose,       NULL);
    init_param(PARAM_OSC1_FINE_TUNE,       "osc1_fine_tune",      "Fine Tune",   PARAM_TYPE_INT,  -1, 127,  64, -64, 0,  8,update_osc_fine_tune,       NULL);
    init_param(PARAM_OSC1_PITCHBEND,       "osc1_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 0, 12,update_osc_pitchbend,       NULL);
    init_param(PARAM_OSC1_AM_LFO,          "osc1_am_mod",         "AM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 0,  1,update_osc_am_lfo,          mod_labels);
    init_param(PARAM_OSC1_AM_LFO_AMOUNT,   "osc1_am_mod_amount",  "AM Amt",      PARAM_TYPE_REAL, 27, 127,  64, -64, 0,  8,update_osc_am_lfo_amount,   NULL);
    init_param(PARAM_OSC1_FREQ_LFO,        "osc1_fm_mod",         "FM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 0,  1,update_osc_freq_lfo,        mod_labels);
    init_param(PARAM_OSC1_FREQ_LFO_AMOUNT, "osc1_fm_mod_amount",  "FM Amt",      PARAM_TYPE_REAL, 27, 127,  64, -64, 0, 12,update_osc_freq_lfo_amount, NULL);
    init_param(PARAM_OSC1_FREQ_LFO_FINE,   "osc1_fm_mod_fine",    "FM Fine",     PARAM_TYPE_REAL, -1, 127,  64, -64, 0,  8,update_osc_freq_lfo_fine,   NULL);
    init_param(PARAM_OSC1_PHASE_LFO,       "osc1_pm_mod",         "PM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 0,  1,update_osc_phase_lfo,       mod_labels);
    init_param(PARAM_OSC1_PHASE_LFO_AMOUNT,"osc1_pm_mod_amount",  "PM Amt",      PARAM_TYPE_REAL, 34, 127,  64, -64, 0, 15,update_osc_phase_lfo_amount,NULL);
    init_param(PARAM_OSC1_WAVE_LFO,        "osc1_wave_lfo",       "Wave LFO",    PARAM_TYPE_BBOX, -1,   4,   4,   0, 0,  1,update_osc_wave_lfo,        lfo_labels);
    init_param(PARAM_OSC1_WAVE_LFO_AMOUNT, "osc1_wave_lfo_amount","Wave Amt",    PARAM_TYPE_REAL, -1, 127,  64, -64, 0,  1,update_osc_wave_lfo_amount, NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_OSC2_MODULATION,      "osc2_modulation",     "Mix Mod",     PARAM_TYPE_BBOX, -1,   3,   1,   0, 1,  1,update_osc_modulation,      mod_type_labels);
    init_param(PARAM_OSC2_WAVE,            "osc2_wave",           "Wave",        PARAM_TYPE_DTNT, 22,  17,   0,   0, 1,  1,update_osc_wave,            wave_labels);
    init_param(PARAM_OSC2_FREQ_BASE,       "osc2_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 1,  1,update_osc_freq_base,       freq_base_labels);
    init_param(PARAM_OSC2_RATE,            "osc2_rate",           "Rate",        PARAM_TYPE_RATE, -1, 127,   0,   0, 1,  4,update_osc_rate,            rate_labels);
    init_param(PARAM_OSC2_POLARITY,        "osc2_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 1,  1,update_osc_polarity,        polarity_labels);
    init_param(PARAM_OSC2_INIT_PHASE,      "osc2_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 1, 16,update_osc_init_phase,      NULL);
    init_param(PARAM_OSC2_TRANSPOSE,       "osc2_transpose",      "Transpose",   PARAM_TYPE_INT,  25, 127,  64, -64, 1, 12,update_osc_transpose,       NULL);
    init_param(PARAM_OSC2_FINE_TUNE,       "osc2_fine_tune",      "Fine Tune",   PARAM_TYPE_INT,  -1, 127,  64, -64, 1,  8,update_osc_fine_tune,       NULL);
    init_param(PARAM_OSC2_PITCHBEND,       "osc2_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 1, 12,update_osc_pitchbend,       NULL);
    init_param(PARAM_OSC2_AM_LFO,          "osc2_am_mod",         "AM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 1,  1,update_osc_am_lfo,          mod_labels);
    init_param(PARAM_OSC2_AM_LFO_AMOUNT,   "osc2_am_mod_amount",  "AM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 1,  8,update_osc_am_lfo_amount,   NULL);
    init_param(PARAM_OSC2_FREQ_LFO,        "osc2_fm_mod",         "FM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 1,  1,update_osc_freq_lfo,        mod_labels);
    init_param(PARAM_OSC2_FREQ_LFO_AMOUNT, "osc2_fm_mod_amount",  "FM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 1, 12,update_osc_freq_lfo_amount, NULL);
    init_param(PARAM_OSC2_FREQ_LFO_FINE,   "osc2_fm_mod_fine",    "FM Fine",     PARAM_TYPE_REAL, -1, 127,  64, -64, 1,  8,update_osc_freq_lfo_fine,   NULL);
    init_param(PARAM_OSC2_PHASE_LFO,       "osc2_pm_mod",         "PM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 1,  1,update_osc_phase_lfo,       mod_labels);
    init_param(PARAM_OSC2_PHASE_LFO_AMOUNT,"osc2_pm_mod_amount",  "PM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 1, 15,update_osc_phase_lfo_amount,NULL);
    init_param(PARAM_OSC2_WAVE_LFO,        "osc2_wave_lfo",       "Wave LFO",    PARAM_TYPE_BBOX, -1,   4,   4,   0, 1,  1,update_osc_wave_lfo,        lfo_labels);
    init_param(PARAM_OSC2_WAVE_LFO_AMOUNT, "osc2_wave_lfo_amount","Wave Amt",    PARAM_TYPE_REAL, -1, 127,  64, -64, 1,  1,update_osc_wave_lfo_amount, NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_OSC3_MODULATION,      "osc3_modulation",     "Mix Mod",     PARAM_TYPE_BBOX, -1,   3,   1,   0, 2,  1,update_osc_modulation,      mod_type_labels);
    init_param(PARAM_OSC3_WAVE,            "osc3_wave",           "Wave",        PARAM_TYPE_DTNT, 19,  17,   0,   0, 2,  1,update_osc_wave,            wave_labels);
    init_param(PARAM_OSC3_FREQ_BASE,       "osc3_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 2,  1,update_osc_freq_base,       freq_base_labels);
    init_param(PARAM_OSC3_RATE,            "osc3_rate",           "Rate",        PARAM_TYPE_RATE, -1, 127,   0,   0, 2,  4,update_osc_rate,            rate_labels);
    init_param(PARAM_OSC3_POLARITY,        "osc3_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 2,  1,update_osc_polarity,        polarity_labels);
    init_param(PARAM_OSC3_INIT_PHASE,      "osc3_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 2, 16,update_osc_init_phase,      NULL);
    init_param(PARAM_OSC3_TRANSPOSE,       "osc3_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 2, 12,update_osc_transpose,       NULL);
    init_param(PARAM_OSC3_FINE_TUNE,       "osc3_fine_tune",      "Fine Tune",   PARAM_TYPE_INT,  -1, 127,  64, -64, 2,  8,update_osc_fine_tune,       NULL);
    init_param(PARAM_OSC3_PITCHBEND,       "osc3_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 2, 12,update_osc_pitchbend,       NULL);
    init_param(PARAM_OSC3_AM_LFO,          "osc3_am_mod",         "AM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 2,  1,update_osc_am_lfo,          mod_labels);
    init_param(PARAM_OSC3_AM_LFO_AMOUNT,   "osc3_am_mod_amount",  "AM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 2,  8,update_osc_am_lfo_amount,   NULL);
    init_param(PARAM_OSC3_FREQ_LFO,        "osc3_fm_mod",         "FM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 2,  1,update_osc_freq_lfo,        mod_labels);
    init_param(PARAM_OSC3_FREQ_LFO_AMOUNT, "osc3_fm_mod_amount",  "FM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 2, 12,update_osc_freq_lfo_amount, NULL);
    init_param(PARAM_OSC3_FREQ_LFO_FINE,   "osc3_fm_mod_fine",    "FM Fine",     PARAM_TYPE_REAL, -1, 127,  64, -64, 2,  8,update_osc_freq_lfo_fine,   NULL);
    init_param(PARAM_OSC3_PHASE_LFO,       "osc3_pm_mod",         "PM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 2,  1,update_osc_phase_lfo,       mod_labels);
    init_param(PARAM_OSC3_PHASE_LFO_AMOUNT,"osc3_pm_mod_amount",  "PM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 2, 15,update_osc_phase_lfo_amount,NULL);
    init_param(PARAM_OSC3_WAVE_LFO,        "osc3_wave_lfo",       "Wave LFO",    PARAM_TYPE_BBOX, -1,   4,   4,   0, 2,  1,update_osc_wave_lfo,        lfo_labels);
    init_param(PARAM_OSC3_WAVE_LFO_AMOUNT, "osc3_wave_lfo_amount","Wave Amt",    PARAM_TYPE_REAL, -1, 127,  64, -64, 2,  1,update_osc_wave_lfo_amount, NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_OSC4_MODULATION,      "osc4_modulation",     "Mix Mod",     PARAM_TYPE_BBOX, -1,   3,   1,   0, 3,  1,update_osc_modulation,      mod_type_labels);
    init_param(PARAM_OSC4_WAVE,            "osc4_wave",           "Wave",        PARAM_TYPE_DTNT, 24,  17,   0,   0, 3,  1,update_osc_wave,            wave_labels);
    init_param(PARAM_OSC4_FREQ_BASE,       "osc4_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 3,  1,update_osc_freq_base,       freq_base_labels);
    init_param(PARAM_OSC4_RATE,            "osc4_rate",           "Rate",        PARAM_TYPE_RATE, -1, 127,   0,   0, 3,  4,update_osc_rate,            rate_labels);
    init_param(PARAM_OSC4_POLARITY,        "osc4_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 3,  1,update_osc_polarity,        polarity_labels);
    init_param(PARAM_OSC4_INIT_PHASE,      "osc4_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 3, 16,update_osc_init_phase,      NULL);
    init_param(PARAM_OSC4_TRANSPOSE,       "osc4_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 3, 12,update_osc_transpose,       NULL);
    init_param(PARAM_OSC4_FINE_TUNE,       "osc4_fine_tune",      "Fine Tune",   PARAM_TYPE_INT,  -1, 127,  64, -64, 3,  8,update_osc_fine_tune,       NULL);
    init_param(PARAM_OSC4_PITCHBEND,       "osc4_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 3, 12,update_osc_pitchbend,       NULL);
    init_param(PARAM_OSC4_AM_LFO,          "osc4_am_mod",         "AM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 3,  1,update_osc_am_lfo,          mod_labels);
    init_param(PARAM_OSC4_AM_LFO_AMOUNT,   "osc4_am_mod_amount",  "AM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 3,  8,update_osc_am_lfo_amount,   NULL);
    init_param(PARAM_OSC4_FREQ_LFO,        "osc4_fm_mod",         "FM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 3,  1,update_osc_freq_lfo,        mod_labels);
    init_param(PARAM_OSC4_FREQ_LFO_AMOUNT, "osc4_fm_mod_amount",  "FM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 3, 12,update_osc_freq_lfo_amount, NULL);
    init_param(PARAM_OSC4_FREQ_LFO_FINE,   "osc4_fm_mod_fine",    "FM Fine",     PARAM_TYPE_REAL, -1, 127,  64, -64, 3,  8,update_osc_freq_lfo_fine,   NULL);
    init_param(PARAM_OSC4_PHASE_LFO,       "osc4_pm_mod",         "PM Mod",      PARAM_TYPE_DTNT, -1,   9,   0,   0, 3,  1,update_osc_phase_lfo,       mod_labels);
    init_param(PARAM_OSC4_PHASE_LFO_AMOUNT,"osc4_pm_mod_amount",  "PM Amt",      PARAM_TYPE_REAL, -1, 127,  64, -64, 3, 15,update_osc_phase_lfo_amount,NULL);
    init_param(PARAM_OSC4_WAVE_LFO,        "osc4_wave_lfo",       "Wave LFO",    PARAM_TYPE_BBOX, -1,   4,   4,   0, 3,  1,update_osc_wave_lfo,        lfo_labels);
    init_param(PARAM_OSC4_WAVE_LFO_AMOUNT, "osc4_wave_lfo_amount","Wave Amt",    PARAM_TYPE_REAL, -1, 127,  64, -64, 3,  1,update_osc_wave_lfo_amount, NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_LFO1_WAVE,            "lfo1_wave",           "Wave",        PARAM_TYPE_DTNT, -1,  17,   0,   0, 0,  1,update_lfo_wave,            wave_labels);
    init_param(PARAM_LFO1_FREQ_BASE,       "lfo1_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 0,  1,update_lfo_freq_base,       freq_base_labels);
    init_param(PARAM_LFO1_RATE,            "lfo1_rate",           "Rate",        PARAM_TYPE_RATE, 67, 127,   0,   0, 0,  4,update_lfo_rate,            rate_labels);
    init_param(PARAM_LFO1_POLARITY,        "lfo1_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 0,  1,update_lfo_polarity,        polarity_labels);
    init_param(PARAM_LFO1_INIT_PHASE,      "lfo1_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 0, 16,update_lfo_init_phase,      NULL);
    init_param(PARAM_LFO1_TRANSPOSE,       "lfo1_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 0, 12,update_lfo_transpose,       NULL);
    init_param(PARAM_LFO1_PITCHBEND,       "lfo1_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 0, 12,update_lfo_pitchbend,       NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_LFO2_WAVE,            "lfo2_wave",           "Wave",        PARAM_TYPE_DTNT, -1,  17,   0,   0, 1,  1,update_lfo_wave,            wave_labels);
    init_param(PARAM_LFO2_FREQ_BASE,       "lfo2_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 1,  1,update_lfo_freq_base,       freq_base_labels);
    init_param(PARAM_LFO2_RATE,            "lfo2_rate",           "Rate",        PARAM_TYPE_RATE, 79, 127,   0,   0, 1,  4,update_lfo_rate,            rate_labels);
    init_param(PARAM_LFO2_POLARITY,        "lfo2_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 1,  1,update_lfo_polarity,        polarity_labels);
    init_param(PARAM_LFO2_INIT_PHASE,      "lfo2_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 1, 16,update_lfo_init_phase,      NULL);
    init_param(PARAM_LFO2_TRANSPOSE,       "lfo2_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 1, 12,update_lfo_transpose,       NULL);
    init_param(PARAM_LFO2_PITCHBEND,       "lfo2_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 1, 12,update_lfo_pitchbend,       NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_LFO3_WAVE,            "lfo3_wave",           "Wave",        PARAM_TYPE_DTNT, -1,  17,   0,   0, 2,  1,update_lfo_wave,            wave_labels);
    init_param(PARAM_LFO3_FREQ_BASE,       "lfo3_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 2,  1,update_lfo_freq_base,       freq_base_labels);
    init_param(PARAM_LFO3_RATE,            "lfo3_rate",           "Rate",        PARAM_TYPE_RATE, -1, 127,   0,   0, 2,  4,update_lfo_rate,            rate_labels);
    init_param(PARAM_LFO3_POLARITY,        "lfo3_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 2,  1,update_lfo_polarity,        polarity_labels);
    init_param(PARAM_LFO3_INIT_PHASE,      "lfo3_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 2, 16,update_lfo_init_phase,      NULL);
    init_param(PARAM_LFO3_TRANSPOSE,       "lfo3_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 2, 12,update_lfo_transpose,       NULL);
    init_param(PARAM_LFO3_PITCHBEND,       "lfo3_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 2, 12,update_lfo_pitchbend,       NULL);
    /*         index                        name                   label         type             cc  lim  ccv  pre ix leap callback                   labels */
    init_param(PARAM_LFO4_WAVE,            "lfo4_wave",           "Wave",        PARAM_TYPE_DTNT, -1,  17,   0,   0, 3,  1,update_lfo_wave,            wave_labels);
    init_param(PARAM_LFO4_FREQ_BASE,       "lfo4_source",         "Source",      PARAM_TYPE_DTNT, -1,   8,   0,   0, 3,  1,update_lfo_freq_base,       freq_base_labels);
    init_param(PARAM_LFO4_RATE,            "lfo4_rate",           "Rate",        PARAM_TYPE_RATE, 57, 127,   0,   0, 3,  4,update_lfo_rate,            rate_labels);
    init_param(PARAM_LFO4_POLARITY,        "lfo4_polarity",       "Polarity",    PARAM_TYPE_BOOL, -1,   1,   0,   0, 3,  1,update_lfo_polarity,        polarity_labels);
    init_param(PARAM_LFO4_INIT_PHASE,      "lfo4_init_phase",     "Init Phase",  PARAM_TYPE_REAL, -1, 127,   0,   0, 3, 16,update_lfo_init_phase,      NULL);
    init_param(PARAM_LFO4_TRANSPOSE,       "lfo4_transpose",      "Transpose",   PARAM_TYPE_INT,  -1, 127,  64, -64, 3, 12,update_lfo_transpose,       NULL);
    init_param(PARAM_LFO4_PITCHBEND,       "lfo4_pitchbend",      "Pitchbend",   PARAM_TYPE_REAL, -1, 127,  64, -64, 3, 12,update_lfo_pitchbend,       NULL);

    /* Initialize the pseudo parameters */
    init_param (PARAM_MIDI_CHANNEL,   "midi_channel",   "MIDI Channel",     PARAM_TYPE_HELP, 0, 0, 0, 0, 0, 0, NULL, NULL);
    init_param (PARAM_PROGRAM_NUMBER, "program_number", "Program #",        PARAM_TYPE_HELP, 0, 0, 0, 0, 0, 0, NULL, NULL);
    init_param (PARAM_PATCH_NAME,     "patch_name",     "Patch Name",       PARAM_TYPE_HELP, 0, 0, 0, 0, 0, 0, NULL, NULL);
    init_param (PARAM_BANK_MEM_MODE,  "bank_mem_mode",  "Bank Memory Mode", PARAM_TYPE_HELP, 0, 0, 0, 0, 0, 0, NULL, NULL);
    init_param (PARAM_PHASEX_HELP,    "using_phasex",   "Using PHASEX",     PARAM_TYPE_HELP, 0, 0, 0, 0, 0, 0, NULL, NULL);
}


/*****************************************************************************
 *
 * INIT_PARAMS_GROUPS()
 *
 * Map out parameter groups.
 * Each group ideally gets its own frame and label. 
 *
 *****************************************************************************/
void
init_param_groups(void) {
    int		j = 0;
    int		k = 0;

    k = 0;
    param_group[j].label = "<b>General</b>";
    param_group[j].full_x  = 0;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 30; //31
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 0;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 30; //33
    param_group[j].param_list[k++] = PARAM_BPM;
    param_group[j].param_list[k++] = PARAM_MASTER_TUNE;
    param_group[j].param_list[k++] = PARAM_KEYMODE;
    param_group[j].param_list[k++] = PARAM_KEYFOLLOW_VOL;
    param_group[j].param_list[k++] = PARAM_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_PORTAMENTO;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Input</b>";
    param_group[j].full_x  = 0;
    param_group[j].full_y0 = 30; //31
    param_group[j].full_y1 = 39; //42
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 0;
    param_group[j].notebook_y0   = 30; //33
    param_group[j].notebook_y1   = 41; //45
    param_group[j].param_list[k++] = PARAM_INPUT_FOLLOW;
    param_group[j].param_list[k++] = PARAM_INPUT_BOOST;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Amplifier</b>";
    param_group[j].full_x  = 0;
    param_group[j].full_y0 = 39; //42
    param_group[j].full_y1 = 76;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 0;
    param_group[j].notebook_y0   = 41; //45
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_VOLUME;
    param_group[j].param_list[k++] = PARAM_PAN;
    param_group[j].param_list[k++] = PARAM_STEREO_WIDTH;
    param_group[j].param_list[k++] = PARAM_AMP_VELOCITY;
    param_group[j].param_list[k++] = PARAM_AMP_ATTACK;
    param_group[j].param_list[k++] = PARAM_AMP_DECAY;
    param_group[j].param_list[k++] = PARAM_AMP_SUSTAIN;
    param_group[j].param_list[k++] = PARAM_AMP_RELEASE;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Chorus</b>";
    param_group[j].full_x  = 0;
    param_group[j].full_y0 = 76;
    param_group[j].full_y1 = 99;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 27;
    param_group[j].param_list[k++] = PARAM_CHORUS_MIX;
    param_group[j].param_list[k++] = PARAM_CHORUS_AMOUNT;
    param_group[j].param_list[k++] = PARAM_CHORUS_TIME;
    param_group[j].param_list[k++] = PARAM_CHORUS_FEED;
    param_group[j].param_list[k++] = PARAM_CHORUS_FLIP;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Chorus LFO</b>";
    param_group[j].full_x  = 0;
    param_group[j].full_y0 = 100;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 27;
    param_group[j].notebook_y1   = 40; 
    param_group[j].param_list[k++] = PARAM_CHORUS_LFO_WAVE;
    param_group[j].param_list[k++] = PARAM_CHORUS_LFO_RATE;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Filter</b>";
    param_group[j].full_x  = 1;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 33;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 1;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 33;
    param_group[j].param_list[k++] = PARAM_FILTER_CUTOFF;
    param_group[j].param_list[k++] = PARAM_FILTER_RESONANCE;
    param_group[j].param_list[k++] = PARAM_FILTER_SMOOTHING;
    param_group[j].param_list[k++] = PARAM_FILTER_KEYFOLLOW;
    param_group[j].param_list[k++] = PARAM_FILTER_MODE;
    param_group[j].param_list[k++] = PARAM_FILTER_TYPE;
    param_group[j].param_list[k++] = PARAM_FILTER_GAIN;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Filter LFO</b>";
    param_group[j].full_x  = 1;
    param_group[j].full_y0 = 33;
    param_group[j].full_y1 = 49;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 1;
    param_group[j].notebook_y0   = 33;
    param_group[j].notebook_y1   = 50;
    param_group[j].param_list[k++] = PARAM_FILTER_LFO;
    param_group[j].param_list[k++] = PARAM_FILTER_LFO_CUTOFF;
    param_group[j].param_list[k++] = PARAM_FILTER_LFO_RESONANCE;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Filter Envelope</b>";
    param_group[j].full_x  = 1;
    param_group[j].full_y0 = 49;
    param_group[j].full_y1 = 76;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 1;
    param_group[j].notebook_y0   = 50;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_FILTER_ENV_SIGN;
    param_group[j].param_list[k++] = PARAM_FILTER_ENV_AMOUNT;
    param_group[j].param_list[k++] = PARAM_FILTER_ATTACK;
    param_group[j].param_list[k++] = PARAM_FILTER_DECAY;
    param_group[j].param_list[k++] = PARAM_FILTER_SUSTAIN;
    param_group[j].param_list[k++] = PARAM_FILTER_RELEASE;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Delay</b>";
    param_group[j].full_x  = 1;
    param_group[j].full_y0 = 76;	
    param_group[j].full_y1 = 100;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 52;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_DELAY_MIX;
    param_group[j].param_list[k++] = PARAM_DELAY_FEED;
    param_group[j].param_list[k++] = PARAM_DELAY_TIME;
    param_group[j].param_list[k++] = PARAM_DELAY_FLIP;
    param_group[j].param_list[k++] = PARAM_DELAY_LFO;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Chorus Phaser</b>";
    param_group[j].full_x  = 1;
    param_group[j].full_y0 = 100;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 40;
    param_group[j].notebook_y1   = 52;
    param_group[j].param_list[k++] = PARAM_CHORUS_PHASE_RATE;
    param_group[j].param_list[k++] = PARAM_CHORUS_PHASE_BALANCE;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-1</b>";
    param_group[j].full_x  = 2;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 39;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 0;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 39;
    param_group[j].param_list[k++] = PARAM_OSC1_MODULATION;
    param_group[j].param_list[k++] = PARAM_OSC1_POLARITY;
    param_group[j].param_list[k++] = PARAM_OSC1_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_OSC1_WAVE;
    param_group[j].param_list[k++] = PARAM_OSC1_RATE;
    param_group[j].param_list[k++] = PARAM_OSC1_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_OSC1_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_OSC1_FINE_TUNE;
    param_group[j].param_list[k++] = PARAM_OSC1_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-1 Modulators</b>";
    param_group[j].full_x  = 2;
    param_group[j].full_y0 = 39;
    param_group[j].full_y1 = 78;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 0;
    param_group[j].notebook_y0   = 39;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_OSC1_AM_LFO;
    param_group[j].param_list[k++] = PARAM_OSC1_AM_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC1_FREQ_LFO;
    param_group[j].param_list[k++] = PARAM_OSC1_FREQ_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC1_FREQ_LFO_FINE;
    param_group[j].param_list[k++] = PARAM_OSC1_PHASE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC1_PHASE_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC1_WAVE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC1_WAVE_LFO_AMOUNT;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>LFO-1</b>";
    param_group[j].full_x  = 2;
    param_group[j].full_y0 = 78;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 3;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 40;
    param_group[j].param_list[k++] = PARAM_LFO1_POLARITY;
    param_group[j].param_list[k++] = PARAM_LFO1_WAVE;
    param_group[j].param_list[k++] = PARAM_LFO1_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_LFO1_RATE;
    param_group[j].param_list[k++] = PARAM_LFO1_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_LFO1_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_LFO1_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-2</b>";
    param_group[j].full_x  = 3;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 39;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 1;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 39;
    param_group[j].param_list[k++] = PARAM_OSC2_MODULATION;
    param_group[j].param_list[k++] = PARAM_OSC2_POLARITY;
    param_group[j].param_list[k++] = PARAM_OSC2_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_OSC2_WAVE;
    param_group[j].param_list[k++] = PARAM_OSC2_RATE;
    param_group[j].param_list[k++] = PARAM_OSC2_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_OSC2_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_OSC2_FINE_TUNE;
    param_group[j].param_list[k++] = PARAM_OSC2_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-2 Modulators</b>";
    param_group[j].full_x  = 3;
    param_group[j].full_y0 = 39;
    param_group[j].full_y1 = 78;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 1;
    param_group[j].notebook_y0   = 39;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_OSC2_AM_LFO;
    param_group[j].param_list[k++] = PARAM_OSC2_AM_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC2_FREQ_LFO;
    param_group[j].param_list[k++] = PARAM_OSC2_FREQ_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC2_FREQ_LFO_FINE;
    param_group[j].param_list[k++] = PARAM_OSC2_PHASE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC2_PHASE_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC2_WAVE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC2_WAVE_LFO_AMOUNT;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>LFO-2</b>";
    param_group[j].full_x  = 3;
    param_group[j].full_y0 = 78;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 3;
    param_group[j].notebook_y0   = 40;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_LFO2_POLARITY;
    param_group[j].param_list[k++] = PARAM_LFO2_WAVE;
    param_group[j].param_list[k++] = PARAM_LFO2_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_LFO2_RATE;
    param_group[j].param_list[k++] = PARAM_LFO2_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_LFO2_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_LFO2_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-3</b>";
    param_group[j].full_x  = 4;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 39;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 39;
    param_group[j].param_list[k++] = PARAM_OSC3_MODULATION;
    param_group[j].param_list[k++] = PARAM_OSC3_POLARITY;
    param_group[j].param_list[k++] = PARAM_OSC3_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_OSC3_WAVE;
    param_group[j].param_list[k++] = PARAM_OSC3_RATE;
    param_group[j].param_list[k++] = PARAM_OSC3_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_OSC3_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_OSC3_FINE_TUNE;
    param_group[j].param_list[k++] = PARAM_OSC3_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-3 Modulators</b>";
    param_group[j].full_x  = 4;
    param_group[j].full_y0 = 39;
    param_group[j].full_y1 = 78;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 2;
    param_group[j].notebook_y0   = 39;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_OSC3_AM_LFO;
    param_group[j].param_list[k++] = PARAM_OSC3_AM_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC3_FREQ_LFO;
    param_group[j].param_list[k++] = PARAM_OSC3_FREQ_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC3_FREQ_LFO_FINE;
    param_group[j].param_list[k++] = PARAM_OSC3_PHASE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC3_PHASE_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC3_WAVE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC3_WAVE_LFO_AMOUNT;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>LFO-3</b>";
    param_group[j].full_x  = 4;
    param_group[j].full_y0 = 78;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 4;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 40;
    param_group[j].param_list[k++] = PARAM_LFO3_POLARITY;
    param_group[j].param_list[k++] = PARAM_LFO3_WAVE;
    param_group[j].param_list[k++] = PARAM_LFO3_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_LFO3_RATE;
    param_group[j].param_list[k++] = PARAM_LFO3_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_LFO3_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_LFO3_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-4</b>";
    param_group[j].full_x  = 5;
    param_group[j].full_y0 = 0;
    param_group[j].full_y1 = 39;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 3;
    param_group[j].notebook_y0   = 0;
    param_group[j].notebook_y1   = 39;
    param_group[j].param_list[k++] = PARAM_OSC4_MODULATION;
    param_group[j].param_list[k++] = PARAM_OSC4_POLARITY;
    param_group[j].param_list[k++] = PARAM_OSC4_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_OSC4_WAVE;
    param_group[j].param_list[k++] = PARAM_OSC4_RATE;
    param_group[j].param_list[k++] = PARAM_OSC4_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_OSC4_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_OSC4_FINE_TUNE;
    param_group[j].param_list[k++] = PARAM_OSC4_PITCHBEND;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>Osc-4 Modulators</b>";
    param_group[j].full_x  = 5;
    param_group[j].full_y0 = 39;
    param_group[j].full_y1 = 78;
    param_group[j].notebook_page = 1;
    param_group[j].notebook_x    = 3;
    param_group[j].notebook_y0   = 39;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_OSC4_AM_LFO;
    param_group[j].param_list[k++] = PARAM_OSC4_AM_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC4_FREQ_LFO;
    param_group[j].param_list[k++] = PARAM_OSC4_FREQ_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC4_FREQ_LFO_FINE;
    param_group[j].param_list[k++] = PARAM_OSC4_PHASE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC4_PHASE_LFO_AMOUNT;
    param_group[j].param_list[k++] = PARAM_OSC4_WAVE_LFO;
    param_group[j].param_list[k++] = PARAM_OSC4_WAVE_LFO_AMOUNT;
    param_group[j].param_list[k++] = -1;
    j++;
    k = 0;
    param_group[j].label = "<b>LFO-4</b>";
    param_group[j].full_x  = 5;
    param_group[j].full_y0 = 78;
    param_group[j].full_y1 = 109;
    param_group[j].notebook_page = 0;
    param_group[j].notebook_x    = 4;
    param_group[j].notebook_y0   = 40;
    param_group[j].notebook_y1   = 78;
    param_group[j].param_list[k++] = PARAM_LFO4_POLARITY;
    param_group[j].param_list[k++] = PARAM_LFO4_WAVE;
    param_group[j].param_list[k++] = PARAM_LFO4_FREQ_BASE;
    param_group[j].param_list[k++] = PARAM_LFO4_RATE;
    param_group[j].param_list[k++] = PARAM_LFO4_INIT_PHASE;
    param_group[j].param_list[k++] = PARAM_LFO4_TRANSPOSE;
    param_group[j].param_list[k++] = PARAM_LFO4_PITCHBEND;
    param_group[j].param_list[k++] = -1;
}


/*****************************************************************************
 *
 * INIT_PARAM_PAGES()
 *
 * Map out the parameter notebook pages for the GUI.
 * Each page has two columns of n parameter groups.
 * Parameter groups are read in order of index.
 * 
 *****************************************************************************/
void
init_param_pages(void) {
    int		j = 0;

    param_page[j].label = "     Main    ";
    j++;
    param_page[j].label = " Oscillators ";
    j++;
}


/*****************************************************************************
 *
 * READ_MIDIMAP()
 * 
 *****************************************************************************/
int 
read_midimap(char *filename) {
    char	buffer[256];
    char	param_name[32];
    FILE	*map_f;
    char	*p;
    char	c;
    int		id;
    int		j;
    int		cc_num;
    int		line = 0;

    /* open the midimap file */
    if ((map_f = fopen (filename, "rt")) == NULL) {
	return -1;
    }

    /* keep track of filename */
    if (filename != midimap_filename) {
	if (midimap_filename != NULL) {
	    free (midimap_filename);
	}
	midimap_filename = strdup (filename);
    }

    /* read midimap entries */
    while (fgets (buffer, 256, map_f) != NULL) {
	line++;

	/* discard comments and blank lines */
	if ((buffer[0] == '\n') || (buffer[0] == '#')) {
	    continue;
	}

	/* convert to lower case and strip comments for simpler parsing */
	p = buffer;
	while ((p < (buffer + 256)) && ((c = *p) != '\0')) {
	    if (isupper (c)) {
		c  = tolower (c);
		*p = c;
	    }
	    else if (c == '#') {
		*p = '\0';
	    }
	    p++;
	}

	/* get parameter name */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	strncpy (param_name, p, sizeof (param_name));
	param_name[sizeof (param_name) - 1] = '\0';

	/* find named parameter */
	id = -1;
	for (j = 0; j < NUM_PARAMS; j++) {
	    if (strcmp (param_name, param[j].name) == 0) {
		id = j;
		break;
	    }
	}

	/* make sure there's an '=' */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	if (*p != '=') {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}

	/* get midi cc num */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	cc_num = atoi (p);
	if ((cc_num < -1) || (cc_num >= 128)) {
	    cc_num = -1;
	}

	/* see if there's a ',locked' */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	if (*p == ',') {
	    if ((p = get_next_token (buffer)) == NULL) {
		while (get_next_token (buffer) != NULL);
		continue;
	    }
	    if (strcmp (p, "locked") != 0) {
		while (get_next_token (buffer) != NULL);
		continue;
	    }
	    if ((id >= 0) && (id < NUM_PARAMS)) {
		param[id].locked = 1;
	    }
	    if ((p = get_next_token (buffer)) == NULL) {
		while (get_next_token (buffer) != NULL);
		continue;
	    }
	}
	else {
	    if ((id >= 0) && (id < NUM_PARAMS)) {
		param[id].locked = 0;
	    }
	}

	/* make sure there's a ';' */
	if (*p != ';') {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}

	/* flush remainder of line */
	while (get_next_token (buffer) != NULL);

	/* set midi cc number */
	if ((id >= 0) && (id < NUM_PARAMS)) {
#ifdef EXTRA_DEBUG
	    if (debug) {
		fprintf (stderr, "Load MIDI Map: id=[%03d] cc=[%03d] param=<%s>\n", id, cc_num, param[id].name);
	    }
#endif
	    param[id].cc_num = cc_num;
	}
	else {
	    if (debug && (strcmp (param_name, "midi_channel") != 0)) {
		fprintf (stderr, "Unknown parameter '%s' in midimap '%s', line %d.\n",
			 param_name, midimap_filename, line);
	    }
	}
    }

    /* done parsing */
    fclose (map_f);
    midimap_modified = 0;

    /* rebuild ccmatrix based on new map */
    build_ccmatrix ();

    return 0;
}


/*****************************************************************************
 *
 * SAVE_MIDIMAP()
 * 
 *****************************************************************************/
int 
save_midimap(char *filename) {
    FILE	*map_f;
    int		j;
    int		k;

    /* open the midimap file */
    if ((map_f = fopen (filename, "wt")) == NULL) {
	fprintf (stderr, "Error opening midimap file %s for write: %s\n",
		 filename, strerror(errno));
	return -1;
    }

    /* keep track of filename */
    if (filename != midimap_filename) {
	if (midimap_filename != NULL) {
	    free (midimap_filename);
	}
	midimap_filename = strdup (filename);
    }

    /* output 'param_name = cc_num;' for each param */
    for (j = 0; j < NUM_PARAMS; j++) {
	fprintf (map_f, "%s", param[j].name);
	k = 32 - strlen (param[j].name);
	while (k > 8) {
	    fputc ('\t', map_f);
	    k -= 8;
	}
	fprintf (map_f, "= %d%s;\n", param[j].cc_num, (param[j].locked ? ",locked" : ""));
    }

    /* done writing file */
    fclose (map_f);
    midimap_modified = 0;

    return 0;
}
