/*****************************************************************************
 *
 * help.c
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
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>
#include "phasex.h"
#include "config.h"
#include "param.h"
#include "gtkui.h"
#include "help.h"


PARAM_HELP	param_help[NUM_HELP_PARAMS];

GtkWidget	*param_help_dialog	= NULL;
GtkWidget	*about_dialog		= NULL;

int		param_help_param_id	= -1;


/*****************************************************************************
 *
 * ABOUT_PHASEX_DIALOG()
 *
 *****************************************************************************/
void 
about_phasex_dialog(void) {
    gchar		license[20480];
    FILE		*license_file;
#if GTK_MINOR_VERSION < 8
    GtkWidget		*label;
#else
    const gchar		*authors[] = {
	"* William Weston <weston@sysex.net>:\n    Original PHASEX code, patches, and samples.",
	"* Tony Garnock-Jones:\n    Original GTKKnob code.",
	"* Sean Bolton:\n    Contributions to the GTKKnob code.",
	"* Peter Shorthose <zenadsl6252@zen.co.uk>:\n    Contributions to GTKKnob and PHASEX GUI.",
	NULL 
    };
#endif
    const gchar		*copyright =
	"Copyright (C) 1999-2009:\n"
	"William Weston <weston@sysex.net> and others";
    const gchar		*short_license =
	"PHASEX is Free and Open Source Software released under the\n"
	"GNU Genereal Public License (GPL), Version 2.\n\n"
	"All patches included with PHASEX are Free in the Public Domian.\n\n";
    const gchar		*comments =
	"[P]hase [H]armonic [A]dvanced [S]ynthesis [EX]periment\n"
	"A MIDI software synthesizer for Linux, ALSA, & JACK.";
    const gchar		*website =
	"http://sysex.net/phasex/";
    size_t		j = 0;

    /* build new dialog window if needed */
    if (about_dialog == NULL) {

	/* read in license from phasex documentation */
	memset (license, '\0', sizeof (license));
	if ((license_file = fopen (PHASEX_LICENSE_FILE, "rt")) == NULL) {
	    strncpy (license, short_license, sizeof (license));
	}
	else {
	    if ((j = fread (license, 1, sizeof (license), license_file) == 0) && ferror (license_file)) {
		fprintf (stderr, "Error reading phasex license file '%s'.\n", PHASEX_LICENSE_FILE);
	    }
	    fclose (license_file);
	}
	j = strlen (license);

	/* read in GPLv2 text */
	if ((license_file = fopen (GPLv2_LICENSE_FILE, "rt")) != NULL) {
	    if ((fread (license + j, 1, sizeof (license) - j, license_file) == 0) && ferror (license_file)) {
		fprintf (stderr, "Error reading GPLv2 text from '%s'.\n", GPLv2_LICENSE_FILE);
	    }
	    fclose (license_file);
	}

	/* play tricks with whitespace so GTK can wrap the license text */
	for (j = 0; (j < sizeof (license)) && (license[j] != '\0'); j++) {
	    if (license[j] == '\n') {
		if ((license[j+1] == '\n') || (license[j+1] == ' ') || (license[j+1] == '\t') || (license[j+1] == '-')) {
		    j++;
		}
		else if (license[j+1] == '') {
		    license[++j] = ' ';
		}
		else {
		    license[j] = ' ';
		}
	    }
	    else if (license[j] == '') {
		license[j] = ' ';
	    }
	}

#if GTK_MINOR_VERSION < 8
	/* for old versions, use a plain old dialog with labels */
	about_dialog = gtk_dialog_new_with_buttons ("About PHASEX",
						    GTK_WINDOW (main_window),
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_STOCK_CLOSE,
						    GTK_RESPONSE_CLOSE,
						    NULL);
	gtk_window_set_wmclass (GTK_WINDOW (about_dialog), "phasex", "phasex-about");
	gtk_window_set_role (GTK_WINDOW (about_dialog), "about");
	widget_set_backing_store (about_dialog);

	label = gtk_label_new ("<span size=\"larger\">PHASEX "PACKAGE_VERSION"</span>");
	widget_set_backing_store (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	label = gtk_label_new (comments);
	widget_set_backing_store (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	label = gtk_label_new (copyright);
	widget_set_backing_store (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	label = gtk_label_new (short_license);
	widget_set_backing_store (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	label = gtk_label_new (website);
	widget_set_backing_store (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
			    label, TRUE, FALSE, 8);
#else /* GTK_MINOR_VERSION >= 8 */
	/* only newer GTK gets to do it the easy way */
	about_dialog = gtk_about_dialog_new ();
	widget_set_backing_store (about_dialog);

	/* set data */
	gtk_about_dialog_set_name         (GTK_ABOUT_DIALOG (about_dialog), PACKAGE_NAME);
	gtk_about_dialog_set_version      (GTK_ABOUT_DIALOG (about_dialog), PACKAGE_VERSION);
	gtk_about_dialog_set_copyright    (GTK_ABOUT_DIALOG (about_dialog), copyright);
	gtk_about_dialog_set_license      (GTK_ABOUT_DIALOG (about_dialog), license);
	gtk_about_dialog_set_wrap_license (GTK_ABOUT_DIALOG (about_dialog), TRUE);
	gtk_about_dialog_set_website      (GTK_ABOUT_DIALOG (about_dialog), website);
	gtk_about_dialog_set_authors      (GTK_ABOUT_DIALOG (about_dialog), authors);
	gtk_about_dialog_set_comments     (GTK_ABOUT_DIALOG (about_dialog), comments);

#endif /* GTK_MINOR_VERSION < 8 */

	/* make sure the dialog gets hidden when the user responds */
	g_signal_connect_swapped (about_dialog, "response", 
				  G_CALLBACK (gtk_widget_hide),
				  about_dialog);
    }

    /* show the user */
    gtk_widget_show_all (about_dialog);
}


/*****************************************************************************
 *
 * INIT_HELP()
 *
 *****************************************************************************/
void 
init_help(void) {
    FILE	*help_f;
    char	linebuf[256];
    char	textbuf[8192];
    char	*name = NULL;
    char	*label = NULL;
    char	*p;
    char	*q;
    char	*r;
    int		j;
    int		in_param_text = 0;

    /* open the parameter help file */
    if ((help_f = fopen (PARAM_HELPFILE, "rt")) == NULL) {
	return;
    }

    /* read help entries */
    memset (linebuf, '\0', sizeof (linebuf));
    memset (textbuf, '\0', sizeof (textbuf));
    while (fgets (linebuf, 256, help_f) != NULL) {

	/* help text can span multiple lines */
	if (in_param_text) {

	    /* help text ends with a dot on its own line */
	    if ((linebuf[0] == '.') && ((linebuf[1] == '\n') || (linebuf[1] == '\0'))) {

		/* find parameter(s) matching name */
		if (strncmp (name, "osc#_", 5) == 0) {
		    for (j = 0; j < NUM_PARAMS; j++) {
			if ((strncmp (param[j].name, "osc", 3) == 0)
			    && (strcmp (&(param[j].name[4]), &(name[4])) == 0)) {
			    param_help[j].label = strdup (label);
			    param_help[j].text  = strdup (textbuf);
			}
		    }
		}
		else if (strncmp (name, "lfo#_", 5) == 0) {
		    for (j = 0; j < NUM_PARAMS; j++) {
			if ((strncmp (param[j].name, "lfo", 3) == 0)
			    && (strcmp (&(param[j].name[4]), &(name[4])) == 0)) {
			    param_help[j].label = strdup (label);
			    param_help[j].text  = strdup (textbuf);
			}
		    }
		}
		else {
		    for (j = 0; j < NUM_HELP_PARAMS; j++) {
			if (strcmp (param[j].name, name) == 0) {
			    param_help[j].label = strdup (label);
			    param_help[j].text  = strdup (textbuf);
			    break;
			}
		    }
		}

		/* prepare to read another parameter name and label */
		in_param_text = 0;
	    }

	    /* if still in help text for this param, add current line to help text */
	    else {
		if (linebuf[0] == '\n') {
		    strcat (textbuf, "\n");
		}
		else if ((p = index ((linebuf + 1), '\n')) != NULL) {
		    *p = ' ';
		}
		strcat (textbuf, linebuf);
	    }
	}

	/* searching for :param_name:Param Label: line */
	else {
	    /* discard anything not starting with a colon */
	    if (linebuf[0] != ':') {
		continue;
	    }
	    p = &(linebuf[1]);

	    /* discard line if second colon is not found */
	    if ((q = index (p, ':')) == NULL) {
		continue;
	    }
	    *q = '\0';
	    q++;

	    /* discard line if last colon is not found */
	    if ((r = index (q, ':')) == NULL) {
		continue;
	    }
	    r++;
	    *r = '\0';

	    /* make copies of name and label because linebuf will be overwritten */
	    if (name != NULL) {
		free (name);
	    }
	    name  = strdup (p);
	    if (label != NULL) {
		free (label);
	    }
	    label = strdup (q);

	    /* prepare to read in multiple lines of text for this param */
	    in_param_text = 1;
	    memset (textbuf, '\0', sizeof (textbuf));
	}
	memset (linebuf, '\0', sizeof (linebuf));
    }

    /* done parsing */
    fclose (help_f);

    if (name != NULL) {
	free (name);
    }
    if (label != NULL) {
	free (label);
    }
}


/*****************************************************************************
 *
 * CLOSE_PARAM_HELP_DIALOG()
 *
 *****************************************************************************/
void 
close_param_help_dialog(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy (widget);
    param_help_dialog = NULL;
}


/*****************************************************************************
 *
 * DISPLAY_PARAM_HELP()
 *
 *****************************************************************************/
void 
display_param_help(int param_id) {
    GtkWidget		*label;
    int			same_param_id = 0;

    /* destroy current help window */
    if (param_help_dialog != NULL) {
	if (param_id == param_help_param_id) {
	    same_param_id = 1;
	}
	gtk_widget_destroy (param_help_dialog);
	param_help_param_id = -1;
    }

    /* only create new help window if it's a new or different parameter */
    if (!same_param_id) {
	param_help_dialog = gtk_dialog_new_with_buttons ("Parameter Description",
							 GTK_WINDOW (main_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_STOCK_CLOSE,
							 GTK_RESPONSE_CLOSE,
							 NULL);
	gtk_window_set_wmclass (GTK_WINDOW (param_help_dialog), "phasex", "phasex-help");
	gtk_window_set_role (GTK_WINDOW (param_help_dialog), "help");

	/* full parameter name */
	label = gtk_label_new (param_help[param_id].label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 8, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (param_help_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	/* parameter help text */
	label = gtk_label_new (param_help[param_id].text);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), 8, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (param_help_dialog)->vbox),
			    label, TRUE, FALSE, 8);

	/* connect signals */
	g_signal_connect_swapped (G_OBJECT (param_help_dialog), "response",
				  GTK_SIGNAL_FUNC (close_param_help_dialog),
				  (gpointer)param_help_dialog);

	g_signal_connect (G_OBJECT (param_help_dialog), "destroy",
			  GTK_SIGNAL_FUNC (close_param_help_dialog),
			  (gpointer)param_help_dialog);

	/* display to the user */
	gtk_widget_show_all (param_help_dialog);

	/* set internal info */
	param_help_param_id = param_id;
    }
}


/*****************************************************************************
 *
 * DISPLAY_PHASEX_HELP()
 *
 *****************************************************************************/
void 
display_phasex_help(void) {
    GtkWidget		*label;
    GtkWidget		*window;
    GdkGeometry		hints = { 0, };

    /* destroy current help window */
    if (param_help_dialog != NULL) {
	gtk_widget_destroy (param_help_dialog);
	param_help_param_id = -1;
    }

    param_help_dialog = gtk_dialog_new_with_buttons ("Using PHASEX",
						     GTK_WINDOW (main_window),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_STOCK_CLOSE,
						     GTK_RESPONSE_CLOSE,
						     NULL);
    gtk_window_set_wmclass (GTK_WINDOW (param_help_dialog), "phasex", "phasex-help");
    gtk_window_set_role (GTK_WINDOW (param_help_dialog), "help");

    /* scrolling window */
    window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window),
				    GTK_POLICY_NEVER,
				    GTK_POLICY_AUTOMATIC);

    /* main help text */
    label = gtk_label_new (param_help[PARAM_PHASEX_HELP].text);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_label_set_width_chars (GTK_LABEL (label), 72);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_misc_set_padding (GTK_MISC (label), 2, 0);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (window), label);

    /* attach to dialog's vbox */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (param_help_dialog)->vbox),
			window, TRUE, TRUE, 2);

    /* set window geometry hints */
    hints.min_width   = 480;
    hints.min_height  = 480;
    gtk_window_set_geometry_hints (GTK_WINDOW (param_help_dialog),
				   NULL, &hints, GDK_HINT_MIN_SIZE);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (param_help_dialog), "response",
			      GTK_SIGNAL_FUNC (close_param_help_dialog),
			      (gpointer)param_help_dialog);

    g_signal_connect (G_OBJECT (param_help_dialog), "destroy",
		      GTK_SIGNAL_FUNC (close_param_help_dialog),
		      (gpointer)param_help_dialog);

    /* display to the user */
    gtk_widget_show_all (param_help_dialog);

    /* set internal info */
    param_help_param_id = PARAM_PHASEX_HELP;
}
