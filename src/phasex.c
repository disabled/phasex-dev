/*****************************************************************************
 *
 * phasex.c
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <asoundlib.h>
#include "phasex.h"
#include "config.h"
#include "jack.h"
#include "engine.h"
#include "wave.h"
#include "filter.h"
#include "patch.h"
#include "param.h"
#include "midi.h"
#include "gtkui.h"
#include "bank.h"
#include "settings.h"
#include "help.h"
#include "lash.h"


/* command line options */
#define HAS_ARG		1
#define NUM_OPTS	(14 + 1)
static struct option long_opts[] = {
    {"undersample",  0,       NULL, 'U'},
    {"oversample",   0,       NULL, 'O'},
    {"input",        HAS_ARG, NULL, 'i'},
    {"output",       HAS_ARG, NULL, 'o'},
    {"port",         HAS_ARG, NULL, 'p'},
    {"bpm",          HAS_ARG, NULL, 'b'},
    {"tuning",       HAS_ARG, NULL, 't'},
    {"midi-channel", HAS_ARG, NULL, 'm'},
    {"fullscreen",   0,       NULL, 'f'},
    {"maximize",     0,       NULL, 'M'},
    {"name",         HAS_ARG, NULL, 'n'},
    {"debug",        0,       NULL, 'd'},
    {"help",         0,       NULL, 'h'},
    {"version",      0,       NULL, 'v'},
    {0, 0, 0, 0}
};


int		debug           = 0;
int		shutdown        = 0;
int		phasex_instance = 0;
char    *phasex_title   = "phasex";

pthread_t	jack_thread_p   = 0;
pthread_t	midi_thread_p   = 0;
pthread_t	engine_thread_p = 0;
pthread_t	gtkui_thread_p  = 0;

char		user_data_dir[PATH_MAX];
char		user_patch_dir[PATH_MAX];
char		user_midimap_dir[PATH_MAX];
char		user_bank_file[PATH_MAX];
char		user_patchdump_file[PATH_MAX];
char		user_midimap_dump_file[PATH_MAX];
char		user_config_file[PATH_MAX];
char		sys_default_patch[PATH_MAX];


/*****************************************************************************
 *
 * SHOWUSAGE()
 *
 *****************************************************************************/
void
showusage(char *argvzero) {
    printf ("usage:  %s [options] [patch-name]\n", argvzero);
    printf ("options:\n");
    printf ("    -f, --fullscreen    start in fullscreen mode.\n");
    printf ("    -M, --maximize      start with main window maximized.\n");
    printf ("    -b, --bpm=          override BPM in patch bank and lock BPM parameter.\n");
    printf ("    -t, --tuning=       base tuning frequency for A4 (defualt 440 Hz).\n");
    printf ("    -m, --midi-channel= global MIDI channel (default 'omni').\n");
    printf ("    -i, --input=        comma separated pair of JACK input matches.\n");
    printf ("    -o, --output=       comma separated pair of JACK output matches.\n");
    printf ("    -p, --port=         set ALSA MIDI port(s) to connect from.\n");
    printf ("    -O, --oversample    use double the sample rate for internal math.\n");
    printf ("    -U, --undersample   use half the sample rate for internal math.\n");
    printf ("    -n, --name=         set instance name (default 'phasex').\n");
    printf ("    -d, --debug         output debug messages on the console.\n");
    printf ("    -v, --version       display version and exit.\n");
    printf ("    -h, --help          display this help message and exit.\n\n");
    printf ("[P]hase [H]armonic [A]dvanced [S]ynthesis [EX]permient %s\n", PACKAGE_VERSION);
    printf ("        (C) 2010 Anton Kormakov <assault64@gmail.com>\n");
    printf ("        (C) 1999-2009 William Weston <weston@sysex.net> and others.\n");
    printf ("Distributed under the terms of the GNU GENERAL Public License, Version 2.\n");
    printf ("                (see AUTHORS and LICENSE for details)\n");
}


/*****************************************************************************
 *
 * GET_INSTANCE_NUM()
 *
 *****************************************************************************/
/*
int
get_instance_num(void) {
    char		buf[1024];
    char		filename[PATH_MAX];
    FILE		*cmdfile;
    DIR			*slashproc;
    struct dirent	*procdir;
    int			j;
    int			i;
    int			instances[20];
    char		*p;
    char		*q;

    for (j = 1; j <= 16; j++) {
	instances[j] = 0;
    }

    if ((slashproc = opendir ("/proc")) == NULL) {
	fprintf (stderr, "Unable to read /proc filesystem!\n");
	exit (1);
    }
    while ((procdir = readdir (slashproc)) != NULL) {
	if (procdir->d_type != DT_DIR) {
	    continue;
	}
	snprintf (filename, PATH_MAX, "/proc/%s/cmdline", procdir->d_name);
	p = q = (char *)(procdir->d_name);
	while (isdigit (*q) && ((q - p) < 8)) {
	    q++;
	}
	if (*q != '\0') {
	    continue;
	}
	if ((cmdfile = fopen (filename, "rt")) == NULL) {
	    continue;
	}
	if (fread (buf, sizeof (char), sizeof (buf), cmdfile) <= 0) {
	    fclose (cmdfile);
	    continue;
	}
	fclose (cmdfile);
	if (strncmp (buf, "phasex-", 7) != 0) {
	    continue;
	}
	i = (10 * (buf[7] - '0')) + (buf[8] - '0');
	if ((i < 1) || (i > 16)) {
	    continue;
	}
	instances[i] = 1;
    }
    closedir (slashproc);
    for (j = 1; j <= 16; j++) {
	if (instances[j] == 0) {
	    return j;
	}
    }

    return -1;
}
*/

/*****************************************************************************
 *
 * CHECK_USER_DATA_DIRS()
 *
 * Build all pathnames based on user's home dir, and build directory
 * structure in user's home directory, if needed.
 *
 *****************************************************************************/
void
check_user_data_dirs(void) {
    DIR			*dir;
    char		*home;
    char		cmd[2048];

    /* look at environment to determine home directory */
    home = getenv ("HOME");
    if (home == NULL) {
	phasex_shutdown ("HOME is not set.  Unable to find user data.\n");
    }

    /* set up user data dir */
    snprintf (user_data_dir, PATH_MAX, "%s/%s", home, USER_DATA_DIR);
    if ((dir = opendir (user_data_dir)) == NULL) {
	if (errno == ENOENT) {
	    if (mkdir (user_data_dir, 0755) != 0) {
		fprintf (stderr, "Unable to create user data directory '%s'.\n", user_data_dir);
		fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
		phasex_shutdown (NULL);
	    }
	    /* copy in system patchbank only when creating new user dir */
	    snprintf (cmd, sizeof (cmd), "/bin/cp %s/patchbank %s/%s",
		     PHASEX_DIR, user_data_dir, USER_BANK_FILE);
	    if (system (cmd) == -1) {
		fprintf (stderr, "Unable to copy '%s/patchbank' to user data directory.\n",
			 PHASEX_DIR);
	    }
	}
	else {
	    fprintf (stderr, "Unable to open user data directory '%s'.\n", user_data_dir);
	    fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
	    phasex_shutdown (NULL);
	}
    }
    else {
	closedir (dir);
    }

    /* check user patch dir, creating if necessary */
    snprintf (user_patch_dir, PATH_MAX, "%s/%s", user_data_dir, USER_PATCH_DIR);
    if ((dir = opendir (user_patch_dir)) == NULL) {
	if (errno == ENOENT) {
	    if (mkdir (user_patch_dir, 0755) != 0) {
		fprintf (stderr, "Unable to create user patch directory '%s'.\n", user_patch_dir);
		fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
		phasex_shutdown (NULL);
	    }
	}
	else {
	    fprintf (stderr, "Unable to open user patch directory '%s'.\n", user_patch_dir);
	    fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
	    phasex_shutdown (NULL);
	}
    }
    else {
	closedir (dir);
    }

    /* check user midimap dir, creating if necessary */
    snprintf (user_midimap_dir, PATH_MAX, "%s/%s", user_data_dir, USER_MIDIMAP_DIR);
    if ((dir = opendir (user_midimap_dir)) == NULL) {
	if (errno == ENOENT) {
	    if (mkdir (user_midimap_dir, 0755) != 0) {
		fprintf (stderr, "Unable to create user midimap directory '%s'.\n", user_midimap_dir);
		fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
		phasex_shutdown (NULL);
	    }
	}
	else {
	    fprintf (stderr, "Unable to open user midimap directory '%s'.\n", user_midimap_dir);
	    fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
	    phasex_shutdown (NULL);
	}
    }
    else {
	closedir (dir);
    }

    snprintf (user_bank_file,         PATH_MAX, "%s/%s",      user_data_dir, USER_BANK_FILE);
    snprintf (user_config_file,       PATH_MAX, "%s/%s",      user_data_dir, USER_CONFIG_FILE);
    snprintf (sys_default_patch,      PATH_MAX, "%s/%s",      PATCH_DIR,     SYS_DEFAULT_PATCH);
}


/*****************************************************************************
 *
 * PHASEX_SHUTDOWN()
 *
 * Main shutdown function.  Can be called from anywhere to cleanly shutdown.
 *
 *****************************************************************************/
void 
phasex_shutdown(const char *msg) {
    pthread_t		self;
    int			missed_thread = 0;

    /* output message from caller */
    if (msg != NULL) {
	fprintf (stderr, msg);
    }

    /* set the global shutdown flag */
    shutdown = 1;

    /* shut down jack client */
    if ((client != NULL) && (jack_thread_p != 0)) {
	jack_client_close (client);
	client = NULL;
	jack_thread_p = 0;
    }

    /* get currently running thread id in order to exit instead of canceling */
    self = pthread_self ();

    /* engine and midi threads are stopped here */
    /* (gtkui and jack threads should shut themselves down) */
    if (midi_thread_p != 0) {
	if (pthread_equal (midi_thread_p, self)) {
	    missed_thread++;
	}
	else {
	    pthread_cancel (midi_thread_p);
	}
    }
    if (engine_thread_p != 0) {
	if (pthread_equal (engine_thread_p, self)) {
	    missed_thread++;
	}
	else {
	    pthread_cancel (engine_thread_p);
	}
    }

    /* if called from a known thread, exit the thread */
    if (missed_thread > 0) {
	pthread_exit (NULL);
    }
    else {
	exit (0);
    }
}


/*****************************************************************************
 *
 * INIT_RT_MUTEX()
 *
 *****************************************************************************/
void
init_rt_mutex(pthread_mutex_t *mutex, int rt) {
    pthread_mutexattr_t		attr;

    /* set attrs for realtime mutexes */
    pthread_mutexattr_init (&attr);
#ifdef HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL
    if (rt) {
	pthread_mutexattr_setprotocol (&attr, PTHREAD_PRIO_INHERIT);
    }
#endif

    /* init mutex with attrs */
    pthread_mutex_init (mutex, &attr);
}


/*****************************************************************************
 *
 * MAIN()
 *
 * Parse command line, load patch, build lookup tables,
 * start engine and jack threads, and wait.
 *
 *****************************************************************************/
int 
main(int argc, char **argv) {
    char		opts[NUM_OPTS * 2 + 1];
    struct option	*op;
    char		*cp;
    char		*alsa_port	= NULL;
    char		*patch_name	= NULL;
    int			c;
    int			j;
    int			ret		= 0;
    int			override_bpm	= 0;
    int			argcount	= argc;
    char		**argvals	= argv;
    char		**envp		= environ;
    char		*argvend	= (char *)argv;
    int			argsize;

    /* lock down memory (rt hates page faults) */
    mlockall (MCL_CURRENT | MCL_FUTURE);

    /* get instance number */
    //phasex_instance = get_instance_num();
    //if (debug) {
	//fprintf (stderr, "PHASEX Instance: %d\n", phasex_instance);
    //}

    /* make sure user data dirs are setup */
    check_user_data_dirs ();

    /* read global configuration before dealing with command line */
    read_settings ();

    /* build the short option string */
    cp = opts;
    for (op = long_opts; op < &long_opts[NUM_OPTS]; op++) {
	*cp++ = op->val;
	if (op->has_arg)
	    *cp++ = ':';
    }

    /* handle options */
    for (;;) {
	c = getopt_long (argc, argv, opts, long_opts, NULL);
	if (c == -1)
	    break;

	switch (c) {
	case 'U':	/* undersample */
	    if (setting_sample_rate_mode == SAMPLE_RATE_OVERSAMPLE) {
		setting_sample_rate_mode = SAMPLE_RATE_NORMAL;
	    }
	    else {
		setting_sample_rate_mode = SAMPLE_RATE_UNDERSAMPLE;
	    }
	    break;
	case 'O':	/* oversample */
	    if (setting_sample_rate_mode == SAMPLE_RATE_UNDERSAMPLE) {
		setting_sample_rate_mode = SAMPLE_RATE_NORMAL;
	    }
	    else {
		setting_sample_rate_mode = SAMPLE_RATE_OVERSAMPLE;
	    }
	    break;
	case 'b':	/* bpm (tempo) */
	    override_bpm = atoi (optarg);
	    if ((override_bpm < 64) || (override_bpm > 191)) {
		override_bpm = 0;
	    }
	    break;
	case 't':	/* tuning frequency */
            setting_tuning_freq = a4freq = atof (optarg);
	    if ((a4freq < 31.0) || (a4freq > 11025.0)) {
                a4freq = A4FREQ;
	    }
	    break;
	case 'f':	/* fullscreen */
	    setting_fullscreen = 1;
	    break;
	case 'M':
	    setting_maximize = 1;
	    setting_fullscreen = 0;
	    break;
	case 'd':	/* debug */
	    debug = 1;
	    break;
	case 'm':	/* MIDI channel */
	    if (strcasecmp (optarg, "omni") == 0) {
		setting_midi_channel = 16;
	    }
	    else {
		setting_midi_channel = atoi (optarg) - 1;
		if ((setting_midi_channel < 0) || (setting_midi_channel > 16)) {
		    setting_midi_channel = 16;
		}
	    }
	    break;
	case 'i':	/* JACK input ports */
	    jack_inputs = strdup (optarg);
	    setting_jack_autoconnect = 0;
	    break;
	case 'o':	/* JACK output ports */
	    jack_outputs = strdup (optarg);
	    setting_jack_autoconnect = 0;
	    break;
	case 'p':	/* ALSA MIDI ports */
	    alsa_port = strdup (optarg);
	    break;
	case 'n':
	    phasex_title = strdup (optarg);
	    break;
	case 'v':	/* version */
	    printf ("phasex-%s\n", PACKAGE_VERSION);
	    return 0;
	case '?':
	case 'h':	/* help */
	default:
	    showusage (argv[0]);
	    //return -1;
	    break;
	}
    }

    /* grab patch name from end of command line */
    if (argv[optind] != NULL) {
	if ((patch_name = strdup (argv[optind])) == NULL) {
	    phasex_shutdown ("Out of memory!\n");
	}
    }

    /* Figure out how much mem we have for process title. */
    /* This includes the orig contiguous mem for argv and envp. */
    //for (j = 0; j <= argcount; j++) {
    //    if ((j == 0) || ((argvend + 1) == argvals[j])) {
    //        argvend = argvals[j] + strlen (argvals[j]);
	//}
    //    else {
    //        continue;
	//}
    //}

    /* steal space from first environment entry */
    //if (envp[0] != NULL) {
	//argvend = envp[0] + strlen (envp[0]);
    //}

    /* calculate size we have for process title */
    //argsize = (char *)argvend - (char *)*argvals - 1;
    //memset (*argvals, 0, argsize);

    /* rewrite process title */
    //argc = 0;
    //snprintf ((char *)*argvals, argsize, "phasex-%02d", phasex_instance);

    /* build the lookup tables */
    build_freq_table ();
    build_freq_shift_table ();
    build_waveform_tables ();
    build_mix_table ();
    build_pan_table ();
    build_gain_table ();
    build_velocity_gain_table ();
    build_keyfollow_table ();

    /* initialize parameter lists */
    init_params ();
    init_param_groups ();
    init_param_pages ();

    /* initialize help system (only after params) */
    init_help ();

    /* build midi controller matrix after init_params() and before midi_thread() */
    build_ccmatrix ();

    /* initialize realtime mutexes */
    init_rt_mutex (&buffer_mutex, 1);
    init_rt_mutex (&sample_rate_mutex, 1);

    /* connect to jack server, retrying for up to 15 seconds */
    for (j = 0; j < 15; j++) {
    phasex_instance = 0;
	if (jack_init () == 0) {
	    break;
	}
	else {
	    if (debug) {
		fprintf (stderr, "Waiting for JACK server...\n");
	    }
	    sleep (1);
	}
    }

    /* shutdown if jack server was not found in 15 seconds */
    if (j == 15) {
	phasex_shutdown ("Unable to conect to JACK server.  Is JACK running?\n");
    }
    
    /* at this point, sample rate and jack buffer size are known */

    /* open midi input */
    if ((midi = open_alsa_midi_in (alsa_port)) == NULL) {
	phasex_shutdown ("Unable to open ALSA MIDI in.\n");
    }

    /* initialize patch, voice, part, and global parameters */
    patch = &(bank[0]);
    init_parameters ();
    
    /* read default patch & midimap files for phasex instance */
    snprintf (user_patchdump_file,    PATH_MAX, "%s/%s-%02d", user_data_dir, USER_PATCHDUMP_FILE,    phasex_instance);
    snprintf (user_midimap_dump_file, PATH_MAX, "%s/%s-%02d", user_data_dir, USER_MIDIMAP_DUMP_FILE, phasex_instance);

    /* Initialize patch and bank */
    init_patch_bank ();
    load_bank ();

    /* handle initial patch from command line */
    if (patch_name != NULL) {
	/* read in patch dump if patch is '-' */
	if (strcmp (patch_name, "-") == 0) {
	    read_patch (user_patchdump_file, program_number);
	}
	/* next, look for initial patch in bank */
	else if ((j = find_patch (patch_name)) != PATCH_BANK_SIZE) {
	    patch          = &(bank[j]);
	    program_number = j;
	}
	/* next, see if patch_name is actually a program number */
	else if (((j = atoi (patch_name)) > 0) && (j < PATCH_BANK_SIZE)) {
	    patch          = &(bank[j]);
	    program_number = j;
	}
	/* fallback to reading the patch from file */
	else {
	    read_patch (patch_name, program_number);
	}
    }

    /* override bpm from command line */
    if (override_bpm > 0) {
	param[PARAM_BPM].locked = 0;
	param[PARAM_BPM].cc_cur = override_bpm - 64;
	for (j = 0; j < PATCH_BANK_SIZE; j++) {
	    param[PARAM_BPM].cc_val[j]  = param[PARAM_BPM].cc_cur;
	    param[PARAM_BPM].int_val[j] = override_bpm;
	}
    }

    /* run the callbacks for all the parameters */
    for (j = 0; j < NUM_PARAMS; j++) {
	param[j].callback (NULL, (gpointer)(&(param[j])));
    }
    patch->modified = 0;

    /* when override_bpm is enabled, lock the bpm parameter */
    if (override_bpm > 0) {
	param[PARAM_BPM].locked = 1;
    }

    /* start midi thread */
    init_rt_mutex (&midi_ready_mutex, 1);
    if ((ret = pthread_create (&midi_thread_p, NULL, &midi_thread, NULL)) != 0) {
	phasex_shutdown ("Unable to start MIDI thread.\n");
    }

    /* Build filter and envelope tables now that sample rate is known */
    build_filter_tables ();
    build_env_tables ();

    /* use midimap from settings, if given */
    if (setting_midimap_file != NULL) {
	read_midimap (setting_midimap_file);
    }

    /* start gtkui thread */
    if ((ret = pthread_create (&gtkui_thread_p, NULL, &gtkui_thread, NULL)) != 0) {
	phasex_shutdown ("Unable to start gtkui thread.\n");
    }

    /* wait until midi thread is ready */
    pthread_mutex_lock (&midi_ready_mutex);
    if (!midi_ready) {
	pthread_cond_wait (&midi_ready_cond, &midi_ready_mutex);
    }
    pthread_mutex_unlock (&midi_ready_mutex);

    /* wait until gtkui thread is ready */
    pthread_mutex_lock (&gtkui_ready_mutex);
    if (!gtkui_ready) {
	pthread_cond_wait (&gtkui_ready_cond, &gtkui_ready_mutex);
    }
    pthread_mutex_unlock (&gtkui_ready_mutex);

    /* start engine thread */
    init_rt_mutex (&engine_ready_mutex, 1);
    if ((ret = pthread_create (&engine_thread_p, NULL, &engine_thread, NULL)) != 0) {
	phasex_shutdown ("Unable to start engine thread.\n");
    }

    /* wait for the engine to be set up and connect jack */
    pthread_mutex_lock (&engine_ready_mutex);
    if (!engine_ready) {
	pthread_cond_wait (&engine_ready_cond, &engine_ready_mutex);
    }
    pthread_mutex_unlock (&engine_ready_mutex);

    if (jack_start () == 0) {
	if (debug) {
	    fprintf (stderr, "Main: Started JACK with client thread 0x%lx\n", jack_thread_p);
	}
    }
    else {
	phasex_shutdown ("Unable to start initial JACK client.  Shutting Down.");
    }

    /* init lash client */
    if (lash_clinit(argc, argv, client, midi->seq) == 0)
    {
    if (debug) {
        fprintf (stderr, "Main: LASH client started.\n");
    }
    }
    else
        fprintf (stderr, "Unable to start lash client.\n");

    /* sit here and restart jack client & poll lash client for events */
    jack_watchdog ();

    /* wait for threads to terminate */
    /* JACK and MIDI threads shouldn't need joining */
    pthread_join (gtkui_thread_p,  NULL);
    pthread_join (engine_thread_p, NULL);

    /* dump the current patch, just in case */
    save_patch (user_patchdump_file, program_number);

    return 0;
}
