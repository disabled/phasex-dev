/*****************************************************************************
 *
 * param.h
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
#ifndef _PHASEX_PARAM_H_
#define _PHASEX_PARAM_H_

#include <gtk/gtk.h>
#include "phasex.h"
#include "gtkknob.h"
#include "bank.h"


/* Parameter IDs */
#define PARAM_BPM			0
#define PARAM_MASTER_TUNE               1
#define PARAM_VOLUME			2
#define PARAM_KEYMODE			3
#define PARAM_KEYFOLLOW_VOL		4
#define PARAM_TRANSPOSE			5
#define PARAM_PORTAMENTO		6
#define PARAM_INPUT_BOOST		7
#define PARAM_INPUT_FOLLOW		8
#define PARAM_PAN			9
#define PARAM_STEREO_WIDTH		10

#define PARAM_AMP_VELOCITY		11
#define PARAM_AMP_ATTACK		12
#define PARAM_AMP_DECAY			13
#define PARAM_AMP_SUSTAIN		14
#define PARAM_AMP_RELEASE		15

#define PARAM_FILTER_CUTOFF		16
#define PARAM_FILTER_RESONANCE		17
#define PARAM_FILTER_SMOOTHING          18
#define PARAM_FILTER_KEYFOLLOW		19
#define PARAM_FILTER_MODE		20
#define PARAM_FILTER_TYPE		21
#define PARAM_FILTER_GAIN		22
#define PARAM_FILTER_ENV_AMOUNT		23
#define PARAM_FILTER_ENV_SIGN		24
#define PARAM_FILTER_LFO		25
#define PARAM_FILTER_LFO_CUTOFF		26
#define PARAM_FILTER_LFO_RESONANCE	27
#define PARAM_FILTER_ATTACK		28
#define PARAM_FILTER_DECAY		29
#define PARAM_FILTER_SUSTAIN		30
#define PARAM_FILTER_RELEASE		31

#define PARAM_CHORUS_MIX		32
#define PARAM_CHORUS_AMOUNT		33
#define PARAM_CHORUS_TIME		34
#define PARAM_CHORUS_PHASE_RATE		35
#define PARAM_CHORUS_PHASE_BALANCE	36
#define PARAM_CHORUS_FEED		37
#define PARAM_CHORUS_LFO_WAVE		38
#define PARAM_CHORUS_LFO_RATE		39
#define PARAM_CHORUS_FLIP		40

#define PARAM_DELAY_MIX			41
#define PARAM_DELAY_FEED		42
#define PARAM_DELAY_FLIP		43
#define PARAM_DELAY_TIME		44
#define PARAM_DELAY_LFO			45

#define PARAM_OSC1_MODULATION		46
#define PARAM_OSC1_WAVE			47
#define PARAM_OSC1_FREQ_BASE		48
#define PARAM_OSC1_RATE			49
#define PARAM_OSC1_POLARITY		50
#define PARAM_OSC1_INIT_PHASE		51
#define PARAM_OSC1_TRANSPOSE		52
#define PARAM_OSC1_FINE_TUNE		53
#define PARAM_OSC1_PITCHBEND		54
#define PARAM_OSC1_AM_LFO		55
#define PARAM_OSC1_AM_LFO_AMOUNT	56
#define PARAM_OSC1_FREQ_LFO		57
#define PARAM_OSC1_FREQ_LFO_AMOUNT	58
#define PARAM_OSC1_FREQ_LFO_FINE	59
#define PARAM_OSC1_PHASE_LFO		60
#define PARAM_OSC1_PHASE_LFO_AMOUNT	61
#define PARAM_OSC1_WAVE_LFO		62
#define PARAM_OSC1_WAVE_LFO_AMOUNT	63

#define PARAM_OSC2_MODULATION		64
#define PARAM_OSC2_WAVE			65
#define PARAM_OSC2_FREQ_BASE		66
#define PARAM_OSC2_RATE			67
#define PARAM_OSC2_POLARITY		68
#define PARAM_OSC2_INIT_PHASE		69
#define PARAM_OSC2_TRANSPOSE		70
#define PARAM_OSC2_FINE_TUNE		71
#define PARAM_OSC2_PITCHBEND		72
#define PARAM_OSC2_AM_LFO		73
#define PARAM_OSC2_AM_LFO_AMOUNT	74
#define PARAM_OSC2_FREQ_LFO		75
#define PARAM_OSC2_FREQ_LFO_AMOUNT	76
#define PARAM_OSC2_FREQ_LFO_FINE	77
#define PARAM_OSC2_PHASE_LFO		78
#define PARAM_OSC2_PHASE_LFO_AMOUNT	79
#define PARAM_OSC2_WAVE_LFO		80
#define PARAM_OSC2_WAVE_LFO_AMOUNT	81

#define PARAM_OSC3_MODULATION		82
#define PARAM_OSC3_WAVE			83
#define PARAM_OSC3_FREQ_BASE		84
#define PARAM_OSC3_RATE			85
#define PARAM_OSC3_POLARITY		86
#define PARAM_OSC3_INIT_PHASE		87
#define PARAM_OSC3_TRANSPOSE		88
#define PARAM_OSC3_FINE_TUNE		89
#define PARAM_OSC3_PITCHBEND		90
#define PARAM_OSC3_AM_LFO		91
#define PARAM_OSC3_AM_LFO_AMOUNT	92
#define PARAM_OSC3_FREQ_LFO		93
#define PARAM_OSC3_FREQ_LFO_AMOUNT	94
#define PARAM_OSC3_FREQ_LFO_FINE	95
#define PARAM_OSC3_PHASE_LFO		96
#define PARAM_OSC3_PHASE_LFO_AMOUNT	97
#define PARAM_OSC3_WAVE_LFO		98
#define PARAM_OSC3_WAVE_LFO_AMOUNT	99

#define PARAM_OSC4_MODULATION		100
#define PARAM_OSC4_WAVE			101
#define PARAM_OSC4_FREQ_BASE		102
#define PARAM_OSC4_RATE			103
#define PARAM_OSC4_POLARITY		104
#define PARAM_OSC4_INIT_PHASE		105
#define PARAM_OSC4_TRANSPOSE		106
#define PARAM_OSC4_FINE_TUNE		107
#define PARAM_OSC4_PITCHBEND		108
#define PARAM_OSC4_AM_LFO		109
#define PARAM_OSC4_AM_LFO_AMOUNT	110
#define PARAM_OSC4_FREQ_LFO		111
#define PARAM_OSC4_FREQ_LFO_AMOUNT	112
#define PARAM_OSC4_FREQ_LFO_FINE	113
#define PARAM_OSC4_PHASE_LFO		114
#define PARAM_OSC4_PHASE_LFO_AMOUNT	115
#define PARAM_OSC4_WAVE_LFO		116
#define PARAM_OSC4_WAVE_LFO_AMOUNT	117

#define PARAM_LFO1_WAVE			118
#define PARAM_LFO1_FREQ_BASE		119
#define PARAM_LFO1_RATE			120
#define PARAM_LFO1_POLARITY		121
#define PARAM_LFO1_INIT_PHASE		122
#define PARAM_LFO1_TRANSPOSE		123
#define PARAM_LFO1_PITCHBEND		124

#define PARAM_LFO2_WAVE			125
#define PARAM_LFO2_FREQ_BASE		126
#define PARAM_LFO2_RATE			127
#define PARAM_LFO2_POLARITY		128
#define PARAM_LFO2_INIT_PHASE		129
#define PARAM_LFO2_TRANSPOSE		130
#define PARAM_LFO2_PITCHBEND		131

#define PARAM_LFO3_WAVE			132
#define PARAM_LFO3_FREQ_BASE		133
#define PARAM_LFO3_RATE			134
#define PARAM_LFO3_POLARITY		135
#define PARAM_LFO3_INIT_PHASE		136
#define PARAM_LFO3_TRANSPOSE		137
#define PARAM_LFO3_PITCHBEND		138

#define PARAM_LFO4_WAVE			139
#define PARAM_LFO4_FREQ_BASE		140
#define PARAM_LFO4_RATE			141
#define PARAM_LFO4_POLARITY		142
#define PARAM_LFO4_INIT_PHASE		143
#define PARAM_LFO4_TRANSPOSE		144
#define PARAM_LFO4_PITCHBEND		145

/* Update NUM_PARAMS after adding or removing parameters */
#define NUM_PARAMS			146

/* The following only behave like parameters for the help system */
#define PARAM_MIDI_CHANNEL		146
#define PARAM_PROGRAM_NUMBER		147
#define PARAM_PATCH_NAME		148
#define PARAM_BANK_MEM_MODE		149

/* Main help for PHASEX */
#define PARAM_PHASEX_HELP		150

/* Update NUM_HELP_PARAMS after adding or removing parameters */
#define NUM_HELP_PARAMS			151

/* Update NUM_PARAM_GROUPS after changing groups in param.c */
#define NUM_PARAM_GROUPS		22

/* Update NUM_PARAM_PAGES after changing paes in param.c */
#define NUM_PARAM_PAGES			2

/* Parameter types */
#define PARAM_TYPE_HELP			0
#define PARAM_TYPE_INT			1
#define PARAM_TYPE_REAL			2
#define PARAM_TYPE_LIST			3
#define PARAM_TYPE_BOOL			4
#define PARAM_TYPE_RATE			5
#define PARAM_TYPE_BBOX			6
#define PARAM_TYPE_DTNT			7


typedef void (*PHASEX_CALLBACK) (GtkWidget *widget, gpointer *data);


/* Parameter struct for handling conversion and value passing */
typedef struct phasex_param {
    const char		*name;		/* Parameter name for patches		*/
    const char		*label;		/* Parameter label for GUI		*/
    int			id;		/* Param unique ID.  See defs above.	*/
    int			type;		/* Integer, real, or list values	*/
    int			cc_num;		/* MIDI controller number		*/
    int			cc_limit;	/* Upper bound for MIDI controller	*/
    int			leap;		/* leap (moderately large step) size	*/
    int			pre_inc;	/* Pre-scaling constant adjustment	*/
    int			index;		/* Index for arrayed parameters		*/
    int			updated;	/* Flag set by non-gui threads		*/
    int			locked;		/* Allow only user-explicit updates	*/
    int			cc_cur;		/* Current  MIDI ctlr value		*/
    int			cc_prev;	/* Previous MIDI ctlr value		*/
    int			cc_default;	/* Default MIDI ctlr value		*/
    int			cc_val[PATCH_BANK_SIZE];	/* Current MIDI ctlr value	*/
    int			int_val[PATCH_BANK_SIZE];	/* Current integer param value	*/
    PHASEX_CALLBACK	callback;	/* Universal parameter update callback	*/
    GtkKnob		*knob;		/* Widget that holds param value	*/
    GtkObject		*adj;		/* Widget that holds param value	*/
    GtkWidget		*spin;		/* Widget that holds param value	*/
    GtkWidget		*combo;		/* Widget that holds param value	*/
    GtkWidget		*text;		/* Widget that holds param value	*/
    GtkWidget		*button[12];	/* Widgets that hold param value	*/
    const gchar		**list_labels;	/* List for list based parameters	*/
} PARAM;

/* Parameter group, with array of parameter IDs */
typedef struct param_group {
    gchar		*label;		/* Frame label for parameter group	*/
    int			full_x;		/* Table column in fullscreen layout	*/
    int			full_y0;	/* Table row start in fullscreen layout	*/
    int			full_y1;	/* Table row end in fullscreen layout	*/
    int			notebook_page;	/* Tab number, starting at 0		*/
    int			notebook_x;	/* Table column in notebook layout	*/
    int			notebook_y0;	/* Table row start in notebook layout	*/
    int			notebook_y1;	/* Table row end in notebook layout	*/
    int			param_list[16];	/* List of up to 15 parameters		*/
} PARAM_GROUP;

/* Parameter page definition */
typedef struct param_page {
    gchar		*label;		/* Label for GUI page			*/
} PARAM_PAGE;


/* List of labels for parameter values in GUI */
extern const char *midi_ch_labels[];
extern const char *on_off_labels[];
extern const char *freq_base_labels[];
extern const char *mod_type_labels[];
extern const char *keymode_labels[];
extern const char *keyfollow_labels[];
extern const char *polarity_labels[];
extern const char *sign_labels[];
extern const char *wave_labels[];
extern const char *filter_mode_labels[];
extern const char *lfo_labels[];
extern const char *rate_labels[];


/* All parameters belong in this array */
extern PARAM		param[NUM_HELP_PARAMS];

/* Parameter groups and pages reference paramenters */
extern PARAM_GROUP	param_group[NUM_PARAM_GROUPS];
extern PARAM_PAGE	param_page[NUM_PARAM_PAGES];

extern char		*midimap_filename;
extern int		midimap_modified;


/* Functions to initialize global arrays */
void init_param(int id, const char *name, const char *label, int type,
		int cc, int lim, int ccv, int pre, int ix, int leap,
		PHASEX_CALLBACK callback, const char *label_list[]);
void init_params(void);
void init_param_groups(void);
void init_param_pages(void);
int read_midimap(char *filename);
int save_midimap(char *filename);


#endif /* _PHASEX_PARAM_H_ */
