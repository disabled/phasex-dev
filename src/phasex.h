/*****************************************************************************
 *
 * phasex.h
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
 *****************************************************************************/
#ifndef _PHASEX_H_
#define _PHASEX_H_

#include <pthread.h>
#include <sys/param.h>


/* Set default cpu class, if not defined by build system */
/* 1 is for old painfully slow hardware */
/* 2 is for average hardware capable of running current linux desktop distros */
/* 3 is for the newer multi-core and other incredibly fast chips */
#if !defined(PHASEX_CPU_POWER)
# define PHASEX_CPU_POWER	2
#endif

/* type to use for all floating point math */
/* changing this to double will impact performance, and is probably overkill */
typedef float sample_t;


/*****************************************************************************
 *
 * Tunable Compile Time Constants
 *
 *****************************************************************************/

/* Default time for gui to sleep in idle function (in microseconds) */
#if (PHASEX_CPU_POWER == 1)
# define DEFAULT_GUI_IDLE_SLEEP	125000
#endif
#if (PHASEX_CPU_POWER == 2)
# define DEFAULT_GUI_IDLE_SLEEP	80000
#endif
#if (PHASEX_CPU_POWER == 3)
# define DEFAULT_GUI_IDLE_SLEEP	50000
#endif

/* Default realtime thread priorities					*/
/* These can be changed at runtime in the preferences			*/
#define MIDI_THREAD_PRIORITY	89
#define ENGINE_THREAD_PRIORITY	88

/* Base tuning frequency, in Hz						*/
/* Use the -t command line flag to override at runtime			*/
#define A4FREQ			440

/* Number of micro-tuning steps between halfsteps			*/
/* A value of 120 seems to provide good harmonics			*/
#define TUNING_RESOLUTION	120
#define F_TUNING_RESOLUTION	120.0		/* must equal above */

/* Factor by which the filter is oversampled.				*/
/* Harmonics are highly dependent on the prime factors in this value.	*/
/* Increase for richer harmonics and more stability at high resonance.	*/
/* Decrease to save CPU cycles or for thinner harmonics.		*/
/* 6x oversampling seems to provide good harmonics at reasonable cost.	*/
#if (PHASEX_CPU_POWER == 1)
# define FILTER_OVERSAMPLE	2
# define F_FILTER_OVERSAMPLE	2.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 2)
# define FILTER_OVERSAMPLE	6
# define F_FILTER_OVERSAMPLE	6.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 3)
# define FILTER_OVERSAMPLE	12
# define F_FILTER_OVERSAMPLE	12.0		/* must equal above */
#endif

/* Number of samples for a single wave period in the osc wave table.	*/
/* Larger values sound cleaner, but burn more memory.			*/
/* Smaller values sound grittier, but burn less memory.			*/
/* On some CPUs, performance degrades with larger lookup tables.	*/
/* Values with more small prime factors provide better harmonics	*/
/*   than large powers of 2.  Try these values first:			*/
/* 24576 28800 32400 44100 48000 49152 57600 64800 75600 73728 86400	*/
/* 88200 96000 97200 98304 115200 129600 132300 144000 162000 176400	*/
#if (PHASEX_CPU_POWER == 1)
# define WAVEFORM_SIZE		32400
# define F_WAVEFORM_SIZE	32400.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 2)
# define WAVEFORM_SIZE		32400
# define F_WAVEFORM_SIZE	32400.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 3)
# define WAVEFORM_SIZE		75600
# define F_WAVEFORM_SIZE	75600.0		/* must equal above */
#endif

/* Size of the lookup table for logarithmic envelope curves.		*/
/* envelope curve table size shouldn't drop much below 14400		*/
#if (PHASEX_CPU_POWER == 1)
# define ENV_CURVE_SIZE		28800
# define F_ENV_CURVE_SIZE	28800.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 2)
# define ENV_CURVE_SIZE		32400
# define F_ENV_CURVE_SIZE	32400.0		/* must equal above */
#endif
#if (PHASEX_CPU_POWER == 3)
# define ENV_CURVE_SIZE		57600
# define F_ENV_CURVE_SIZE	57600.0		/* must equal above */
#endif

/* Total polyphony */
/* As a general rule of thumb, this should be somewhat lower than	*/
/*   one voice per 100 MHz CPU power.  YMMV.				*/
#if (PHASEX_CPU_POWER == 1)
# define DEFAULT_POLYPHONY	6
#endif
#if (PHASEX_CPU_POWER == 2)
# define DEFAULT_POLYPHONY	12
#endif
#if (PHASEX_CPU_POWER == 3)
# define DEFAULT_POLYPHONY	16
#endif

/* turn on experimental hermite interpolation on chorus buffer reads */
/* somewhat expensive, so don't enable for slow CPUs */
#if PHASEX_CPU_POWER >= 2
# define INTERPOLATE_CHORUS
#endif

/* Default realtime scheduling policy for midi and engine threads */
#define PHASEX_SCHED_POLICY	SCHED_RR

/* User files and directories for configs, patches, midimaps, etc.	*/
/* Since these are not full paths, any new code should reference the	*/
/*   globals provided at the bottom of this file instead.		*/
#define USER_DATA_DIR		".phasex"
#define USER_PATCH_DIR		"user-patches"
#define USER_MIDIMAP_DIR	"user-midimaps"
#define USER_BANK_FILE		"patchbank"
#define USER_PATCHDUMP_FILE	"patchdump"
#define USER_MIDIMAP_DUMP_FILE	"midimapdump"
#define USER_CONFIG_FILE	"phasex.cfg"
#define SYS_DEFAULT_PATCH	"default.phx"

/* Default font (overridden with font setting) */
#define PHASEX_DEFAULT_FONT	"Sans 9"

/* Default GTK theme engine (change this in gtkenginerc as well) */
#define PHASEX_GTK_ENGINE	"nodoka"

/* define this if there are problems when disconnecting from JACK */
//define JACK_DEACTIVATE_BEFORE_CLOSE

/* define this if JACK reconnect code seems broken */
//define JOIN_JACK_CLIENT_THREAD

/* Misc options */
#define RESTART_THREAD_WITH_GUI	/* restart gtkui thread with gtkui restart? */
#define CUSTOM_FONTS_IN_MENUS	/* apply user selectable fonts to menus too? */
//define MIDI_CHANNEL_IN_MENU	/* add midi channel submenu to midi menu? */
//define BANK_MEM_MODE_IN_SUBMENU /* add bank mem mode submenu to patch menu? */

/* Debug options */
//define EXTRA_DEBUG		/* extra noisy debug */
//define DEBUG_SAMPLE_VALUES	/* very very noisy debug */
//define IGNORE_SAMPLES		/* skip sample loading for fast start-up */


/*****************************************************************************
 *
 * Non-Tunable Constants
 *
 *****************************************************************************/

/* Number of per-voice oscs and per-part lfos */
#define NUM_OSCS	       	4
#define NUM_LFOS	       	4

/* The Off/Velocity LFOs and Oscilators get their own slot */
#define LFO_OFF			NUM_LFOS
#define MOD_OFF			NUM_OSCS
#define LFO_VELOCITY		NUM_LFOS
#define MOD_VELOCITY		NUM_OSCS

/* modulator types */
#define MOD_TYPE_OSC		0
#define MOD_TYPE_LFO		1
#define MOD_TYPE_VELOCITY	2

/* Hard polyphony limit */
#define MAX_VOICES		32

/* three options for sample rate: 1, 1/2, or 2 */
#define SAMPLE_RATE_NORMAL	0
#define SAMPLE_RATE_UNDERSAMPLE	1
#define SAMPLE_RATE_OVERSAMPLE	2

/* Update after adding new waveforms */
#define NUM_WAVEFORMS		18

/* Maximum delay time, in samples */
#define DELAY_MAX		720000
#define CHORUS_MAX		16384

/* Even-multiple octaves work best. */
/* Must be able to handle patch transpose + part transpose + pitchbend + fm */
#define FREQ_SHIFT_HALFSTEPS	384
#define F_FREQ_SHIFT_HALFSTEPS	384.0
#define FREQ_SHIFT_TABLE_SIZE	(FREQ_SHIFT_HALFSTEPS * TUNING_RESOLUTION)
#define F_FREQ_SHIFT_TABLE_SIZE	(F_FREQ_SHIFT_HALFSTEPS * F_TUNING_RESOLUTION)
#define FREQ_SHIFT_ZERO_OFFSET	(F_FREQ_SHIFT_TABLE_SIZE * 0.5)

/* max number of samples to use in the ringbuffer */
#define PHASEX_MAX_BUFSIZE	8192

/* Compile fixups */
#if !defined(PATH_MAX)
# define PATH_MAX		1024
#endif


/* globals from phasex.c */
extern int debug;
extern int phasex_instance;
extern char *phasex_title;
extern int shutdown;

extern pthread_t jack_thread_p;
extern pthread_t midi_thread_p;
extern pthread_t engine_thread_p;
extern pthread_t gtkui_thread_p;

extern char user_data_dir[PATH_MAX];
extern char user_patch_dir[PATH_MAX];
extern char user_midimap_dir[PATH_MAX];
extern char user_bank_file[PATH_MAX];
extern char user_patchdump_file[PATH_MAX];
extern char user_midimap_dump_file[PATH_MAX];
extern char user_config_file[PATH_MAX];
extern char sys_default_patch[PATH_MAX];


/* functions from phasex.c */
void phasex_shutdown(const char *msg);


#endif /* _PHASEX_H_ */
