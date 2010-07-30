/*****************************************************************************
 *
 * settings.c
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <math.h>
#include <gtk/gtk.h>
#include "phasex.h"
#include "config.h"
#include "patch.h"
#include "param.h"
#include "wave.h"
#include "filter.h"
#include "midi.h"
#include "jack.h"
#include "gtkui.h"
#include "bank.h"
#include "gtkknob.h"
#include "callback.h"
#include "settings.h"


int			setting_window_layout		= LAYOUT_ONE_PAGE;
int			setting_fullscreen		= 0;
int			setting_maximize		= 0;
int			setting_backing_store		= 1;
int			setting_jack_autoconnect	= 1;
int			setting_sample_rate_mode	= SAMPLE_RATE_NORMAL;
int			setting_bank_mem_mode		= BANK_MEM_WARN;
int			setting_gui_idle_sleep		= DEFAULT_GUI_IDLE_SLEEP;
int			setting_midi_priority		= MIDI_THREAD_PRIORITY;
int			setting_engine_priority		= ENGINE_THREAD_PRIORITY;
int			setting_sched_policy		= PHASEX_SCHED_POLICY;
int			setting_polyphony		= DEFAULT_POLYPHONY;
int			setting_theme			= PHASEX_THEME_DARK;
int			setting_midi_channel		= 16; // Omni

double			setting_tuning_freq		= A4FREQ;

char			*setting_midimap_file		= NULL;
char			*setting_custom_theme		= NULL;
char			*setting_font			= NULL;

PangoFontDescription	*phasex_font_desc		= NULL;

GtkWidget		*config_dialog			= NULL;
GtkWidget		*custom_theme_button		= NULL;


char *sample_rate_mode_names[] = {
    "normal",
    "undersample",
    "oversample",
    NULL
};

char *bank_mode_names[] = {
    "autosave",
    "warn",
    "protect",
    NULL
};

char *layout_names[] = {
    "one_page",
    "notebook",
    NULL
};

char *theme_names[] = {
    "dark",
    "light",
    "system",
    "custom",
    NULL
};


/*****************************************************************************
 *
 * READ_SETTINGS()
 *
 *****************************************************************************/
void
read_settings(void) {
    FILE	*config_f;
    char	*p;
    char	setting_name[64];
    char	setting_value[1024];
    char	buffer[1024];
    char	c;
    int		prio;
    int		line = 0;

    /* open the config file */
    if ((config_f = fopen (user_config_file, "rt")) == NULL) {
	return;
    }

    /* read config settings */
    while (fgets (buffer, sizeof (buffer), config_f) != NULL) {
	line++;

	/* discard comments and blank lines */
	if ((buffer[0] == '\n') || (buffer[0] == '#')) {
	    continue;
	}

	/* strip comments */
	p = buffer;
	while ((p < (buffer + sizeof (buffer))) && ((c = *p) != '\0')) {
	    if (c == '#') {
		*p = '\0';
	    }
	    p++;
	}

	/* get setting name */
	if ((p = get_next_token (buffer)) == NULL) {
	    continue;
	}
	strncpy (setting_name, p, sizeof (setting_name));
	setting_name[sizeof (setting_name) - 1] = '\0';

	/* make sure there's an '=' */
	if ((p = get_next_token (buffer)) == NULL) {
	    continue;
	}
	if (strcmp(p, "=") != 0) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}

	/* get setting value */
	if ((p = get_next_token (buffer)) == NULL) {
	    continue;
	}
	strncpy (setting_value, p, sizeof (setting_value));
	setting_value[sizeof (setting_value) - 1] = '\0';

	/* make sure there's a ';' */
	if ((p = get_next_token (buffer)) == NULL) {
	    continue;
	}
	if (strcmp(p, ";") != 0) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}

	/* flush remainder of line */
	while (get_next_token (buffer) != NULL);

	/* find out which setting this is and set the value */
	if (strcasecmp (setting_name, "midi_channel") == 0) {
	    if (strcasecmp (setting_value, "omni") == 0) {
		setting_midi_channel = 16;
	    }
	    else {
		setting_midi_channel = atoi (setting_value) - 1;
		if ((setting_midi_channel < 0) || (setting_midi_channel > 16)) {
		    setting_midi_channel = 16;
		}
	    }
	}

	else if (strcasecmp (setting_name, "fullscreen") == 0) {
	    setting_fullscreen = get_boolean (setting_value, NULL, 0);
	}

	else if (strcasecmp (setting_name, "maximize") == 0) {
	    setting_maximize = get_boolean (setting_value, NULL, 0);
	}

	else if (strcasecmp (setting_name, "window_layout") == 0) {
	    if (strcasecmp (setting_value, "one_page")) {
		setting_window_layout = LAYOUT_ONE_PAGE;
	    }
	    if (strcasecmp (setting_value, "notebook")) {
		setting_window_layout = LAYOUT_NOTEBOOK;
	    }
	}

	else if (strcasecmp (setting_name, "jack_autoconnect") == 0) {
	    setting_jack_autoconnect = get_boolean (setting_value, NULL, 0);
	}

	else if (strcasecmp (setting_name, "sample_rate_mode") == 0) {
	    if (strcasecmp (setting_value, "normal") == 0) {
		setting_sample_rate_mode = SAMPLE_RATE_NORMAL;
	    }
	    else if (strcasecmp (setting_value, "undersample") == 0) {
		setting_sample_rate_mode = SAMPLE_RATE_UNDERSAMPLE;
	    }
	    else if (strcasecmp (setting_value, "oversample") == 0) {
		setting_sample_rate_mode = SAMPLE_RATE_OVERSAMPLE;
	    }
	    else {
		setting_sample_rate_mode = SAMPLE_RATE_NORMAL;
	    }
	    sample_rate_mode = setting_sample_rate_mode;
	}

	else if (strcasecmp (setting_name, "bank_mem_mode") == 0) {
	    if (strcasecmp (setting_value, "autosave") == 0) {
		setting_bank_mem_mode = BANK_MEM_AUTOSAVE;
	    }
	    else if (strcasecmp (setting_value, "warn") == 0) {
		setting_bank_mem_mode = BANK_MEM_WARN;
	    }
	    else if (strcasecmp (setting_value, "protect") == 0) {
		setting_bank_mem_mode = BANK_MEM_PROTECT;
	    }
	    else {
		setting_bank_mem_mode = BANK_MEM_WARN;
	    }
	}

	else if (strcasecmp (setting_name, "gui_idle_sleep") == 0) {
	    setting_gui_idle_sleep = atoi (setting_value);
	    if (setting_gui_idle_sleep < 20000) {
		setting_gui_idle_sleep = 20000;
	    }
	    else if (setting_gui_idle_sleep > 200000) {
		setting_gui_idle_sleep = 200000;
	    }
	}

	else if (strcasecmp (setting_name, "midi_thread_priority") == 0) {
	    setting_midi_priority = atoi (setting_value);
	    prio = sched_get_priority_min (PHASEX_SCHED_POLICY);
	    if (setting_midi_priority < prio) {
		setting_midi_priority = prio;
	    }
	    else {
		prio = sched_get_priority_max (PHASEX_SCHED_POLICY);
		if (setting_midi_priority > prio) {
		    setting_midi_priority = prio;
		}
	    }
	}

	else if (strcasecmp (setting_name, "engine_thread_priority") == 0) {
	    setting_engine_priority = atoi (setting_value);
	    prio = sched_get_priority_min (PHASEX_SCHED_POLICY);
	    if (setting_engine_priority < prio) {
		setting_engine_priority = prio;
	    }
	    else {
		prio = sched_get_priority_max (PHASEX_SCHED_POLICY);
		if (setting_engine_priority > prio) {
		    setting_engine_priority = prio;
		}
	    }
	}

	else if (strcasecmp (setting_name, "sched_policy") == 0) {
	    if (strcasecmp (setting_value, "sched_fifo") == 0) {
		setting_sched_policy = SCHED_FIFO;
	    }
	    else if (strcasecmp (setting_value, "sched_rr") == 0) {
		setting_sched_policy = SCHED_RR;
	    }
	    else {
		setting_sched_policy = PHASEX_SCHED_POLICY;
	    }
	}

	else if (strcasecmp (setting_name, "midimap_file") == 0) {
	    setting_midimap_file = strdup (setting_value);
	}

	else if (strcasecmp (setting_name, "polyphony") == 0) {
	    setting_polyphony = atoi (setting_value);
	    if (setting_polyphony <= 0) {
		setting_polyphony = DEFAULT_POLYPHONY;
	    }
	}

	else if (strcasecmp (setting_name, "tuning_freq") == 0) {
	    setting_tuning_freq = a4freq = (sample_t)atof (setting_value);
	}

	else if (strcasecmp (setting_name, "backing_store") == 0) {
	    setting_backing_store = get_boolean (setting_value, NULL, 0);
	}

	else if (strcasecmp (setting_name, "theme") == 0) {
	    if (strcasecmp (setting_value, "system") == 0) {
		setting_theme = PHASEX_THEME_SYSTEM;
	    }
	    else if (strcasecmp (setting_value, "dark") == 0) {
		setting_theme = PHASEX_THEME_DARK;
	    }
	    else if (strcasecmp (setting_value, "light") == 0) {
		setting_theme = PHASEX_THEME_LIGHT;
	    }
	    else if (strcasecmp (setting_value, "custom") == 0) {
		setting_theme = PHASEX_THEME_CUSTOM;
	    }
	    else {
		setting_theme = PHASEX_THEME_DARK;
	    }
	}

	else if (strcasecmp (setting_name, "custom_theme") == 0) {
	    setting_custom_theme = strdup (setting_value);
	}

	else if (strcasecmp (setting_name, "font") == 0) {
	    setting_font = strdup (setting_value);
	    phasex_font_desc = pango_font_description_from_string (setting_font);
	}
    }

    /* done parsing */
    fclose (config_f);

    return;
}


/*****************************************************************************
 *
 * SAVE_SETTINGS()
 *
 *****************************************************************************/
void
save_settings(void) {
    FILE	*config_f;

    /* open the config file */
    if ((config_f = fopen (user_config_file, "wt")) == NULL) {
	return;
    }

    /* write the settings */
    fprintf (config_f, "# PHASEX %s Configuration\n",     PACKAGE_VERSION);
    fprintf (config_f, "midi_channel\t\t= %s;\n",	  midi_ch_labels[setting_midi_channel]);
    fprintf (config_f, "window_layout\t\t= %s;\n",        layout_names[setting_window_layout]);
    fprintf (config_f, "fullscreen\t\t= %s;\n",           boolean_names[setting_fullscreen]);
    fprintf (config_f, "maximize\t\t= %s;\n",		  boolean_names[setting_maximize]);
    fprintf (config_f, "backing_store\t\t= %s;\n",        boolean_names[setting_backing_store]);
    fprintf (config_f, "jack_autoconnect\t= %s;\n",       boolean_names[setting_jack_autoconnect]);
    fprintf (config_f, "sample_rate_mode\t= %s;\n",       sample_rate_mode_names[setting_sample_rate_mode]);
    fprintf (config_f, "bank_mem_mode\t\t= %s;\n",        bank_mode_names[setting_bank_mem_mode]);
    fprintf (config_f, "tuning_freq\t\t= %f;\n",          setting_tuning_freq);
    fprintf (config_f, "polyphony\t\t= %d;\n",		  setting_polyphony);
    fprintf (config_f, "gui_idle_sleep\t\t= %d;\n",       setting_gui_idle_sleep);
    fprintf (config_f, "midi_thread_priority\t= %d;\n",   setting_midi_priority);
    fprintf (config_f, "engine_thread_priority\t= %d;\n", setting_engine_priority);
    fprintf (config_f, "sched_policy\t\t= %s;\n",         ((setting_sched_policy == SCHED_RR) ? "sched_rr" : "sched_fifo"));
    fprintf (config_f, "font\t\t\t= \"%s\";\n",           setting_font);
    fprintf (config_f, "theme\t\t\t= \"%s\";\n",          theme_names[setting_theme]);
    if (setting_custom_theme != NULL) {
	fprintf (config_f, "custom_theme\t\t= \"%s\";\n", setting_custom_theme);
    }
    if (setting_midimap_file != NULL) {
	fprintf (config_f, "midimap_file\t\t= \"%s\";\n", setting_midimap_file);
    }

    /* done */
    fclose (config_f);
}


/*****************************************************************************
 *
 * SET_MIDI_CHANNEL()
 *
 *****************************************************************************/
void
set_midi_channel(GtkWidget *widget, gpointer data, GtkWidget *widget2) {
    int		new_channel    = (long int)data;

    /* called from menu */
    if (widget == NULL) {
	if ((widget2 == NULL) || !GTK_IS_CHECK_MENU_ITEM (widget2) ||
	    !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget2))) {
	    return;
	}
    }

    /* called from knob */
    else {
	new_channel = (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));
	if ((midi_channel_label != NULL) && (GTK_IS_LABEL (midi_channel_label))) {
	    gtk_label_set_text (GTK_LABEL (midi_channel_label), midi_ch_labels[new_channel]);
	}
    }

    /* only deal with real changes */
    if (setting_midi_channel != new_channel) {

	setting_midi_channel = new_channel;

	/* set menu radiobutton */
	if ((menu_item_midi_ch[new_channel] != NULL) &&
	    !gtk_check_menu_item_get_active (menu_item_midi_ch[new_channel])) {
	    gtk_check_menu_item_set_active (menu_item_midi_ch[new_channel], TRUE);
	}

	/* set adjustment for spin button */
	gtk_adjustment_set_value (GTK_ADJUSTMENT (midi_channel_adj), setting_midi_channel);

	save_settings ();
    }
}


/*****************************************************************************
 *
 * SET_FULLSCREEN_MODE()
 *
 *****************************************************************************/
void
set_fullscreen_mode(GtkWidget *widget, gpointer data1, gpointer data2) {
    int		new_mode = 0;
    int		old_mode = setting_fullscreen;
    GtkWidget	*button;

    /* callback used from two different places, so find the button */
    if (widget == NULL) {
	button = (GtkWidget *)data2;
    }
    else {
	button = (GtkWidget *)widget;
    }

    /* check to see if button is active */
    if ( ( (GTK_IS_TOGGLE_BUTTON (button)) &&
	   (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) ) ||
	 ( (GTK_IS_CHECK_MENU_ITEM (button)) &&
	   (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (button))) ) ) {
	new_mode = 1;
    }

    /* only (un)maximize if changing modes, and leave maximize setting alone */
    if (new_mode != old_mode) {
	if (new_mode) {
	    setting_fullscreen = 1;
	    gtk_window_set_decorated (GTK_WINDOW (main_window), FALSE);
	    gtk_window_fullscreen (GTK_WINDOW (main_window));
	    if ((widget != NULL) && (menu_item_fullscreen != NULL)) {
		gtk_check_menu_item_set_active (menu_item_fullscreen, TRUE);
	    }
	}
	else {
	    setting_fullscreen = 0;
	    gtk_window_set_decorated (GTK_WINDOW (main_window), TRUE);
	    gtk_window_unfullscreen (GTK_WINDOW (main_window));
	    if ((widget != NULL) && (menu_item_fullscreen != NULL)) {
		gtk_check_menu_item_set_active (menu_item_fullscreen, FALSE);
	    }
	}

	save_settings ();
    }
}


/*****************************************************************************
 *
 * SET_MAXIMIZE_MODE()
 *
 *****************************************************************************/
void
set_maximize_mode(GtkWidget *widget, gpointer data1) {
    int		new_mode = (long int)data1;

    /* only do something if changing modes */
    if (new_mode != setting_maximize) {
	if (new_mode) {
	    setting_maximize = MAXIMIZE_ON;
	    if (setting_fullscreen) {
		setting_fullscreen = FULLSCREEN_OFF;
		gtk_window_unfullscreen (GTK_WINDOW (main_window));
	    }
	    gtk_window_set_decorated (GTK_WINDOW (main_window), TRUE);
	    gtk_window_maximize (GTK_WINDOW (main_window));
	    if ((menu_item_fullscreen != NULL)) {
		gtk_check_menu_item_set_active (menu_item_fullscreen, FALSE);
	    }
	}
	else {
	    setting_maximize = MAXIMIZE_OFF;
	    //gtk_window_set_decorated (GTK_WINDOW (main_window), TRUE);
	    gtk_window_unmaximize (GTK_WINDOW (main_window));
	}

	save_settings ();
    }
}


/*****************************************************************************
 *
 * SET_WINDOW_LAYOUT()
 *
 *****************************************************************************/
void
set_window_layout(GtkWidget *widget, gpointer data, GtkWidget *widget2) {
    int		layout = (long int)data;

    /* make sure this is a button toggling ON and actually changing layout */
    if ( (layout != setting_window_layout) &&
	 ( ( (GTK_IS_TOGGLE_BUTTON (widget)) &&
	     (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) ) ||
	   ( (GTK_IS_CHECK_MENU_ITEM (widget2)) &&
	     (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget2))) ) ) ) {

	/* set new layout mode */
	switch (layout) {
	case LAYOUT_NOTEBOOK:
	    setting_window_layout = LAYOUT_NOTEBOOK;
	    if ((widget != NULL) && (menu_item_notebook != NULL)) {
		gtk_check_menu_item_set_active (menu_item_notebook, TRUE);
	    }
	    break;
	case LAYOUT_ONE_PAGE:
	    setting_window_layout = LAYOUT_ONE_PAGE;
	    if ((widget != NULL) && (menu_item_one_page != NULL)) {
		gtk_check_menu_item_set_active (menu_item_one_page, TRUE);
	    }
	    break;
	}

	/* save settings and restart gui */
	save_settings ();
	restart_gtkui ();
    }
}


/*****************************************************************************
 *
 * SET_GUI_IDLE_SLEEP()
 *
 *****************************************************************************/
void
set_gui_idle_sleep(GtkWidget *widget, gpointer data) {
    setting_gui_idle_sleep = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));
}


/*****************************************************************************
 *
 * SET_MIDI_PRIORITY()
 *
 *****************************************************************************/
void
set_midi_priority(GtkWidget *widget, gpointer data) {
    struct sched_param	schedparam;

    setting_midi_priority = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));

    memset (&schedparam, 0, sizeof (struct sched_param));
    schedparam.sched_priority = setting_midi_priority;
    pthread_setschedparam (midi_thread_p, setting_sched_policy, &schedparam);
}


/*****************************************************************************
 *
 * SET_ENGINE_PRIORITY()
 *
 *****************************************************************************/
void
set_engine_priority(GtkWidget *widget, gpointer data) {
    struct sched_param	schedparam;

    setting_engine_priority = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));

    memset (&schedparam, 0, sizeof (struct sched_param));
    schedparam.sched_priority = setting_engine_priority;
    pthread_setschedparam (engine_thread_p, setting_sched_policy, &schedparam);
}


/*****************************************************************************
 *
 * SET_SCHED_POLICY()
 *
 *****************************************************************************/
void
set_sched_policy(GtkWidget *widget, gpointer data) {
    struct sched_param	schedparam;
    int			policy = (long int)data;

    /* make sure this is a button toggling ON and actually changing policy */
    if ( (policy != setting_sched_policy) &&
	 (GTK_IS_TOGGLE_BUTTON (widget)) &&
	 (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) ) {

	/* set new scheduling policy setting */
	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
	    setting_sched_policy = policy;
	    break;
	default:
	    setting_sched_policy = PHASEX_SCHED_POLICY;
	    break;
	}

	/* update scheduling policy of running midi and engine threads */
	memset (&schedparam, 0, sizeof (struct sched_param));
	schedparam.sched_priority = setting_midi_priority;
	pthread_setschedparam (midi_thread_p, setting_sched_policy, &schedparam);
	schedparam.sched_priority = setting_engine_priority;
	pthread_setschedparam (engine_thread_p, setting_sched_policy, &schedparam);
    }
}


/*****************************************************************************
 *
 * SET_JACK_AUTOCONNECT()
 *
 *****************************************************************************/
void
set_jack_autoconnect(GtkWidget *widget, gpointer data, GtkWidget *widget2) {
    int		autoconnect = (long int)data;

    /* make sure this is a button toggling ON and actually changing connect mode */
    if ( (autoconnect != setting_jack_autoconnect) &&
	 ( ( (GTK_IS_TOGGLE_BUTTON (widget)) &&
	     (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) ) ||
	   ( (GTK_IS_CHECK_MENU_ITEM (widget2)) &&
	     (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget2))) ) ) ) {

	/* set new autoconnect mode */
	if (autoconnect) {
	    setting_jack_autoconnect = 1;
	    if ((widget != NULL) && (menu_item_autoconnect != NULL)) {
		gtk_check_menu_item_set_active (menu_item_autoconnect, TRUE);
	    }
	}
	else {
	    setting_jack_autoconnect = 0;
	    if ((widget != NULL) && (menu_item_manualconnect != NULL)) {
		gtk_check_menu_item_set_active (menu_item_manualconnect, TRUE);
	    }
	}

	/* save settings and restart JACK connection */
	save_settings ();
	jack_restart ();
    }
}


/*****************************************************************************
 *
 * SET_SAMPLE_RATE_MODE()
 *
 *****************************************************************************/
void
set_sample_rate_mode(GtkWidget *widget, gpointer data) {
    int		mode = (long int)data;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	switch (mode) {
	case SAMPLE_RATE_UNDERSAMPLE:
	    setting_sample_rate_mode = SAMPLE_RATE_UNDERSAMPLE;
	    break;
	case SAMPLE_RATE_OVERSAMPLE:
	    setting_sample_rate_mode = SAMPLE_RATE_OVERSAMPLE;
	    break;
	case SAMPLE_RATE_NORMAL:
	default:
	    setting_sample_rate_mode = SAMPLE_RATE_NORMAL;
	    break;
	}
    }
}


/*****************************************************************************
 *
 * SET_BANK_MEM_MODE()
 *
 *****************************************************************************/
void
set_bank_mem_mode(GtkWidget *widget, gpointer data, GtkWidget *widget2) {
    int		mode = (long int)data;

    /* make sure this is a button toggling ON and actually changing mode */
    if ( (mode != setting_bank_mem_mode) &&
	 ( ( (widget != NULL) && (widget > (GtkWidget *)3) && (GTK_IS_TOGGLE_BUTTON (widget)) &&
	     (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) ) ||
	   ( (widget2 != NULL) && (widget2 > (GtkWidget *)3) && (GTK_IS_CHECK_MENU_ITEM (widget2)) &&
	     (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget2))) ) ) ) {

	switch (mode) {
	case BANK_MEM_AUTOSAVE:
	    setting_bank_mem_mode = BANK_MEM_AUTOSAVE;
	    if ((bank_autosave_button != NULL) && (GTK_IS_TOGGLE_BUTTON (bank_autosave_button)) &&
		!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bank_autosave_button))) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_autosave_button), TRUE);
	    }
	    if ((menu_item_autosave != NULL)) {
		gtk_check_menu_item_set_active (menu_item_autosave, TRUE);
	    }
	    break;
	case BANK_MEM_PROTECT:
	    setting_bank_mem_mode = BANK_MEM_PROTECT;
	    if ((bank_protect_button != NULL) && (GTK_IS_TOGGLE_BUTTON (bank_protect_button)) &&
		!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bank_protect_button))) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_protect_button), TRUE);
	    }
	    if ((menu_item_protect != NULL)) {
		gtk_check_menu_item_set_active (menu_item_protect, TRUE);
	    }
	    break;
	case BANK_MEM_WARN:
	default:
	    setting_bank_mem_mode = BANK_MEM_WARN;
	    if ((bank_warn_button != NULL) && (GTK_IS_TOGGLE_BUTTON (bank_warn_button)) &&
		!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bank_warn_button))) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_warn_button), TRUE);
	    }
	    if ((menu_item_warn != NULL)) {
		gtk_check_menu_item_set_active (menu_item_warn, TRUE);
	    }
	    break;
	}

	save_settings ();
    }
}


/*****************************************************************************
 *
 * SET_TUNING_FREQ()
 *
 *****************************************************************************/
void
set_tuning_freq(GtkWidget *widget, gpointer data) {
    setting_tuning_freq = a4freq = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data));
    build_freq_table ();
    build_filter_tables ();
}


/*****************************************************************************
 *
 * SET_POLYPHONY()
 *
 *****************************************************************************/
void
set_polyphony(GtkWidget *widget, gpointer data) {
    setting_polyphony = (int)gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));
}


/*****************************************************************************
 *
 * SET_BACKING_STORE()
 *
 *****************************************************************************/
void
set_backing_store(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data))) {
	setting_backing_store = 1;
    }
    else {
	setting_backing_store = 0;
    }
    restart_gtkui ();
}


/*****************************************************************************
 *
 * SET_THEME()
 *
 *****************************************************************************/
void
set_theme(GtkWidget *widget, gpointer data) {
    int		mode = (long int)data;
    int		old_theme = setting_theme;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	switch (mode) {
	case PHASEX_THEME_LIGHT:
	    setting_theme = PHASEX_THEME_LIGHT;
	    break;
	case PHASEX_THEME_SYSTEM:
	    setting_theme = PHASEX_THEME_SYSTEM;
	    break;
	case PHASEX_THEME_CUSTOM:
	    setting_theme = PHASEX_THEME_CUSTOM;
	    break;
	case PHASEX_THEME_DARK:
	default:
	    setting_theme = PHASEX_THEME_DARK;
	    break;
	}

	if (old_theme != setting_theme) {
	    save_settings ();
	}
    }
}


/*****************************************************************************
 *
 * SET_CUSTOM_THEME()
 *
 *****************************************************************************/
void
set_custom_theme(GtkWidget *widget, gpointer data) {
    char		*theme_file;

    theme_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));

    if (theme_file != NULL) {
	if (strcmp (theme_file, setting_custom_theme) != 0) {
	    setting_theme = PHASEX_THEME_CUSTOM;
	    if (setting_custom_theme != NULL) {
		free (setting_custom_theme);
	    }
	    setting_custom_theme = strdup (theme_file);
	    save_settings ();
	}
    }
}


/*****************************************************************************
 *
 * SET_THEME_ENV()
 *
 *****************************************************************************/
void
set_theme_env(void) {
    const char	*theme_selection = NULL;
    char	rc_file_list[256] = "\0";

    switch (setting_theme) {
    case PHASEX_THEME_CUSTOM:
	if (setting_custom_theme != NULL) {
	    theme_selection = setting_custom_theme;
	}
	else {
	    setting_theme = PHASEX_THEME_DARK;
	    theme_selection = GTKRC_DARK;
	}
	break;
    case PHASEX_THEME_LIGHT:
	theme_selection = GTKRC_LIGHT;
	break;
    case PHASEX_THEME_DARK:
    default:
	theme_selection = GTKRC_DARK;
	break;
    }

    /* no theme selected, no rc files... use system defaults */
    if (theme_selection == NULL) {
	    rc_file_list[0] = '\0';
    }
#ifdef GTK_ENGINE_DIR
    /* if GTK supports engines, make sure our engine is on the system */
    else if (g_file_test (g_module_build_path (GTK_ENGINE_DIR, PHASEX_GTK_ENGINE), G_FILE_TEST_EXISTS)) {
	snprintf (rc_file_list, sizeof (rc_file_list), "%s:%s", PHASEX_GTK_ENGINE_RC, theme_selection);
    }
#endif
    /* no engine available, so just load the main theme rc */
    else {
	snprintf (rc_file_list, sizeof (rc_file_list), "%s", theme_selection);
    }

    if (rc_file_list[0] == '\0') {
	unsetenv ("GTK2_RC_FILES");
    }
    else {
	setenv ("GTK2_RC_FILES", rc_file_list, 1);
    }
}


/*****************************************************************************
 *
 * SET_FONT()
 *
 *****************************************************************************/
void
set_font(GtkFontButton *button, GtkWidget *widget) {
    const gchar			*font;

    font = gtk_font_button_get_font_name (button);
    setting_font = strdup (font);
    phasex_font_desc = pango_font_description_from_string (font);

    restart_gtkui ();
}


/*****************************************************************************
 *
 * CLOSE_CONFIG_DIALOG()
 *
 *****************************************************************************/
void
close_config_dialog(void) {
    save_settings ();
    if ((config_dialog != NULL) && GTK_IS_DIALOG (config_dialog)) {
	gtk_widget_destroy (config_dialog);
    }
    config_dialog        = NULL;
    custom_theme_button  = NULL;
    bank_autosave_button = NULL;
    bank_warn_button     = NULL;
    bank_protect_button  = NULL;
}


/*****************************************************************************
 *
 * CREATE_CONFIG_DIALOG()
 *
 *****************************************************************************/
void
create_config_dialog(void) {
    static char		*theme_dir = NULL;
    GtkWidget		*frame;
    GtkWidget		*event;
    GtkWidget		*hbox;
    GtkWidget		*vbox;
    GtkWidget		*box;
    GtkWidget		*sep;
    GtkWidget		*label;
    GtkWidget		*button1;
    GtkWidget		*button2;
    GtkWidget		*button3;
    GtkWidget		*button4;
    GtkWidget		*spin;
    GtkObject		*adj;
    int			min_prio;
    int			max_prio;


    /* don't do anything if the dialog already exists */
    if (config_dialog != NULL) {
	return;
    }

    /* non-modal dialog w/ close button */
    config_dialog = gtk_dialog_new_with_buttons ("PHASEX Preferences",
						 GTK_WINDOW (main_window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_STOCK_CLOSE,
						 GTK_RESPONSE_CLOSE,
						 NULL);
    gtk_window_set_wmclass (GTK_WINDOW (config_dialog), "phasex", "phasex");
    gtk_window_set_role (GTK_WINDOW (config_dialog), "preferences");

    /* main hbox */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_dialog)->vbox),
			hbox, TRUE, TRUE, 8);

/* left vbox */
    frame = gtk_frame_new (NULL);
    widget_set_backing_store (frame);
    gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 8);

    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_container_add (GTK_CONTAINER (frame), event);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (event), vbox);

    /* tuning frequency: hbox w/ label + spinbutton */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

    label = gtk_label_new ("A4 Tuning Frequency:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);

    adj = gtk_adjustment_new (setting_tuning_freq, 220.0, 880.0,
			      1.0, 10.0, 0);

    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 3);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), setting_tuning_freq);
    gtk_box_pack_end (GTK_BOX (box), spin, FALSE, FALSE, 2);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_tuning_freq),
		      (gpointer)spin);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* polyphony: hbox w/ label + spinbutton */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

    label = gtk_label_new ("Polyphony:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);

    adj = gtk_adjustment_new (setting_polyphony, 2, MAX_VOICES,
			      1, DEFAULT_POLYPHONY, 0);

    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), setting_polyphony);
    gtk_box_pack_end (GTK_BOX (box), spin, FALSE, FALSE, 2);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_polyphony),
		      (gpointer)spin);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* bank mem mode: label + 3 radiobuttons w/labels */
    label = gtk_label_new ("Bank Memory Mode:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* Autosave / Warn / Protect radiobuttons */
    bank_autosave_button = gtk_radio_button_new_with_label (NULL, "Autosave");
    gtk_button_set_alignment (GTK_BUTTON (bank_autosave_button), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), bank_autosave_button, FALSE, FALSE, 0);

    bank_warn_button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (bank_autosave_button), "Warn");
    gtk_button_set_alignment (GTK_BUTTON (bank_warn_button), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), bank_warn_button, FALSE, FALSE, 0);

    bank_protect_button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (bank_warn_button), "Protect");
    gtk_button_set_alignment (GTK_BUTTON (bank_protect_button), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), bank_protect_button, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    switch (setting_bank_mem_mode) {
    case BANK_MEM_AUTOSAVE:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_autosave_button), TRUE);
	break;
    case BANK_MEM_WARN:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_warn_button), TRUE);
	break;
    case BANK_MEM_PROTECT:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bank_protect_button), TRUE);
	break;
    }

    /* connect signals for mem mode radiobuttons */
    g_signal_connect (GTK_OBJECT (bank_autosave_button), "toggled",
		      GTK_SIGNAL_FUNC (set_bank_mem_mode),
		      (gpointer)BANK_MEM_AUTOSAVE);

    g_signal_connect (GTK_OBJECT (bank_warn_button), "toggled",
		      GTK_SIGNAL_FUNC (set_bank_mem_mode),
		      (gpointer)BANK_MEM_WARN);

    g_signal_connect (GTK_OBJECT (bank_protect_button), "toggled",
		      GTK_SIGNAL_FUNC (set_bank_mem_mode),
		      (gpointer)BANK_MEM_PROTECT);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* jack autoconnect mode: label + 2 radiobuttons w/labels */
    label = gtk_label_new ("JACK Connection Mode:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button1 = gtk_radio_button_new_with_label (NULL, "Manual");
    gtk_button_set_alignment (GTK_BUTTON (button1), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    button2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "Autoconnect");
    gtk_button_set_alignment (GTK_BUTTON (button2), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    if (setting_jack_autoconnect) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), TRUE);
    }
    else {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
    }

    /* connect signals for jack autoconnect radiobuttons */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_jack_autoconnect),
		      (gpointer)0);

    g_signal_connect (GTK_OBJECT (button2), "toggled",
		      GTK_SIGNAL_FUNC (set_jack_autoconnect),
		      (gpointer)1);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* sample rate mode: label + 3 radiobuttons w/labels */
    label = gtk_label_new ("Sample Rate Mode (*):");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* normal / undersample / oversample  radiobuttons */
    button1 = gtk_radio_button_new_with_label (NULL, "Normal");
    gtk_button_set_alignment (GTK_BUTTON (button1), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    button2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "Undersample");
    gtk_button_set_alignment (GTK_BUTTON (button2), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

    button3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button2), "Oversample");
    gtk_button_set_alignment (GTK_BUTTON (button3), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button3, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    switch (setting_sample_rate_mode) {
    case SAMPLE_RATE_NORMAL:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
	break;
    case SAMPLE_RATE_UNDERSAMPLE:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), TRUE);
	break;
    case SAMPLE_RATE_OVERSAMPLE:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button3), TRUE);
	break;
    }

    /* connect signals for sample rate mode radiobuttons */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_sample_rate_mode),
		      (gpointer)SAMPLE_RATE_NORMAL);

    g_signal_connect (GTK_OBJECT (button2), "toggled",
		      GTK_SIGNAL_FUNC (set_sample_rate_mode),
		      (gpointer)SAMPLE_RATE_UNDERSAMPLE);

    g_signal_connect (GTK_OBJECT (button3), "toggled",
		      GTK_SIGNAL_FUNC (set_sample_rate_mode),
		      (gpointer)SAMPLE_RATE_OVERSAMPLE);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* midi thread priority: hbox w/ label + spinbutton */
    box = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

    label = gtk_label_new ("MIDI Thread Priority:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

    min_prio = sched_get_priority_min (PHASEX_SCHED_POLICY);
    max_prio = sched_get_priority_max (PHASEX_SCHED_POLICY);

    adj = gtk_adjustment_new (setting_midi_priority, min_prio, max_prio,
			      1, 10, 0);

    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), setting_midi_priority);
    gtk_box_pack_end (GTK_BOX (box), spin, FALSE, FALSE, 2);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_midi_priority),
		      (gpointer)spin);

    /* engine thread priority: hbox w/ label + spinbutton */
    box = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

    label = gtk_label_new ("Engine Thread Priority:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

    min_prio = sched_get_priority_min (PHASEX_SCHED_POLICY);
    max_prio = sched_get_priority_max (PHASEX_SCHED_POLICY);

    adj = gtk_adjustment_new (setting_engine_priority, min_prio, max_prio,
			      1, 10, 0);

    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), setting_engine_priority);
    gtk_box_pack_end (GTK_BOX (box), spin, FALSE, FALSE, 2);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_engine_priority),
		      (gpointer)spin);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* engine scheduling policy: label + 2 radiobuttons w/labels */
    label = gtk_label_new ("Realtime Scheduling Policy:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button1 = gtk_radio_button_new_with_label (NULL, "SCHED_FIFO");
    gtk_button_set_alignment (GTK_BUTTON (button1), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    button2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "SCHED_RR");
    gtk_button_set_alignment (GTK_BUTTON (button2), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    if (setting_sched_policy == SCHED_FIFO) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
    }
    else if (setting_sched_policy == SCHED_RR) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), TRUE);
    }

    /* connect signals for jack autoconnect radiobuttons */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_sched_policy),
		      (gpointer)SCHED_FIFO);

    g_signal_connect (GTK_OBJECT (button2), "toggled",
		      GTK_SIGNAL_FUNC (set_sched_policy),
		      (gpointer)SCHED_RR);

/* end left vbox */

/* right vbox */
    frame = gtk_frame_new (NULL);
    widget_set_backing_store (frame);
    gtk_box_pack_end (GTK_BOX (hbox), frame, TRUE, TRUE, 8);

    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_container_add (GTK_CONTAINER (frame), event);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (event), vbox);

    /* fullscreen enable: hbox w/ label + checkbutton */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 4);

    label = gtk_label_new ("Fullscreen Mode:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

    button1 = gtk_check_button_new ();
    gtk_button_set_alignment (GTK_BUTTON (button1), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (box), button1, FALSE, FALSE, 12);

    /* set active button before connecting signals */
    if (setting_fullscreen) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
    }
    else {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), FALSE);
    }

    /* connect signal for backing store checkbutton */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_fullscreen_mode),
		      (gpointer)button1);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* backing store enable: hbox w/ label + checkbutton */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 4);

    label = gtk_label_new ("Enable Backing Store:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

    button1 = gtk_check_button_new ();
    gtk_button_set_alignment (GTK_BUTTON (button1), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (box), button1, FALSE, FALSE, 12);

    /* set active button before connecting signals */
    if (setting_backing_store) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
    }
    else {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), FALSE);
    }

    /* connect signal for backing store checkbutton */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_backing_store),
		      (gpointer)button1);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* layout mode: label + 2 radiobuttons w/labels */
    label = gtk_label_new ("Window Layout:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button1 = gtk_radio_button_new_with_label (NULL, "Notebook");
    gtk_button_set_alignment (GTK_BUTTON (button1), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    button2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "One Page");
    gtk_button_set_alignment (GTK_BUTTON (button2), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    if (setting_window_layout == LAYOUT_ONE_PAGE) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), TRUE);
    }
    else {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
    }

    /* connect signals for layout mode radiobuttons */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_window_layout),
		      (gpointer)LAYOUT_NOTEBOOK);

    g_signal_connect (GTK_OBJECT (button2), "toggled",
		      GTK_SIGNAL_FUNC (set_window_layout),
		      (gpointer)LAYOUT_ONE_PAGE);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* theme: label + 4 radiobuttons w/labels */
    label = gtk_label_new ("GTK Theme (*):");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* dark / light / system / custom radiobuttons */
    button1 = gtk_radio_button_new_with_label (NULL, "Dark");
    gtk_button_set_alignment (GTK_BUTTON (button1), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    button2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "Light");
    gtk_button_set_alignment (GTK_BUTTON (button2), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

    button3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "System");
    gtk_button_set_alignment (GTK_BUTTON (button3), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button3, FALSE, FALSE, 0);

    button4 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button1), "Custom");
    gtk_button_set_alignment (GTK_BUTTON (button4), 0.125, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), button4, FALSE, FALSE, 0);

    /* set active button before connecting signals */
    switch (setting_theme) {
    case PHASEX_THEME_DARK:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button1), TRUE);
	break;
    case PHASEX_THEME_LIGHT:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), TRUE);
	break;
    case PHASEX_THEME_SYSTEM:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button3), TRUE);
	break;
    case PHASEX_THEME_CUSTOM:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button4), TRUE);
	break;
    }

    /* connect signals for theme radiobuttons */
    g_signal_connect (GTK_OBJECT (button1), "toggled",
		      GTK_SIGNAL_FUNC (set_theme),
		      (gpointer)PHASEX_THEME_DARK);

    g_signal_connect (GTK_OBJECT (button2), "toggled",
		      GTK_SIGNAL_FUNC (set_theme),
		      (gpointer)PHASEX_THEME_LIGHT);

    g_signal_connect (GTK_OBJECT (button3), "toggled",
		      GTK_SIGNAL_FUNC (set_theme),
		      (gpointer)PHASEX_THEME_SYSTEM);

    g_signal_connect (GTK_OBJECT (button4), "toggled",
		      GTK_SIGNAL_FUNC (set_theme),
		      (gpointer)PHASEX_THEME_CUSTOM);

    custom_theme_button = button4;

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* custom theme: label + file chooser button */
    label = gtk_label_new ("Custom GTK Theme (*):");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button1 = gtk_file_chooser_button_new ("Select a theme",
					   GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);
    if (setting_custom_theme != NULL) {
	if (theme_dir != NULL) {
	    free (theme_dir);
	}
	theme_dir = dirname (strdup (setting_custom_theme));
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (button1),
					     theme_dir);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (button1),
				       setting_custom_theme);
    }
    else {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (button1),
					     g_get_home_dir() );
    }

    /* connect signals */
    g_signal_connect (GTK_OBJECT (button1), "file_set",
		      GTK_SIGNAL_FUNC (set_custom_theme),
		      (gpointer)button1);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* font selection */
    label = gtk_label_new ("Font:");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    button1 = gtk_font_button_new_with_font (setting_font);
    gtk_box_pack_start (GTK_BOX (vbox), button1, FALSE, FALSE, 0);

    gtk_font_button_set_title (GTK_FONT_BUTTON (button1), "Select a Font");
    gtk_font_button_set_use_font (GTK_FONT_BUTTON (button1), TRUE);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (button1), "font_set",
		      GTK_SIGNAL_FUNC (set_font),
		      (gpointer)button1);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* gui sleep time: hbox w/ label + spinbutton */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

    label = gtk_label_new ("GUI Idle Sleep (μs):");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 2);

    adj = gtk_adjustment_new (setting_gui_idle_sleep, 5000, 250000,
			      5000, 20000, 0);

    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), setting_gui_idle_sleep);
    gtk_box_pack_end (GTK_BOX (box), spin, FALSE, FALSE, 2);

    /* connect signals */
    g_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_gui_idle_sleep),
		      (gpointer)spin);

    /* separator */
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 8);

    /* restart required label */
    label = gtk_label_new ("(*) = Restart Required.");
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_box_pack_end (GTK_BOX (vbox), label, TRUE, TRUE, 0);

    /* empty space */
    label = gtk_label_new (" ");
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_box_pack_end (GTK_BOX (vbox), label, TRUE, TRUE, 0);

/* end right vbox */

    /* connect signals for dialog window */
    g_signal_connect_swapped (G_OBJECT (config_dialog), "response",
			      GTK_SIGNAL_FUNC (close_config_dialog),
			      (gpointer)config_dialog);

    g_signal_connect (G_OBJECT (config_dialog), "destroy",
		      GTK_SIGNAL_FUNC (close_config_dialog),
		      (gpointer)config_dialog);

    widget_set_custom_font (config_dialog);


    /* show the dialog now that it's built */
    gtk_widget_show_all (config_dialog);
}
