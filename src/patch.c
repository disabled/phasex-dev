/*****************************************************************************
 *
 * patch.c
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
#include <libgen.h>
#include "phasex.h"
#include "config.h"
#include "engine.h"
#include "filter.h"
#include "patch.h"
#include "callback.h"
#include "bank.h"


/* One global patch for this beast */
PATCH		*patch		= NULL;

PATCH_DIR_LIST	*patch_dir_list	= NULL;


/* time based rate values */
char *rate_names[] = {
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

/* Key mapping modes */
char *keymode_names[] = {
    "mono_smooth",
    "mono_retrig",
    "mono_multikey",
    "poly",
    NULL
};

/* Keyfollow modes */
char *keyfollow_names[] = {
    "off",
    "last",
    "high",
    "low",
    "keytrig",
    NULL
};

/* Names of modulators (oscs and lfos) */
char *modulator_names[] = {
    "off",
    "osc-1",
    "osc-2",
    "osc-3",
    "osc-4",
    "lfo-1",
    "lfo-2",
    "lfo-3",
    "lfo-4",
    "velocity",
    NULL
};

/* Names of waves in wave_table */
char *wave_names[] = {
    "sine",
    "triangle",
    "saw",
    "revsaw",
    "square",
    "stair",
    "bl_saw",
    "bl_revsaw",
    "bl_square",
    "bl_stair",
    "juno_osc",
    "juno_saw",
    "juno_square",
    "juno_poly",
    "vox_1",
    "vox_2",
    "null",
    "identity",
    NULL
};

/* Filter modes */
char *filter_mode_names[] = {
    "lp",
    "hp",
    "bp",
    "bs",
    "lp+bp",
    "hp+bp",
    "lp+hp",
    "bs+bp",
    NULL
};

/* Filter types */
char *filter_type_names[] = {
    "dist",
    "retro",
    NULL
};

/* Frequency basees */
char *freq_base_names[] = {
    "midi_key",
    "input_1",
    "input_2",
    "input_stereo",
    "amp_env",
    "filter_env",
    "velocity",
    "tempo",
    "keytrig",
    NULL
};

/* Modulation types */
char *modulation_names[] = {
    "off",
    "mix",
    "am",
    "mod",
    NULL
};

/* Polarity types */
char *polarity_names[] = {
    "bipolar",
    "unipolar",
    NULL
};

/* Sign names */
char *sign_names[] = {
    "negative",
    "positive",
    NULL
};

/* Boolean values */
char *boolean_names[] = {
    "off",
    "on",
    NULL
};

/* MIDI channel names */
char *midi_ch_names[] = {
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
    "omni",
    NULL
};


/*****************************************************************************
 *
 * GET_RATIONAL_FLOAT()
 *
 * Based on input token (integer, decimal, or fraction),
 * gives a float precision floating point value.
 *
 *****************************************************************************/
sample_t
get_rational_float(char *token) {
    sample_t	numerator;
    sample_t	denominator;
    char	*p;
    char	*token_copy;

    if ((token_copy = strdup (token)) == NULL) {
	phasex_shutdown ("Out of memory!\n");
    }

    if ((p = index (token_copy, '/')) != NULL) {
	*p++ = '\0';
	numerator = atof (token);
	if ((denominator = atof (p)) == 0.0) {
	    if (debug) {
		fprintf (stderr, "illegal rational number '%s/%s'\n", token, p);
	    }
	    free (token_copy);
	    return 1.0;
	}
	free (token_copy);
	return numerator / denominator;
    }

    free (token_copy);
    return atof(token);
}


/*****************************************************************************
 *
 * GET_RATE_VAL()
 *
 * Given an input MIDI cltr value,
 * returns a time based rate to be used by the engine.
 *
 *****************************************************************************/
sample_t
get_rate_val(int ctlr) {
    if (ctlr <= 0) {
	return 32.0;
    }
    else if (ctlr <= 64) {
	return (1.0 / (((sample_t)(ctlr)) * 4.0 / 64.0));
    }
    else if (ctlr <= 111) {
	return (1.0 / (((sample_t)(ctlr - 64)) * 4.0 / 48.0));
    }
    else if (ctlr <= 127) {
	return (1.0 / (((sample_t)(ctlr - 111)) * 4.0));
    }

    return 0.25;
}


/*****************************************************************************
 *
 * GET_RATE_CTLR()
 *
 * Given a rate value, checks it in the supported rates list and
 * returns the corresponding MIDI cc value.
 *
 *****************************************************************************/
int
get_rate_ctlr(char *token, char *filename, int line) {
    int		j;

    for (j = 0; j < 128; j++) {
	if (strncmp (token, rate_names[j], 5) == 0)
	    return j;
    }

    if (debug) {
	fprintf (stderr, "unsupported rate '%s' in %s, line %d -- using default\n",
		 token, filename, line);
    }

    return 0;
}


/*****************************************************************************
 *
 * GET_WAVE()
 *
 * Given an input token (waveform name),
 * returns a wave type enumerator.
 *
 *****************************************************************************/
int
get_wave(char *token, char *filename, int line) {
    int		wave = 0;

    if (strcmp (token, "sine") == 0)
	return WAVE_SINE;
    else if (strcmp (token, "triangle") == 0)
	return WAVE_TRIANGLE;
    else if (strcmp (token, "tri") == 0)
	return WAVE_TRIANGLE;
    else if (strcmp (token, "saw") == 0)
	return WAVE_SAW;
    else if (strcmp (token, "revsaw") == 0)
	return WAVE_REVSAW;
    else if (strcmp (token, "square") == 0)
	return WAVE_SQUARE;
    else if (strcmp (token, "stair") == 0)
	return WAVE_STAIR;
    else if (strcmp (token, "saw_s") == 0)
	return WAVE_SAW_S;
    else if (strcmp (token, "bl_saw") == 0)
	return WAVE_SAW_S;
    else if (strcmp (token, "revsaw_s") == 0)
	return WAVE_REVSAW_S;
    else if (strcmp (token, "bl_revsaw") == 0)
	return WAVE_REVSAW_S;
    else if (strcmp (token, "square_s") == 0)
	return WAVE_SQUARE_S;
    else if (strcmp (token, "bl_square") == 0)
	return WAVE_SQUARE_S;
    else if (strcmp (token, "stair_s") == 0)
	return WAVE_STAIR_S;
    else if (strcmp (token, "bl_stair") == 0)
	return WAVE_STAIR_S;
    else if (strcmp (token, "identity") == 0)
	return WAVE_IDENTITY;
    else if (strcmp (token, "null") == 0)
	return WAVE_NULL;
    else if (strcmp (token, "juno_osc") == 0)
	return WAVE_JUNO_OSC;
    else if (strcmp (token, "juno_saw") == 0)
	return WAVE_JUNO_SAW;
    else if (strcmp (token, "juno_square") == 0)
	return WAVE_JUNO_SQUARE;
    else if (strcmp (token, "juno_poly") == 0)
	return WAVE_JUNO_POLY;
    else if (strcmp (token, "vox_1") == 0)
	return WAVE_VOX_1;
    else if (strcmp (token, "vox_2") == 0)
	return WAVE_VOX_2;
    else if (((wave = atoi (token)) > 0) && (wave < NUM_WAVEFORMS))
	return wave;
    else
	fprintf (stderr, "unknown wave type '%s' in %s, line %d -- using sine\n",
		 token, filename, line);

    return WAVE_SINE;
}


/*****************************************************************************
 *
 * GET_POLARITY()
 *
 * Given an input token for polarity,
 * returns a polarity enumerator.
 *
 *****************************************************************************/
int
get_polarity(char *token, char *filename, int line) {
    if (strcmp (token, "unipolar") == 0)
	return POLARITY_UNIPOLAR;
    else if (strcmp (token, "bipolar") == 0)
	return POLARITY_BIPOLAR;

    fprintf (stderr, "unknown polarity '%s' in %s, line %d -- using bipolar\n",
	     token, filename, line);

    return POLARITY_BIPOLAR;
}


/*****************************************************************************
 *
 * GET_CTLR()
 *
 * Given an input token, returns a MIDI controller value (0-127).
 *
 *****************************************************************************/
int
get_ctlr(char *token, char *filename, int line) {
    return atoi (token) % 128;
}


/*****************************************************************************
 *
 * GET_BOOLEAN()
 *
 * Given an input token, returns a boolean value of 1 or 0.
 *
 *****************************************************************************/
int
get_boolean(char *token, char *filename, int line) {
    if (strcmp (token, "1") == 0)
	return 1;
    else if (strcmp (token, "true") == 0)
	return 1;
    else if (strcmp (token, "yes") == 0)
	return 1;
    else if (strcmp (token, "on") == 0)
	return 1;
    else
	return 0;
}


/*****************************************************************************
 *
 * GET_FREQ_BASE()
 *
 * Given an input token, returns waveform frequency/timing freq_base enumerator.
 *
 *****************************************************************************/
int
get_freq_base(char *token, char *filename, int line) {
    if (strcmp (token, "midi_key") == 0)
	return FREQ_BASE_MIDI_KEY;
    else if (strcmp (token, "tempo") == 0)
	return FREQ_BASE_TEMPO;
    else if (strcmp (token, "keytrig") == 0)
	return FREQ_BASE_TEMPO_KEYTRIG;
    else if (strcmp (token, "input_1") == 0)
	return FREQ_BASE_INPUT_1;
    else if (strcmp (token, "input_2") == 0)
	return FREQ_BASE_INPUT_2;
    else if (strcmp (token, "input_stereo") == 0)
	return FREQ_BASE_INPUT_STEREO;
    else if (strncmp (token, "amp_env", 7) == 0)
	return FREQ_BASE_AMP_ENVELOPE;
    else if (strncmp (token, "filter_env", 10) == 0)
	return FREQ_BASE_FILTER_ENVELOPE;
    else if (strncmp (token, "velocity", 8) == 0)
	return FREQ_BASE_VELOCITY;
    else
	return FREQ_BASE_MIDI_KEY;
}


/*****************************************************************************
 *
 * GET_NEXT_TOKEN()
 *
 * Use in a while loop to split a patch definition line into its tokens.
 * Whitespace always delimits a token.  The special tokens '{', '}', '=', and
 * ';' are always tokenized, regardless of leading or trailing whitespace.
 *
 *****************************************************************************/
char *
get_next_token(char *inbuf) {
    int			len;
    int			in_quote = 0;
    static int		eob = 1;
    char		*token_begin;
    static char		*cur_index = NULL;
    static char		*last_inbuf = NULL;
    static char		token_buf[256];

    /* keep us out of trouble */
    if ((inbuf == NULL) && (last_inbuf == NULL)) {
	return NULL;
    }

    /* was end of buffer set last time? */
    if (eob) {
	eob = 0;
	cur_index = inbuf;
    }

    /* keep us out of more trouble */
    if ( (cur_index == NULL) || (*cur_index == '\0') || 
	 (*cur_index == '#') || (*cur_index == '\n') )
    {
	eob = 1;
	return NULL;
    }

    /* skip past whitespace */
    while ((*cur_index == ' ') || (*cur_index == '\t')) {
	cur_index++;
    }

    /* check for quoted token */
    if (*cur_index == '"') {
	in_quote = 1;
	cur_index++;
    }

    /* we're at the start of the current token */
    token_begin = cur_index;

    /* go just past the last character of the token */
    if (in_quote) {
	while (*cur_index != '"') {
	    cur_index++;
	}
	//cur_index++;
    }
    else if ( (*cur_index == '{') || (*cur_index == '}') ||
	 (*cur_index == ';') || (*cur_index == '=') || (*cur_index == ',') )
    {
	cur_index++;
    }
    else {
	while ( (*cur_index != ' ')  && (*cur_index != '\t') &&
		(*cur_index != '{')  && (*cur_index != '}')  &&
		(*cur_index != ';')  && (*cur_index != '=')  &&
		(*cur_index != '\0') && (*cur_index != '\n') &&
		(*cur_index != '#')  && (*cur_index != ',') )
	{
	    cur_index++;
	}
    }

    /* check for end of buffer (null, newline, or comment delim) */
    if ((cur_index == token_begin) && ((*cur_index == '\0') || (*cur_index == '\n') || (*cur_index == '#'))) {
	eob = 1;
	return NULL;
    }

    /* copy the token to our static buffer and terminate */
    len = (cur_index - token_begin) % sizeof (token_buf);
    strncpy (token_buf, token_begin, len);
    token_buf[len] = '\0';

    /* skip past quote */
    if (in_quote) {
	cur_index++;
    }

    /* skip past whitespace */
    while ((*cur_index == ' ') || (*cur_index == '\t')) {
	cur_index++;
    }

    /* return the address to the token buffer */
    return token_buf;
}


/*****************************************************************************
 *
 * INIT_PATCH()
 *
 * Initialize the global patch struct.
 *
 *****************************************************************************/
void
init_patch(PATCH *patch) {
    static int	once = 1;
    int		cur_osc;
    int		cur_lfo;
    int		j;

    /* free dynamic mem */
    if (patch->name != NULL) {
	free (patch->name);
	patch->name = NULL;
    }
    if (patch->filename != NULL) {
	free (patch->filename);
	patch->filename = NULL;
    }
    if (patch->directory != NULL) {
	free (patch->directory);
	patch->directory = NULL;
    }
    patch->modified			= 0;

    /* set some sane defaults */
    patch->volume			= 0.0;
    patch->volume_cc			= 0;
    patch->bpm				= 120.0;
    patch->bpm_cc			= 120 - 64;
    patch->master_tune			= 0;
    patch->master_tune_cc		= 64;
    patch->portamento			= 2;
    patch->keymode			= KEYMODE_MONO_SMOOTH;
    patch->keyfollow_vol		= 64;
    patch->transpose			= 0;
    patch->transpose_cc			= 64;
    patch->input_boost			= 5.0;
    patch->input_boost_cc		= 64;
    patch->input_follow			= 0;
    patch->pan_cc			= 64;
    patch->stereo_width			= 1.0;
    patch->stereo_width_cc		= 127;
    patch->delay_mix			= 0.5;
    patch->delay_mix_cc			= 64;
    patch->delay_feed			= 0.5;
    patch->delay_feed_cc		= 64;
    patch->delay_flip			= 1;
    patch->delay_time			= 3;
    patch->delay_time_cc		= 48;
    patch->delay_lfo			= LFO_OFF;
    patch->delay_lfo_cc			= 0;
    patch->chorus_mix			= 0.5;
    patch->chorus_mix_cc		= 64;
    patch->chorus_feed			= 0.5;
    patch->chorus_feed_cc		= 64;
    patch->chorus_flip			= 1;
    patch->chorus_time			= 32.0 * f_sample_rate / 12000.0;
    patch->chorus_time_cc		= 32;
    patch->chorus_amount		= 64.0 / 128.0;
    patch->chorus_amount_cc		= 64;
    patch->chorus_phase_rate		= 0.25;
    patch->chorus_phase_rate_cc		= 0;
    patch->chorus_phase_balance		= 0.125;
    patch->chorus_phase_balance_cc	= 32;
    patch->chorus_lfo_wave		= WAVE_SINE;
    patch->chorus_lfo_rate		= 0.25;
    patch->chorus_lfo_rate_cc		= 0;
    patch->amp_attack			= 16;
    patch->amp_decay			= 16;
    patch->amp_sustain			= 100.0 / 127.0;
    patch->amp_sustain_cc		= 100;
    patch->amp_release			= 16;
    patch->filter_cutoff		= 127.0;
    patch->filter_cutoff_cc		= 127;
    patch->filter_resonance		= 0;
    patch->filter_resonance_cc		= 0;
    patch->filter_smoothing		= 32;
    patch->filter_keyfollow		= KEYFOLLOW_MIDI;
    patch->filter_mode			= FILTER_MODE_LP;
    patch->filter_type			= FILTER_TYPE_DIST;
    patch->filter_gain			= 0.5;
    patch->filter_gain_cc		= 64;
    patch->filter_env_amount		= 0.0;
    patch->filter_env_amount_cc		= 0;
    patch->filter_env_sign		= 1.0;
    patch->filter_env_sign_cc		= SIGN_POSITIVE;
    patch->filter_attack		= 16;
    patch->filter_decay			= 16;
    patch->filter_sustain		= 100.0 / 127.0;
    patch->filter_sustain_cc		= 100;
    patch->filter_release		= 16;
    patch->filter_lfo			= LFO_OFF;
    patch->filter_lfo_cc		= 0;
    patch->filter_lfo_cutoff		= 0.0;
    patch->filter_lfo_cutoff_cc		= 64;
    patch->filter_lfo_resonance		= 0.0;
    patch->filter_lfo_resonance_cc	= 64;
    for (cur_osc = 0; cur_osc < NUM_OSCS; cur_osc++) {
	patch->osc_wave[cur_osc]		= WAVE_SINE;
	patch->osc_freq_base[cur_osc]		= FREQ_BASE_MIDI_KEY;
	patch->osc_polarity_cc[cur_osc]		= POLARITY_BIPOLAR;
	patch->osc_rate[cur_osc]		= 0.25;
	patch->osc_rate_cc[cur_osc]		= 0;
	patch->osc_init_phase[cur_osc]		= 0.0;
	patch->osc_init_phase_cc[cur_osc]	= 0;
	patch->osc_transpose[cur_osc]		= 0;
	patch->osc_transpose_cc[cur_osc]	= 64;
	patch->osc_fine_tune[cur_osc]		= 0.0;
	patch->osc_fine_tune_cc[cur_osc]	= 64;
	patch->osc_modulation[cur_osc]		= MOD_TYPE_NONE;
	patch->osc_pitchbend[cur_osc]		= 0;
	patch->osc_pitchbend_cc[cur_osc]	= 64;
	patch->osc_channel[cur_osc]		= 1;
	patch->am_lfo[cur_osc]			= LFO_OFF;
	patch->am_lfo_cc[cur_osc]		= 0;
	patch->am_lfo_amount[cur_osc]		= 0.0;
	patch->am_lfo_amount_cc[cur_osc]	= 64;
	patch->freq_lfo[cur_osc]		= LFO_OFF;
	patch->freq_lfo_cc[cur_osc]		= 0;
	patch->freq_lfo_amount[cur_osc]		= 0.0;
	patch->freq_lfo_amount_cc[cur_osc]	= 64;
	patch->freq_lfo_fine[cur_osc]		= 0.0;
	patch->freq_lfo_fine_cc[cur_osc]	= 64;
	patch->phase_lfo[cur_osc]		= LFO_OFF;
	patch->phase_lfo_cc[cur_osc]		= 0;
	patch->phase_lfo_amount[cur_osc]	= 0.0;
	patch->phase_lfo_amount_cc[cur_osc]	= 64;
	patch->wave_lfo[cur_osc]		= LFO_OFF;
	patch->wave_lfo_cc[cur_osc]		= 0;
	patch->wave_lfo_amount[cur_osc]		= 0.0;
	patch->wave_lfo_amount_cc[cur_osc]	= 64;
    }
    for (cur_lfo = 0; cur_lfo < NUM_LFOS; cur_lfo++) {
	patch->lfo_wave[cur_lfo]		= WAVE_SINE;
	patch->lfo_freq_base[cur_lfo]		= FREQ_BASE_TEMPO_KEYTRIG;
	patch->lfo_polarity[cur_lfo]		= 0.0;
	patch->lfo_polarity_cc[cur_lfo]		= POLARITY_BIPOLAR;
	patch->lfo_rate[cur_lfo]		= 0.25;
	patch->lfo_rate_cc[cur_lfo]		= 0;
	patch->lfo_init_phase[cur_lfo]		= 0.0;
	patch->lfo_init_phase_cc[cur_lfo]	= 0;
	patch->lfo_transpose[cur_lfo]		= 0;
	patch->lfo_transpose_cc[cur_lfo]	= 64;
	patch->lfo_pitchbend[cur_lfo]		= 0;
	patch->lfo_pitchbend_cc[cur_lfo]	= 64;
    }

    /* volume should always be last */
    patch->volume			= 1.0;
    patch->volume_cc			= 120;

    /* first time through, set the updated flag for the gui */
    /* do not call param callbacks here... patch loading will do that */
    if (once) {
	once = 0;
	for (j = 0; j < NUM_PARAMS; j++) {
	    param[j].updated = 1;
	}
    }
}


/*****************************************************************************
 *
 * READ_PATCH()
 *
 * State machine parser for patch definition files.
 * Reads a file into the patchbank in the designated slot.
 *
 *****************************************************************************/
int 
read_patch(char *filename, int program_number) {
    PATCH		*patch		= &(bank[program_number]);
    PATCH_DIR_LIST	*pdir		= patch_dir_list;
    PATCH_DIR_LIST	*ldir		= NULL;
    FILE		*patch_f;
    char		*token;
    char		*p;
    char		buffer[256];
    char		c;
    int			updated[NUM_PARAMS];
    int			j;
    int			id;
    int			line		= 0;
    int			state		= PARSE_STATE_DEF_TYPE;
    int			cur_osc		= 0;
    int			cur_lfo		= 0;
    int			dir_found	= 0;

    /* return error on missing filename */
    if ((filename == NULL) || (filename[0] == '\0')) {
	return -1;
    }

    /* open the patch file */
    if ((patch_f = fopen (filename, "rt")) == NULL) {
	return -1;
    }

    /* free strings (will be rebuilt next) */
    if (patch->filename != NULL) {
	free (patch->filename);
	patch->filename = NULL;
    }
    if (patch->directory != NULL) {
	free (patch->directory);
	patch->directory = NULL;
    }
    if (patch->name != NULL) {
	free (patch->name);
	patch->name = NULL;
    }

    /* keep track of filename, directory, and patch name, ignoring dump files */
    if ((filename != sys_default_patch) && (strncmp (filename, "/tmp/patch", 10) != 0)) {
	patch->filename = strdup (filename);
	p = strdup (filename);
	patch->directory = strdup (dirname (p));
	memcpy (p, filename, (strlen (filename) + 1));
	token = basename (p);
	j = strlen (token) - 4;
	if (strcmp (token + j, ".phx") == 0) {
	    token[j] = '\0';
	}
	patch->name = strdup (token);
	free (p);

	/* maintain patch directory list */
	if ((patch->directory != NULL) && (*(patch->directory) != '\0') &&
	    (strcmp (patch->directory, PATCH_DIR) != 0) &&
	    (strcmp (patch->directory, user_patch_dir) != 0))
	{
	    while ((pdir != NULL) && (!dir_found)) {
		if (strcmp(pdir->name, patch->directory) == 0) {
		    dir_found = 1;
		}
		else {
		    ldir = pdir;
		    pdir = pdir->next;
		}
	    }
	    if (!dir_found) {
		if ((pdir = malloc (sizeof (PATCH_DIR))) == NULL) {
		    phasex_shutdown("Out of Memory!\n");
		}
		pdir->name = strdup (patch->directory);
		pdir->load_shortcut = pdir->save_shortcut = 0;
		if (ldir == NULL) {
		    pdir->next = NULL;
		    patch_dir_list = pdir;
		}
		else {
		    pdir->next = ldir->next;
		    ldir->next = pdir;
		}
	    }
	}
    }

    /* initialize updated array */
    for (j = 0; j < NUM_PARAMS; j++) {
	updated[j] = 0;
    }

    /* read patch entries */
    while (fgets (buffer, 256, patch_f) != NULL) {
	line++;

	/* discard comments and blank lines */
	if ((buffer[0] == '\n') || (buffer[0] == '#'))
	    continue;

	/* convert to lower case and strip comments for simpler parsing */
	p = buffer;
	while ((p < (buffer + 256)) && ((c = *p) != '\0')) {
	    if (isupper (c)) {
		c  = tolower (c);
		*p = c;
	    }
	    else if (c == '#')
		*p = '\0';
	    p++;
	}

	/* parse each token on this line */
	while ((token = get_next_token (buffer)) != NULL) {

	    /* enter the parser's state machine */
	    switch (state) {

	    /* looking for a valid definition type */
	    case PARSE_STATE_DEF_TYPE:
		/* parse definition types (only 'phasex_patch' for now...) */
		if (strcmp (token, "phasex_patch") == 0) {
		    state = PARSE_STATE_PATCH_START;
		}
		else {
		    fprintf (stderr, "unknown definition type '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		}
		break;

	    /* looking for a '{' */
	    case PARSE_STATE_PATCH_START:
		if (strcmp (token, "{") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		else if (debug) {
		    fprintf (stderr, "expecting '{' to begin patch subsection in %s, line %d -- discarding\n",
			     filename, line);
		}
		break;

	    /* looking for a patch subsection name or a '}' */
	    case PARSE_STATE_SUBSECT_ENTRY:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_PATCH_END;
		else if (strcmp (token, "oscillator") == 0) {
		    if (cur_osc >= NUM_OSCS)
			fprintf (stderr, "exceeded oscillator limit in %s, line %d -- discarding\n",
				 filename, line);
		    else
			state = PARSE_STATE_OSC_START;
		}
		else if (strcmp (token, "lfo") == 0) {
		    if (cur_lfo >= NUM_LFOS)
			fprintf (stderr, "exceeded lfo limit in %s, line %d -- discarding\n",
				 filename, line);
		    else
			state = PARSE_STATE_LFO_START;
		}
		else if (strcmp (token, "envelope") == 0)
		    state = PARSE_STATE_ENV_START;
		else if (strcmp (token, "filter") == 0)
		    state = PARSE_STATE_FILTER_START;
		else if (strcmp (token, "delay") == 0)
		    state = PARSE_STATE_DELAY_START;
		else if (strcmp (token, "chorus") == 0)
		    state = PARSE_STATE_CHORUS_START;
		else if (strcmp (token, "general") == 0)
		    state = PARSE_STATE_GENERAL_START;
		else if (strcmp (token, "info") == 0)
		    state = PARSE_STATE_UNKNOWN_START;
		else if (*token == '_')
		    state = PARSE_STATE_UNKNOWN_START;
		else {
		    fprintf (stderr, "unknown patch subsection '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_START;
		}
		break;

	    /* looking for a '{' to start a subsection */
	    case PARSE_STATE_OSC_START:
	    case PARSE_STATE_LFO_START:
	    case PARSE_STATE_ENV_START:
	    case PARSE_STATE_FILTER_START:
	    case PARSE_STATE_DELAY_START:
	    case PARSE_STATE_CHORUS_START:
	    case PARSE_STATE_GENERAL_START:
	    case PARSE_STATE_UNKNOWN_START:
		if (strcmp (token, "{") == 0)
		    state += 16;
		else if (debug) {
		    fprintf (stderr, "expecting '{' to begin subsection in %s, line %d\n",
			     filename, line);
		}
		break;

	    /* looking for an oscillator attribute */
	    case PARSE_STATE_OSC_ATTR:
		/* done with this oscillator */
		if (strcmp (token, "}") == 0) {
		    cur_osc++;
		    state = PARSE_STATE_SUBSECT_ENTRY;
		}
		/* still handling params for this oscillator */
		else if (strcmp (token, "wave") == 0)
		    state = PARSE_STATE_OSC_WAVE_EQ;
		else if (strcmp (token, "init_phase") == 0)
		    state = PARSE_STATE_OSC_INIT_PHASE_EQ;
		else if (strcmp (token, "polarity") == 0)
		    state = PARSE_STATE_OSC_POLARITY_EQ;
		else if (strcmp (token, "scaling") == 0)
		    state = PARSE_STATE_OSC_POLARITY_EQ;
		else if (strcmp (token, "freq_base") == 0)
		    state = PARSE_STATE_OSC_FREQ_BASE_EQ;
		else if (strcmp (token, "source") == 0)
		    state = PARSE_STATE_OSC_FREQ_BASE_EQ;
		else if (strcmp (token, "rate") == 0)
		    state = PARSE_STATE_OSC_RATE_EQ;
		else if (strcmp (token, "modulation") == 0)
		    state = PARSE_STATE_OSC_MODULATION_EQ;
		else if (strcmp (token, "transpose") == 0)
		    state = PARSE_STATE_OSC_TRANSPOSE_EQ;
		else if (strcmp (token, "fine_tune") == 0)
		    state = PARSE_STATE_OSC_FINE_TUNE_EQ;
		else if (strcmp (token, "pitchbend") == 0)
		    state = PARSE_STATE_OSC_PITCHBEND_AMOUNT_EQ;
		else if (strcmp (token, "midi_channel") == 0)
		    state = PARSE_STATE_OSC_MIDI_CHANNEL_EQ;
		else if (strcmp (token, "am_lfo") == 0)
		    state = PARSE_STATE_OSC_AM_LFO_EQ;
		else if (strcmp (token, "am_lfo_amount") == 0)
		    state = PARSE_STATE_OSC_AM_LFO_AMOUNT_EQ;
		else if (strcmp (token, "am_mod") == 0)
		    state = PARSE_STATE_OSC_AM_LFO_EQ;
		else if (strcmp (token, "am_mod_amount") == 0)
		    state = PARSE_STATE_OSC_AM_LFO_AMOUNT_EQ;
		else if (strcmp (token, "freq_lfo") == 0)
		    state = PARSE_STATE_OSC_FREQ_LFO_EQ;
		else if (strcmp (token, "freq_lfo_amount") == 0)
		    state = PARSE_STATE_OSC_FREQ_LFO_AMOUNT_EQ;
		else if (strcmp (token, "fm_mod") == 0)
		    state = PARSE_STATE_OSC_FREQ_LFO_EQ;
		else if (strcmp (token, "fm_mod_amount") == 0)
		    state = PARSE_STATE_OSC_FREQ_LFO_AMOUNT_EQ;
		else if (strcmp (token, "fm_mod_fine") == 0)
		    state = PARSE_STATE_OSC_FREQ_LFO_FINE_EQ;
		else if (strcmp (token, "phase_lfo") == 0)
		    state = PARSE_STATE_OSC_PHASE_LFO_EQ;
		else if (strcmp (token, "phase_lfo_amount") == 0)
		    state = PARSE_STATE_OSC_PHASE_LFO_AMOUNT_EQ;
		else if (strcmp (token, "pm_mod") == 0)
		    state = PARSE_STATE_OSC_PHASE_LFO_EQ;
		else if (strcmp (token, "pm_mod_amount") == 0)
		    state = PARSE_STATE_OSC_PHASE_LFO_AMOUNT_EQ;
		else if (strcmp (token, "wave_lfo") == 0)
		    state = PARSE_STATE_OSC_WAVE_LFO_EQ;
		else if (strcmp (token, "wave_lfo_amount") == 0)
		    state = PARSE_STATE_OSC_WAVE_LFO_AMOUNT_EQ;
		else {
		    fprintf (stderr, "unknown osc attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for an LFO attribute */
	    case PARSE_STATE_LFO_ATTR:
		/* done with this lfo */
		if (strcmp (token, "}") == 0) {
		    cur_lfo++;
		    state = PARSE_STATE_SUBSECT_ENTRY;
		}
		/* still handling params for this LFO */
		else if (strcmp (token, "wave") == 0)
		    state = PARSE_STATE_LFO_WAVE_EQ;
		else if (strcmp (token, "init_phase") == 0)
		    state = PARSE_STATE_LFO_INIT_PHASE_EQ;
		else if (strcmp (token, "polarity") == 0)
		    state = PARSE_STATE_LFO_POLARITY_EQ;
		else if (strcmp (token, "scaling") == 0)
		    state = PARSE_STATE_LFO_POLARITY_EQ;
		else if (strcmp (token, "freq_base") == 0)
		    state = PARSE_STATE_LFO_FREQ_BASE_EQ;
		else if (strcmp (token, "source") == 0)
		    state = PARSE_STATE_LFO_FREQ_BASE_EQ;
		else if (strcmp (token, "rate") == 0)
		    state = PARSE_STATE_LFO_RATE_EQ;
		else if (strcmp (token, "transpose") == 0)
		    state = PARSE_STATE_LFO_TRANSPOSE_EQ;
		else if (strcmp (token, "pitchbend") == 0)
		    state = PARSE_STATE_LFO_PITCHBEND_AMOUNT_EQ;
		else {
		    fprintf (stderr, "unknown lfo attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for an envelope attribute */
	    case PARSE_STATE_ENV_ATTR:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		else if (strcmp (token, "attack") == 0)
		    state = PARSE_STATE_ENV_ATTACK_EQ;
		else if (strcmp (token, "decay") == 0)
		    state = PARSE_STATE_ENV_DECAY_EQ;
		else if (strcmp (token, "sustain") == 0)
		    state = PARSE_STATE_ENV_SUSTAIN_EQ;
		else if (strcmp (token, "release") == 0)
		    state = PARSE_STATE_ENV_RELEASE_EQ;
		else {
		    fprintf (stderr, "unknown env attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for a filter attribute */
	    case PARSE_STATE_FILTER_ATTR:
		if (strcmp (token, "}") == 0) {
		    state = PARSE_STATE_SUBSECT_ENTRY;
		}
		else if (strcmp (token, "cutoff") == 0)
		    state = PARSE_STATE_FILTER_CUTOFF_EQ;
		else if (strcmp (token, "resonance") == 0)
		    state = PARSE_STATE_FILTER_RESONANCE_EQ;
		else if (strcmp (token, "smoothing") == 0)
		    state = PARSE_STATE_FILTER_SMOOTHING_EQ;
		else if (strcmp (token, "keyfollow") == 0)
		    state = PARSE_STATE_FILTER_KEYFOLLOW_EQ;
		else if (strcmp (token, "mode") == 0)
		    state = PARSE_STATE_FILTER_MODE_EQ;
		else if (strcmp (token, "type") == 0)
		    state = PARSE_STATE_FILTER_TYPE_EQ;
		else if (strcmp (token, "gain") == 0)
		    state = PARSE_STATE_FILTER_GAIN_EQ;
		else if (strcmp (token, "env_amount") == 0)
		    state = PARSE_STATE_FILTER_ENV_AMOUNT_EQ;
		else if (strcmp (token, "env_sign") == 0)
		    state = PARSE_STATE_FILTER_ENV_SIGN_EQ;
		else if (strcmp (token, "attack") == 0)
		    state = PARSE_STATE_FILTER_ENV_ATTACK_EQ;
		else if (strcmp (token, "decay") == 0)
		    state = PARSE_STATE_FILTER_ENV_DECAY_EQ;
		else if (strcmp (token, "sustain") == 0)
		    state = PARSE_STATE_FILTER_ENV_SUSTAIN_EQ;
		else if (strcmp (token, "release") == 0)
		    state = PARSE_STATE_FILTER_ENV_RELEASE_EQ;
		else if (strcmp (token, "lfo") == 0)
		    state = PARSE_STATE_FILTER_LFO_EQ;
		else if (strcmp (token, "lfo_amount") == 0)
		    state = PARSE_STATE_FILTER_LFO_CUTOFF_EQ;
		else if (strcmp (token, "lfo_cutoff") == 0)
		    state = PARSE_STATE_FILTER_LFO_CUTOFF_EQ;
		else if (strcmp (token, "lfo_resonance") == 0)
		    state = PARSE_STATE_FILTER_LFO_RESONANCE_EQ;
		else {
		    fprintf (stderr, "unknown filter attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for a delay attribute */
	    case PARSE_STATE_DELAY_ATTR:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		else if (strcmp (token, "mix") == 0)
		    state = PARSE_STATE_DELAY_MIX_EQ;
		else if (strcmp (token, "feed") == 0)
		    state = PARSE_STATE_DELAY_FEED_EQ;
		else if (strcmp (token, "flip") == 0)
		    state = PARSE_STATE_DELAY_FLIP_EQ;
		else if (strcmp (token, "crossover") == 0)
		    state = PARSE_STATE_DELAY_FLIP_EQ;
		else if (strcmp (token, "time") == 0)
		    state = PARSE_STATE_DELAY_TIME_EQ;
		else if (strcmp (token, "lfo") == 0)
		    state = PARSE_STATE_DELAY_LFO_EQ;
		else {
		    fprintf (stderr, "unknown delay attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for a chorus attribute */
	    case PARSE_STATE_CHORUS_ATTR:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		else if (strcmp (token, "mix") == 0)
		    state = PARSE_STATE_CHORUS_MIX_EQ;
		else if (strcmp (token, "feed") == 0)
		    state = PARSE_STATE_CHORUS_FEED_EQ;
		else if (strcmp (token, "flip") == 0)
		    state = PARSE_STATE_CHORUS_FLIP_EQ;
		else if (strcmp (token, "crossover") == 0)
		    state = PARSE_STATE_CHORUS_FLIP_EQ;
		else if (strcmp (token, "time") == 0)
		    state = PARSE_STATE_CHORUS_TIME_EQ;
		else if (strcmp (token, "amount") == 0)
		    state = PARSE_STATE_CHORUS_AMOUNT_EQ;
		else if (strcmp (token, "depth") == 0)
		    state = PARSE_STATE_CHORUS_AMOUNT_EQ;
		else if (strcmp (token, "phase_rate") == 0)
		    state = PARSE_STATE_CHORUS_PHASE_RATE_EQ;
		else if (strcmp (token, "phase_balance") == 0)
		    state = PARSE_STATE_CHORUS_PHASE_BALANCE_EQ;
		else if (strcmp (token, "phase_amount") == 0)
		    state = PARSE_STATE_CHORUS_PHASE_BALANCE_EQ;
		else if (strcmp (token, "lfo_wave") == 0)
		    state = PARSE_STATE_CHORUS_LFO_WAVE_EQ;
		else if (strcmp (token, "lfo_rate") == 0)
		    state = PARSE_STATE_CHORUS_LFO_RATE_EQ;
		else {
		    fprintf (stderr, "unknown chorus attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for a parameter attribute */
	    case PARSE_STATE_GENERAL_ATTR:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		else if (strcmp (token, "bpm") == 0)
		    state = PARSE_STATE_GENERAL_BPM_EQ;
		else if (strcmp (token, "master_tune") == 0)
		    state = PARSE_STATE_GENERAL_MASTER_TUNE_EQ;
		else if (strcmp (token, "portamento") == 0)
		    state = PARSE_STATE_GENERAL_PORTAMENTO_EQ;
		else if (strcmp (token, "midi_channel") == 0)
		    state = PARSE_STATE_GENERAL_MIDI_CHANNEL_EQ;
		else if (strcmp (token, "keymode") == 0)
		    state = PARSE_STATE_GENERAL_KEYMODE_EQ;
		else if (strcmp (token, "keyfollow_vol") == 0)
		    state = PARSE_STATE_GENERAL_KEYFOLLOW_VOL_EQ;
		else if (strcmp (token, "volume") == 0)
		    state = PARSE_STATE_GENERAL_VOLUME_EQ;
		else if (strcmp (token, "transpose") == 0)
		    state = PARSE_STATE_GENERAL_TRANSPOSE_EQ;
		else if (strcmp (token, "input_boost") == 0)
		    state = PARSE_STATE_GENERAL_INPUT_BOOST_EQ;
		else if (strcmp (token, "input_follow") == 0)
		    state = PARSE_STATE_GENERAL_INPUT_FOLLOW_EQ;
		else if (strcmp (token, "pan") == 0)
		    state = PARSE_STATE_GENERAL_PAN_EQ;
		else if (strcmp (token, "stereo_width") == 0)
		    state = PARSE_STATE_GENERAL_STEREO_WIDTH_EQ;
		else if (strcmp (token, "amp_velocity") == 0)
		    state = PARSE_STATE_GENERAL_AMP_VELOCITY_EQ;
		else {
		    fprintf (stderr, "unknown parameter attr '%s' in %s, line %d -- discarding\n",
			     token, filename, line);
		    state = PARSE_STATE_UNKNOWN_EQ;
		}
		break;

	    /* looking for a '}' to close an unknown subsection */
	    case PARSE_STATE_UNKNOWN_ATTR:
		if (strcmp (token, "}") == 0)
		    state = PARSE_STATE_SUBSECT_ENTRY;
		break;

	    /* looking for an '=' */
	    case PARSE_STATE_OSC_WAVE_EQ:
	    case PARSE_STATE_OSC_INIT_PHASE_EQ:
	    case PARSE_STATE_OSC_POLARITY_EQ:
	    case PARSE_STATE_OSC_FREQ_BASE_EQ:
	    case PARSE_STATE_OSC_MODULATION_EQ:
	    case PARSE_STATE_OSC_RATE_EQ:
	    case PARSE_STATE_OSC_TRANSPOSE_EQ:
	    case PARSE_STATE_OSC_FINE_TUNE_EQ:
	    case PARSE_STATE_OSC_PITCHBEND_AMOUNT_EQ:
	    case PARSE_STATE_OSC_MIDI_CHANNEL_EQ:
	    case PARSE_STATE_OSC_AM_LFO_EQ:
	    case PARSE_STATE_OSC_AM_LFO_AMOUNT_EQ:
	    case PARSE_STATE_OSC_FREQ_LFO_EQ:
	    case PARSE_STATE_OSC_FREQ_LFO_AMOUNT_EQ:
	    case PARSE_STATE_OSC_FREQ_LFO_FINE_EQ:
	    case PARSE_STATE_OSC_PHASE_LFO_EQ:
	    case PARSE_STATE_OSC_PHASE_LFO_AMOUNT_EQ:
	    case PARSE_STATE_OSC_WAVE_LFO_EQ:
	    case PARSE_STATE_OSC_WAVE_LFO_AMOUNT_EQ:
	    case PARSE_STATE_LFO_WAVE_EQ:
	    case PARSE_STATE_LFO_INIT_PHASE_EQ:
	    case PARSE_STATE_LFO_POLARITY_EQ:
	    case PARSE_STATE_LFO_FREQ_BASE_EQ:
	    case PARSE_STATE_LFO_RATE_EQ:
	    case PARSE_STATE_LFO_TRANSPOSE_EQ:
	    case PARSE_STATE_LFO_PITCHBEND_AMOUNT_EQ:
	    case PARSE_STATE_ENV_ATTACK_EQ:
	    case PARSE_STATE_ENV_DECAY_EQ:
	    case PARSE_STATE_ENV_SUSTAIN_EQ:
	    case PARSE_STATE_ENV_RELEASE_EQ:
	    case PARSE_STATE_FILTER_CUTOFF_EQ:
	    case PARSE_STATE_FILTER_RESONANCE_EQ:
	    case PARSE_STATE_FILTER_SMOOTHING_EQ:
	    case PARSE_STATE_FILTER_KEYFOLLOW_EQ:
	    case PARSE_STATE_FILTER_MODE_EQ:
	    case PARSE_STATE_FILTER_TYPE_EQ:
	    case PARSE_STATE_FILTER_GAIN_EQ:
	    case PARSE_STATE_FILTER_ENV_AMOUNT_EQ:
	    case PARSE_STATE_FILTER_ENV_SIGN_EQ:
	    case PARSE_STATE_FILTER_ENV_ATTACK_EQ:
	    case PARSE_STATE_FILTER_ENV_DECAY_EQ:
	    case PARSE_STATE_FILTER_ENV_SUSTAIN_EQ:
	    case PARSE_STATE_FILTER_ENV_RELEASE_EQ:
	    case PARSE_STATE_FILTER_LFO_EQ:
	    case PARSE_STATE_FILTER_LFO_CUTOFF_EQ:
	    case PARSE_STATE_FILTER_LFO_RESONANCE_EQ:
	    case PARSE_STATE_DELAY_MIX_EQ:
	    case PARSE_STATE_DELAY_FEED_EQ:
	    case PARSE_STATE_DELAY_FLIP_EQ:
	    case PARSE_STATE_DELAY_TIME_EQ:
	    case PARSE_STATE_DELAY_LFO_EQ:
	    case PARSE_STATE_CHORUS_MIX_EQ:
	    case PARSE_STATE_CHORUS_FEED_EQ:
	    case PARSE_STATE_CHORUS_FLIP_EQ:
	    case PARSE_STATE_CHORUS_TIME_EQ:
	    case PARSE_STATE_CHORUS_AMOUNT_EQ:
	    case PARSE_STATE_CHORUS_PHASE_RATE_EQ:
	    case PARSE_STATE_CHORUS_PHASE_BALANCE_EQ:
	    case PARSE_STATE_CHORUS_LFO_WAVE_EQ:
	    case PARSE_STATE_CHORUS_LFO_RATE_EQ:
	    case PARSE_STATE_GENERAL_BPM_EQ:
	    case PARSE_STATE_GENERAL_MASTER_TUNE_EQ:
	    case PARSE_STATE_GENERAL_PORTAMENTO_EQ:
	    case PARSE_STATE_GENERAL_MIDI_CHANNEL_EQ:
	    case PARSE_STATE_GENERAL_KEYMODE_EQ:
	    case PARSE_STATE_GENERAL_KEYFOLLOW_VOL_EQ:
	    case PARSE_STATE_GENERAL_VOLUME_EQ:
	    case PARSE_STATE_GENERAL_TRANSPOSE_EQ:
	    case PARSE_STATE_GENERAL_INPUT_BOOST_EQ:
	    case PARSE_STATE_GENERAL_INPUT_FOLLOW_EQ:
	    case PARSE_STATE_GENERAL_PAN_EQ:
	    case PARSE_STATE_GENERAL_STEREO_WIDTH_EQ:
	    case PARSE_STATE_GENERAL_AMP_VELOCITY_EQ:
	    case PARSE_STATE_UNKNOWN_EQ:
		if (strcmp (token, "=") == 0)
		    state += 256;
		else
		    fprintf (stderr, "expecting '=' after attr name in %s, line %d\n",
			     filename, line);
		break;

	    /* looking for an attribute value */

	    /* main oscillator attrs */
	    case PARSE_STATE_OSC_WAVE_VAL:
		id = PARAM_OSC1_WAVE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_wave (token, filename, line);
		update_osc_wave (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_INIT_PHASE_VAL:
		id = PARAM_OSC1_INIT_PHASE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_init_phase (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_POLARITY_VAL:
		id = PARAM_OSC1_POLARITY + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_polarity (token, filename, line);
		update_osc_polarity (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_FREQ_BASE_VAL:
		id = PARAM_OSC1_FREQ_BASE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_freq_base (token, filename, line);
		update_osc_freq_base (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_MODULATION_VAL:
		id = PARAM_OSC1_MODULATION + (18 * cur_osc);
		updated[id] = 1;
		if (strcmp (token, "off") == 0)
		    param[id].cc_val[program_number] = MOD_TYPE_OFF;
		else if (strcmp (token, "mix") == 0)
		    param[id].cc_val[program_number] = MOD_TYPE_MIX;
		else if (strcmp (token, "am") == 0)
		    param[id].cc_val[program_number] = MOD_TYPE_AM;
		else
		    param[id].cc_val[program_number] = MOD_TYPE_NONE;
		update_osc_modulation (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_RATE_VAL:
		id = PARAM_OSC1_RATE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_rate_ctlr (token, filename, line);
		update_osc_rate (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_TRANSPOSE_VAL:
		id = PARAM_OSC1_TRANSPOSE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		state = PARSE_STATE_OSC_ATTR_END;
		update_osc_transpose (NULL, (void *)(&(param[id])));
		break;
	    case PARSE_STATE_OSC_FINE_TUNE_VAL:
		id = PARAM_OSC1_FINE_TUNE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		state = PARSE_STATE_OSC_ATTR_END;
		update_osc_fine_tune (NULL, (void *)(&(param[id])));
		break;
	    case PARSE_STATE_OSC_PITCHBEND_AMOUNT_VAL:
		id = PARAM_OSC1_PITCHBEND + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_pitchbend (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
#ifdef EXPERIMENTAL_CODE
	    case PARSE_STATE_OSC_MIDI_CHANNEL_VAL:
		id = PARAM_OSC1_MIDI_CHANNEL + (18 * cur_osc);
		updated[id] = 1;
		if (strcmp (token, "omni") == 0)
		    param[id].cc_val[program_number] = 16;
		else
		    param[id].cc_val[program_number] = atoi(token);
		if (param[id].cc_val[program_number] > 16)
		    param[id].cc_val[program_number] = 16;
		update_osc_midi_channel (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
#endif
	    case PARSE_STATE_OSC_AM_LFO_VAL:
		id = PARAM_OSC1_AM_LFO + (18 * cur_osc);
		updated[id] = 1;
		if (strcmp (token, "off") == 0)
		    param[id].cc_val[program_number] = 0;
		else if (strcmp (token, "osc-1") == 0)
		    param[id].cc_val[program_number] = 1;
		else if (strcmp (token, "osc-2") == 0)
		    param[id].cc_val[program_number] = 2;
		else if (strcmp (token, "osc-3") == 0)
		    param[id].cc_val[program_number] = 3;
		else if (strcmp (token, "osc-4") == 0)
		    param[id].cc_val[program_number] = 4;
		else if (strcmp (token, "lfo-1") == 0)
		    param[id].cc_val[program_number] = 5;
		else if (strcmp (token, "lfo-2") == 0)
		    param[id].cc_val[program_number] = 6;
		else if (strcmp (token, "lfo-3") == 0)
		    param[id].cc_val[program_number] = 7;
		else if (strcmp (token, "lfo-4") == 0)
		    param[id].cc_val[program_number] = 8;
		else if (strcmp (token, "velocity") == 0)
		    param[id].cc_val[program_number] = 9;
		else
		    param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_am_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_AM_LFO_AMOUNT_VAL:
		id = PARAM_OSC1_AM_LFO_AMOUNT + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_am_lfo_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_FREQ_LFO_VAL:
		id = PARAM_OSC1_FREQ_LFO + (18 * cur_osc);
		updated[id] = 1;
		if (strcmp (token, "off") == 0)
		    param[id].cc_val[program_number] = 0;
		else if (strcmp (token, "osc-1") == 0)
		    param[id].cc_val[program_number] = 1;
		else if (strcmp (token, "osc-2") == 0)
		    param[id].cc_val[program_number] = 2;
		else if (strcmp (token, "osc-3") == 0)
		    param[id].cc_val[program_number] = 3;
		else if (strcmp (token, "osc-4") == 0)
		    param[id].cc_val[program_number] = 4;
		else if (strcmp (token, "lfo-1") == 0)
		    param[id].cc_val[program_number] = 5;
		else if (strcmp (token, "lfo-2") == 0)
		    param[id].cc_val[program_number] = 6;
		else if (strcmp (token, "lfo-3") == 0)
		    param[id].cc_val[program_number] = 7;
		else if (strcmp (token, "lfo-4") == 0)
		    param[id].cc_val[program_number] = 8;
		else if (strcmp (token, "velocity") == 0)
		    param[id].cc_val[program_number] = 9;
		else
		    param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_freq_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_FREQ_LFO_AMOUNT_VAL:
		id = PARAM_OSC1_FREQ_LFO_AMOUNT + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_freq_lfo_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_FREQ_LFO_FINE_VAL:
		id = PARAM_OSC1_FREQ_LFO_FINE + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_freq_lfo_fine (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_PHASE_LFO_VAL:
		id = PARAM_OSC1_PHASE_LFO + (18 * cur_osc);
		updated[id] = 1;
		if (strcmp (token, "off") == 0)
		    param[id].cc_val[program_number] = 0;
		else if (strcmp (token, "osc-1") == 0)
		    param[id].cc_val[program_number] = 1;
		else if (strcmp (token, "osc-2") == 0)
		    param[id].cc_val[program_number] = 2;
		else if (strcmp (token, "osc-3") == 0)
		    param[id].cc_val[program_number] = 3;
		else if (strcmp (token, "osc-4") == 0)
		    param[id].cc_val[program_number] = 4;
		else if (strcmp (token, "lfo-1") == 0)
		    param[id].cc_val[program_number] = 5;
		else if (strcmp (token, "lfo-2") == 0)
		    param[id].cc_val[program_number] = 6;
		else if (strcmp (token, "lfo-3") == 0)
		    param[id].cc_val[program_number] = 7;
		else if (strcmp (token, "lfo-4") == 0)
		    param[id].cc_val[program_number] = 8;
		else if (strcmp (token, "velocity") == 0)
		    param[id].cc_val[program_number] = 9;
		else
		    param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_phase_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_PHASE_LFO_AMOUNT_VAL:
		id = PARAM_OSC1_PHASE_LFO_AMOUNT + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_phase_lfo_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_WAVE_LFO_VAL:
		id = PARAM_OSC1_WAVE_LFO + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line) % (NUM_LFOS + 1);
		update_osc_wave_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;
	    case PARSE_STATE_OSC_WAVE_LFO_AMOUNT_VAL:
		id = PARAM_OSC1_WAVE_LFO_AMOUNT + (18 * cur_osc);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_osc_wave_lfo_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_OSC_ATTR_END;
		break;

	    /* main lfo attrs */
	    case PARSE_STATE_LFO_WAVE_VAL:
		id = PARAM_LFO1_WAVE + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_wave (token, filename, line);
		update_lfo_wave (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_POLARITY_VAL:
		id = PARAM_LFO1_POLARITY + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_polarity (token, filename, line);
		update_lfo_polarity (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_FREQ_BASE_VAL:
		id = PARAM_LFO1_FREQ_BASE + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_freq_base (token, filename, line);
		update_lfo_freq_base (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_RATE_VAL:
		id = PARAM_LFO1_RATE + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_rate_ctlr (token, filename, line);
		update_lfo_rate (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_INIT_PHASE_VAL:
		id = PARAM_LFO1_INIT_PHASE + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_lfo_init_phase (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_TRANSPOSE_VAL:
		id = PARAM_LFO1_TRANSPOSE + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_lfo_transpose (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;
	    case PARSE_STATE_LFO_PITCHBEND_AMOUNT_VAL:
		id = PARAM_LFO1_PITCHBEND + (7 * cur_lfo);
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
		update_lfo_pitchbend (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_LFO_ATTR_END;
		break;

	    /* envelope attrs */
	    case PARSE_STATE_ENV_ATTACK_VAL:
		id = PARAM_AMP_ATTACK;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_amp_attack (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_ENV_ATTR_END;
		break;
	    case PARSE_STATE_ENV_DECAY_VAL:
		id = PARAM_AMP_DECAY;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_amp_decay (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_ENV_ATTR_END;
		break;
	    case PARSE_STATE_ENV_SUSTAIN_VAL:
		id = PARAM_AMP_SUSTAIN;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_amp_sustain (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_ENV_ATTR_END;
		break;
	    case PARSE_STATE_ENV_RELEASE_VAL:
		id = PARAM_AMP_RELEASE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_amp_release (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_ENV_ATTR_END;
		break;

	    /* main filter attrs */
	    case PARSE_STATE_FILTER_CUTOFF_VAL:
		id = PARAM_FILTER_CUTOFF;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_cutoff (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_RESONANCE_VAL:
		id = PARAM_FILTER_RESONANCE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_resonance (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_SMOOTHING_VAL:
		id = PARAM_FILTER_SMOOTHING;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_smoothing (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_KEYFOLLOW_VAL:
		id = PARAM_FILTER_KEYFOLLOW;
		updated[id] = 1;
		if (strcmp (token, "last") == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_LAST;
		else if (strncmp (token, "new", 3) == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_LAST;
		else if (strncmp (token, "high", 4) == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_HIGH;
		else if (strncmp (token, "low", 3) == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_LOW;
		else if (strcmp (token, "keytrig") == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_MIDI;
		else if (strcmp (token, "midi") == 0)
		    param[id].cc_val[program_number] = KEYFOLLOW_MIDI;
		else
		    param[id].cc_val[program_number] = KEYFOLLOW_NONE;
                update_filter_keyfollow (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_MODE_VAL:
		id = PARAM_FILTER_MODE;
		updated[id] = 1;
		if (strcmp (token, "lp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_LP;
		else if (strcmp (token, "hp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_HP;
		else if (strcmp (token, "bp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BP;
		else if (strcmp (token, "bs") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BS;
		else if (strcmp (token, "lp+bp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_LP_PLUS;
		else if (strcmp (token, "lp+") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_LP_PLUS;
		else if (strcmp (token, "hp+bp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_HP_PLUS;
		else if (strcmp (token, "hp+") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_HP_PLUS;
		else if (strcmp (token, "lp+hp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BP_PLUS;
		else if (strcmp (token, "bp+") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BP_PLUS;
		else if (strcmp (token, "bs+bp") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BS_PLUS;
		else if (strcmp (token, "bs+") == 0)
		    param[id].cc_val[program_number] = FILTER_MODE_BS_PLUS;
		else
		    param[id].cc_val[program_number] = FILTER_MODE_LP_PLUS;
                update_filter_mode (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_TYPE_VAL:
		id = PARAM_FILTER_TYPE;
		updated[id] = 1;
		if (strcmp (token, "dist") == 0)
		    param[id].cc_val[program_number] = FILTER_TYPE_DIST;
		else if (strcmp (token, "retro") == 0)
		    param[id].cc_val[program_number] = FILTER_TYPE_RETRO;
		else
		    param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_type (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_GAIN_VAL:
		id = PARAM_FILTER_GAIN;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_gain (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_LFO_VAL:
		id = PARAM_FILTER_LFO;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_LFO_CUTOFF_VAL:
		id = PARAM_FILTER_LFO_CUTOFF;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_lfo_cutoff (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_LFO_RESONANCE_VAL:
		id = PARAM_FILTER_LFO_RESONANCE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_lfo_resonance (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;

	    /* filter envelope attrs */
	    case PARSE_STATE_FILTER_ENV_AMOUNT_VAL:
		id = PARAM_FILTER_ENV_AMOUNT;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_env_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_ENV_SIGN_VAL:
		id = PARAM_FILTER_ENV_SIGN;
		updated[id] = 1;
		if ((strcmp (token, "+") == 0) || (strcmp (token, "positive") == 0)) {
		    param[id].cc_val[program_number] = SIGN_POSITIVE;
		}
		else if ((strcmp (token, "-") == 0) || (strcmp (token, "negative") == 0)) {
		    param[id].cc_val[program_number] = SIGN_NEGATIVE;
		}
		else {
		    param[id].cc_val[program_number] = SIGN_POSITIVE;
		}
                update_filter_env_sign (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_ENV_ATTACK_VAL:
		id = PARAM_FILTER_ATTACK;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_attack (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_ENV_DECAY_VAL:
		id = PARAM_FILTER_DECAY;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_decay (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_ENV_SUSTAIN_VAL:
		id = PARAM_FILTER_SUSTAIN;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_sustain (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;
	    case PARSE_STATE_FILTER_ENV_RELEASE_VAL:
		id = PARAM_FILTER_RELEASE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_filter_release (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_FILTER_ATTR_END;
		break;

	    /* main delay attrs */
	    case PARSE_STATE_DELAY_MIX_VAL:
		id = PARAM_DELAY_MIX;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_delay_mix (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_DELAY_ATTR_END;
		break;
	    case PARSE_STATE_DELAY_FEED_VAL:
		id = PARAM_DELAY_FEED;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_delay_feed (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_DELAY_ATTR_END;
		break;
	    case PARSE_STATE_DELAY_FLIP_VAL:
		id = PARAM_DELAY_FLIP;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_boolean (token, filename, line);
                update_delay_flip (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_DELAY_ATTR_END;
		break;
	    case PARSE_STATE_DELAY_TIME_VAL:
		id = PARAM_DELAY_TIME;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_rate_ctlr (token, filename, line);
                update_delay_time (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_DELAY_ATTR_END;
		break;
	    case PARSE_STATE_DELAY_LFO_VAL:
		id = PARAM_DELAY_LFO;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line) % (NUM_LFOS + 1);
                update_delay_lfo (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_DELAY_ATTR_END;
		break;

	    /* main chorus attrs */
	    case PARSE_STATE_CHORUS_MIX_VAL:
		id = PARAM_CHORUS_MIX;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_chorus_mix (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_FEED_VAL:
		id = PARAM_CHORUS_FEED;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_chorus_feed (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_FLIP_VAL:
		id = PARAM_CHORUS_FLIP;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_boolean (token, filename, line);
                update_chorus_flip (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_TIME_VAL:
		id = PARAM_CHORUS_TIME;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_chorus_time (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_AMOUNT_VAL:
		id = PARAM_CHORUS_AMOUNT;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_chorus_amount (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_PHASE_RATE_VAL:
		id = PARAM_CHORUS_PHASE_RATE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_rate_ctlr (token, filename, line);
                update_chorus_phase_rate (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_PHASE_BALANCE_VAL:
		id = PARAM_CHORUS_PHASE_BALANCE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_chorus_phase_balance (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;

	    /* chorus read lfo attrs */
	    case PARSE_STATE_CHORUS_LFO_WAVE_VAL:
		id = PARAM_CHORUS_LFO_WAVE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_wave (token, filename, line);
                update_chorus_lfo_wave (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;
	    case PARSE_STATE_CHORUS_LFO_RATE_VAL:
		id = PARAM_CHORUS_LFO_RATE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_rate_ctlr (token, filename, line);
                update_chorus_lfo_rate (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_CHORUS_ATTR_END;
		break;

	    /* general patch parameter attrs */
	    case PARSE_STATE_GENERAL_BPM_VAL:
		id = PARAM_BPM;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_bpm (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_MASTER_TUNE_VAL:
		id = PARAM_MASTER_TUNE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_master_tune (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_PORTAMENTO_VAL:
		id = PARAM_PORTAMENTO;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_portamento (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_MIDI_CHANNEL_VAL:
		/* midi is a setting now, not a parameter. */
		/* we still need this parse state for old patches. */
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_KEYMODE_VAL:
		id = PARAM_KEYMODE;
		updated[id] = 1;
		if (strcmp (token, "mono_smooth") == 0)
		    param[id].cc_val[program_number] = KEYMODE_MONO_SMOOTH;
		else if (strcmp (token, "mono_multikey") == 0)
		    param[id].cc_val[program_number] = KEYMODE_MONO_MULTIKEY;
		else if (strncmp (token, "mono_retrig", 11) == 0)
		    param[id].cc_val[program_number] = KEYMODE_MONO_RETRIGGER;
		else if (strncmp (token, "poly", 4) == 0)
		    param[id].cc_val[program_number] = KEYMODE_POLY;
                update_keymode (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_KEYFOLLOW_VOL_VAL:
		id = PARAM_KEYFOLLOW_VOL;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_keyfollow_vol (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_VOLUME_VAL:
		id = PARAM_VOLUME;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_volume (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_TRANSPOSE_VAL:
		id = PARAM_TRANSPOSE;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_transpose (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_INPUT_BOOST_VAL:
		id = PARAM_INPUT_BOOST;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_input_boost (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_INPUT_FOLLOW_VAL:
		id = PARAM_INPUT_FOLLOW;
		updated[id] = 1;
                update_input_follow (NULL, (void *)(&(param[id])));
		param[id].cc_val[program_number] = get_boolean (token, filename, line);
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_PAN_VAL:
		id = PARAM_PAN;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_pan (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_STEREO_WIDTH_VAL:
		id = PARAM_STEREO_WIDTH;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_stereo_width (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_GENERAL_AMP_VELOCITY_VAL:
		id = PARAM_AMP_VELOCITY;
		updated[id] = 1;
		param[id].cc_val[program_number] = get_ctlr (token, filename, line);
                update_amp_velocity (NULL, (void *)(&(param[id])));
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;
	    case PARSE_STATE_UNKNOWN_VAL:
		state = PARSE_STATE_GENERAL_ATTR_END;
		break;


	    /* looking for terminating ';' on an attribute */
	    case PARSE_STATE_OSC_ATTR_END:
	    case PARSE_STATE_LFO_ATTR_END:
	    case PARSE_STATE_ENV_ATTR_END:
	    case PARSE_STATE_FILTER_ATTR_END:
	    case PARSE_STATE_DELAY_ATTR_END:
	    case PARSE_STATE_CHORUS_ATTR_END:
	    case PARSE_STATE_GENERAL_ATTR_END:
		if (strcmp (token, ";") == 0)
		    state -= 16;
		else
		    fprintf (stderr, "expecting ';' but found '%s' after attr value in %s, line %d\n",
			     token, filename, line);
		break;

	    /* at patch end, stay in end state */
	    case PARSE_STATE_PATCH_END:
		break;
	    }
	}
    }

    /* done parsing */
    fclose (patch_f);


    /* start with default param values for unlocked params */
    for (j = 0; j < NUM_PARAMS; j++) {
	if ((!updated[j]) && (!param[j].locked)) {
	    param[j].cc_val[program_number] = param[j].cc_default;
	    param[j].callback (NULL, (gpointer)(&(param[j])));
	}
    }

    patch->modified = 0;

    /* kick filter in case it got NaN'd */
    for (j = 0; j < MAX_VOICES; j++) {
	voice[j].filter_hp1 = 0.0;
	voice[j].filter_hp2 = 0.0;
	voice[j].filter_bp1 = 0.0;
	voice[j].filter_bp2 = 0.0;
	voice[j].filter_lp1 = 0.0;
	voice[j].filter_lp2 = 0.0;
    }

    return 0;
}


/*****************************************************************************
 *
 * SAVE_PATCH()
 *
 * Saves a patch in the same human and machine readable format
 * that patches are loaded in.
 *
 *****************************************************************************/
int
save_patch(char *filename, int program_number) {
    PATCH		*patch			= &(bank[program_number]);
    PATCH_DIR_LIST	*pdir			= patch_dir_list;
    PATCH_DIR_LIST	*ldir			= NULL;
    FILE		*patch_f;
    char		*name;
    char		*tmp;
    int			j;
    int			dir_found		= 0;

    /* return error on missing filename */
    if ((filename == NULL) || (filename[0] == '\0')) {
	return -1;
    }

    /* sanity check filename */
    for (j = 0; filename[j] != '\0'; j++) {
	if (filename[j] < 32) {
	    filename[j] = '_';
	}
    }

    /* open the patch file */
    if ((patch_f = fopen (filename, "wt")) == NULL) {
	if (debug) {
	    fprintf (stderr, "Error opening patch file %s for write: %s\n",
		     filename, strerror (errno));
	}
	return -1;
    }

    /* keep track of filename changes, ignoring dump files */
    if ((filename != patch->filename) && (filename != user_patchdump_file)) {
	if (patch->filename != NULL) {
	    free (patch->filename);
	}
	patch->filename = strdup (filename);

	/* keep track of directory and patch name as well */
	if (patch->directory != NULL) {
	    free (patch->directory);
	}
	tmp = strdup (filename);
	patch->directory = strdup (dirname(tmp));
	memcpy (tmp, filename, (strlen(filename) + 1));
	name = basename (tmp);
	j = strlen (name) - 4;
	if (strcmp (name + j, ".phx") == 0) {
	    name[j] = '\0';
	}
	patch->name = strdup (name);
	free (tmp);

	/* maintain patch directory list */
	if ((patch->directory != NULL) && (*(patch->directory) != '\0') &&
	    (strcmp (patch->directory, PATCH_DIR) != 0) &&
	    (strcmp (patch->directory, user_patch_dir) != 0))
	{
	    while ((pdir != NULL) && (!dir_found)) {
		if (strcmp(pdir->name, patch->directory) == 0) {
		    dir_found = 1;
		}
		else {
		    ldir = pdir;
		    pdir = pdir->next;
		}
	    }
	    if (!dir_found) {
		if ((pdir = malloc (sizeof (PATCH_DIR))) == NULL) {
		    phasex_shutdown("Out of Memory!\n");
		}
		pdir->name = strdup (patch->directory);
		pdir->load_shortcut = pdir->save_shortcut = 0;
		if (ldir == NULL) {
		    pdir->next = NULL;
		    patch_dir_list = pdir;
		}
		else {
		    pdir->next = ldir->next;
		    ldir->next = pdir;
		}
	    }
	}
    }

    /* write the patch in the easy to read format */
    fprintf (patch_f, "phasex_patch {\n");

    /* dump info */
    fprintf (patch_f, "\tinfo {\n");

    fprintf (patch_f, "\t\tversion\t\t\t= %s;\n",	PACKAGE_VERSION);

    fprintf (patch_f, "\t}\n");

    /* dump main parameters */
    fprintf (patch_f, "\tgeneral {\n");


    fprintf (patch_f, "\t\tbpm\t\t\t= %d;\n",		patch->bpm_cc);
    fprintf (patch_f, "\t\tmaster_tune\t\t= %d;\n",	patch->master_tune_cc);
    fprintf (patch_f, "\t\tportamento\t\t= %d;\n",	patch->portamento);
    fprintf (patch_f, "\t\tkeymode\t\t\t= %s;\n",	keymode_names[patch->keymode]);
    fprintf (patch_f, "\t\tkeyfollow_vol\t\t= %d;\n",	patch->keyfollow_vol);
    fprintf (patch_f, "\t\tvolume\t\t\t= %d;\n",	patch->volume_cc);
    fprintf (patch_f, "\t\ttranspose\t\t= %d;\n",	patch->transpose_cc);
    fprintf (patch_f, "\t\tinput_boost\t\t= %d;\n",	patch->input_boost_cc);
    fprintf (patch_f, "\t\tinput_follow\t\t= %s;\n",	boolean_names[patch->input_follow]);
    fprintf (patch_f, "\t\tpan\t\t\t= %d;\n",		patch->pan_cc);
    fprintf (patch_f, "\t\tstereo_width\t\t= %d;\n",	patch->stereo_width_cc);
    fprintf (patch_f, "\t\tamp_velocity\t\t= %d;\n",	patch->amp_velocity_cc);

    fprintf (patch_f, "\t}\n");
    fprintf (patch_f, "\tfilter {\n");

    fprintf (patch_f, "\t\tcutoff\t\t\t= %d;\n",	patch->filter_cutoff_cc);
    fprintf (patch_f, "\t\tresonance\t\t= %d;\n",	patch->filter_resonance_cc);
    fprintf (patch_f, "\t\tsmoothing\t\t= %d;\n",	patch->filter_smoothing);
    fprintf (patch_f, "\t\tkeyfollow\t\t= %s;\n",	keyfollow_names[patch->filter_keyfollow]);
    fprintf (patch_f, "\t\tmode\t\t\t= %s;\n",		filter_mode_names[patch->filter_mode]);
    fprintf (patch_f, "\t\ttype\t\t\t= %d;\n",		patch->filter_type);
    fprintf (patch_f, "\t\tgain\t\t\t= %d;\n",		patch->filter_gain_cc);
    fprintf (patch_f, "\t\tenv_amount\t\t= %d;\n",	patch->filter_env_amount_cc);
    fprintf (patch_f, "\t\tenv_sign\t\t= %s;\n",	sign_names[patch->filter_env_sign_cc]);
    fprintf (patch_f, "\t\tattack\t\t\t= %d;\n",	patch->filter_attack);
    fprintf (patch_f, "\t\tdecay\t\t\t= %d;\n",		patch->filter_decay);
    fprintf (patch_f, "\t\tsustain\t\t\t= %d;\n",	patch->filter_sustain_cc);
    fprintf (patch_f, "\t\trelease\t\t\t= %d;\n",	patch->filter_release);
    fprintf (patch_f, "\t\tlfo\t\t\t= %d;\n",		patch->filter_lfo_cc);
    fprintf (patch_f, "\t\tlfo_cutoff\t\t= %d;\n",	patch->filter_lfo_cutoff_cc);
    fprintf (patch_f, "\t\tlfo_resonance\t\t= %d;\n",	patch->filter_lfo_resonance_cc);

    fprintf (patch_f, "\t}\n");
    fprintf (patch_f, "\tdelay {\n");

    fprintf (patch_f, "\t\tmix\t\t\t= %d;\n",		patch->delay_mix_cc);
    fprintf (patch_f, "\t\tfeed\t\t\t= %d;\n",		patch->delay_feed_cc);
    fprintf (patch_f, "\t\tcrossover\t\t= %s;\n",	boolean_names[patch->delay_flip]);
    fprintf (patch_f, "\t\ttime\t\t\t= %s;\n",		rate_names[patch->delay_time_cc]);
    fprintf (patch_f, "\t\tlfo\t\t\t= %d;\n",		patch->delay_lfo_cc);

    fprintf (patch_f, "\t}\n");
    fprintf (patch_f, "\tchorus {\n");

    fprintf (patch_f, "\t\tmix\t\t\t= %d;\n",		patch->chorus_mix_cc);
    fprintf (patch_f, "\t\tfeed\t\t\t= %d;\n",		patch->chorus_feed_cc);
    fprintf (patch_f, "\t\tcrossover\t\t= %s;\n",	boolean_names[patch->chorus_flip]);
    fprintf (patch_f, "\t\ttime\t\t\t= %d;\n",		patch->chorus_time_cc);
    fprintf (patch_f, "\t\tdepth\t\t\t= %d;\n",		patch->chorus_amount_cc);
    fprintf (patch_f, "\t\tphase_rate\t\t= %s;\n",	rate_names[patch->chorus_phase_rate_cc]);
    fprintf (patch_f, "\t\tphase_balance\t\t= %d;\n",	patch->chorus_phase_balance_cc);
    fprintf (patch_f, "\t\tlfo_wave\t\t= %s;\n",	wave_names[patch->chorus_lfo_wave]);
    fprintf (patch_f, "\t\tlfo_rate\t\t= %s;\n",	rate_names[patch->chorus_lfo_rate_cc]);

    fprintf (patch_f, "\t}\n");
    fprintf (patch_f, "\tenvelope {\n");

    fprintf (patch_f, "\t\tattack\t\t\t= %d;\n",	patch->amp_attack);
    fprintf (patch_f, "\t\tdecay\t\t\t= %d;\n",		patch->amp_decay);
    fprintf (patch_f, "\t\tsustain\t\t\t= %d;\n",	patch->amp_sustain_cc);
    fprintf (patch_f, "\t\trelease\t\t\t= %d;\n",	patch->amp_release);

    fprintf (patch_f, "\t}\n");

    /* dump parameters for each oscillator */
    for (j = 0; j < NUM_OSCS; j++) {

	fprintf (patch_f, "\toscillator {\n");

	fprintf (patch_f, "\t\tmodulation\t\t= %s;\n",		modulation_names[patch->osc_modulation[j]]);
	fprintf (patch_f, "\t\tpolarity\t\t= %s;\n",		polarity_names[patch->osc_polarity_cc[j]]);
	fprintf (patch_f, "\t\tsource\t\t\t= %s;\n",		freq_base_names[patch->osc_freq_base[j]]);
	fprintf (patch_f, "\t\twave\t\t\t= %s;\n",		wave_names[patch->osc_wave[j]]);
	fprintf (patch_f, "\t\tinit_phase\t\t= %d;\n",		patch->osc_init_phase_cc[j]);
	fprintf (patch_f, "\t\trate\t\t\t= %s;\n",		rate_names[patch->osc_rate_cc[j]]);
	fprintf (patch_f, "\t\ttranspose\t\t= %d;\n",		patch->osc_transpose_cc[j]);
	fprintf (patch_f, "\t\tfine_tune\t\t= %d;\n",		patch->osc_fine_tune_cc[j]);
	fprintf (patch_f, "\t\tpitchbend\t\t= %d;\n",		patch->osc_pitchbend_cc[j]);
	fprintf (patch_f, "\t\tam_mod\t\t\t= %s;\n",		modulator_names[patch->am_lfo_cc[j]]);
	fprintf (patch_f, "\t\tam_mod_amount\t\t= %d;\n",	patch->am_lfo_amount_cc[j]);
	fprintf (patch_f, "\t\tfm_mod\t\t\t= %s;\n",		modulator_names[patch->freq_lfo_cc[j]]);
	fprintf (patch_f, "\t\tfm_mod_amount\t\t= %d;\n",	patch->freq_lfo_amount_cc[j]);
	fprintf (patch_f, "\t\tfm_mod_fine\t\t= %d;\n",		patch->freq_lfo_fine_cc[j]);
	fprintf (patch_f, "\t\tpm_mod\t\t\t= %s;\n",		modulator_names[patch->phase_lfo_cc[j]]);
	fprintf (patch_f, "\t\tpm_mod_amount\t\t= %d;\n",	patch->phase_lfo_amount_cc[j]);
	fprintf (patch_f, "\t\twave_lfo\t\t= %d;\n",		patch->wave_lfo_cc[j]);
	fprintf (patch_f, "\t\twave_lfo_amount\t\t= %d;\n",	patch->wave_lfo_amount_cc[j]);

	// not implemented yet
	//fprintf (patch_f, "\t\tchannel\t\t= %d;\n",	patch->osc_channel[j]);

	fprintf (patch_f, "\t}\n");
    }

    /* dump parameters for each lfo */
    for (j = 0; j < NUM_LFOS; j++) {

	fprintf (patch_f, "\tlfo {\n");

	fprintf (patch_f, "\t\tsource\t\t\t= %s;\n",	freq_base_names[patch->lfo_freq_base[j]]);
	fprintf (patch_f, "\t\tpolarity\t\t= %s;\n",	polarity_names[patch->lfo_polarity_cc[j]]);
	fprintf (patch_f, "\t\twave\t\t\t= %s;\n",	wave_names[patch->lfo_wave[j]]);
	fprintf (patch_f, "\t\tinit_phase\t\t= %d;\n",	patch->lfo_init_phase_cc[j]);
	fprintf (patch_f, "\t\trate\t\t\t= %s;\n",	rate_names[patch->lfo_rate_cc[j]]);
	fprintf (patch_f, "\t\ttranspose\t\t= %d;\n",	patch->lfo_transpose_cc[j]);
	fprintf (patch_f, "\t\tpitchbend\t\t= %d;\n",	patch->lfo_pitchbend_cc[j]);

	fprintf (patch_f, "\t}\n");
    }

    fprintf (patch_f, "}\n");

    /* Done writing patch */
    fclose (patch_f);

    /* Mark patch unmodified for non buffer dumps */
    if (strcmp (filename, user_patchdump_file) != 0) {
	patch->modified = 0;
    }

    return 0;
}
