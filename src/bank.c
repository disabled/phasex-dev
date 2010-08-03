/*****************************************************************************
 *
 * bank.c
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
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include "phasex.h"
#include "config.h"
#include "patch.h"
#include "param.h"
#include "gtkui.h"
#include "gtkknob.h"
#include "callback.h"
#include "bank.h"
#include "settings.h"
#include "engine.h"


int		program_number			= 0;
int		program_change_request		= -1;

PATCH		bank[PATCH_BANK_SIZE];

GtkObject	*program_adj			= NULL;
GtkWidget	*program_spin			= NULL;
GtkWidget	*program_entry			= NULL;

GtkWidget	*midi_channel_label		= NULL;
GtkObject	*midi_channel_adj		= NULL;

GtkWidget	*bank_autosave_button		= NULL;
GtkWidget	*bank_warn_button		= NULL;
GtkWidget	*bank_protect_button		= NULL;

GtkWidget	*modified_label			= NULL;

GtkWidget	*patch_load_dialog		= NULL;
GtkWidget	*patch_save_dialog		= NULL;

GtkWidget	*patch_load_start_spin		= NULL;

int		patch_load_start		= 0;

int		show_modified			= 0;


/*****************************************************************************
 *
 * INIT_PATCH_BANK()
 *
 *****************************************************************************/
void 
init_patch_bank(void) {
    int		j;

    patch = &(bank[0]);
    for (j = 0; j < PATCH_BANK_SIZE; j++) {
	bank[j].name      = NULL;
	bank[j].filename  = NULL;
	bank[j].directory = NULL;
	init_patch (&(bank[j]));
    }
    program_number = 0;
}


/*****************************************************************************
 *
 * LOAD_BANK()
 *
 *****************************************************************************/
void 
load_bank(void) {
    FILE	*bank_f;
    char	*p;
    char	*filename;
    char	*tmpname;
    char	buffer[1024];
    int		prog;
    int		orig_prog	= program_number;
    int		line		= 0;
    int		once		= 1;
    int		result;

    /* open the bank file */
    if ((bank_f = fopen (user_bank_file, "rt")) == NULL) {
	return;
    }

    /* read bank entries */
    while (fgets (buffer, 1024, bank_f) != NULL) {
	line++;

	/* discard comments and blank lines */
	if ((buffer[0] == '\n') || (buffer[0] == '#')) {
	    continue;
	}

	/* get program number */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	prog = atoi(p);
	if ((prog < 0) || (prog >= PATCH_BANK_SIZE)) {
	    prog = 0;
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

	/* get patch name */
	if ((tmpname = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	if (*tmpname == ';') {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	if ((filename = strdup (tmpname)) == NULL) {
	    phasex_shutdown ("Out of memory!\n");
	}

	/* make sure there's a ';' */
	if ((p = get_next_token (buffer)) == NULL) {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}
	if (*p != ';') {
	    while (get_next_token (buffer) != NULL);
	    continue;
	}

	/* flush remainder of line */
	while (get_next_token (buffer) != NULL);

	/* load patch into bank */
	program_number = prog;
	patch          = &(bank[prog]);
	result         = 0;
	
	/* handle bare patch names from 0.10.x versions */
	if (filename[0] != '/') {
	    snprintf (buffer, sizeof (buffer), "%s/%s.phx", user_patch_dir, filename);
	    result = read_patch (buffer, prog);
	    if (result != 0) {
		snprintf (buffer, sizeof (buffer), "%s/%s.phx", PATCH_DIR, filename);
		result = read_patch (buffer, prog);
	    }
	}

	/* handle fully qualified filenames */
	else {
	    result = read_patch (filename, prog);
	}

	/* initialize on failure and name based on program number */
	if (result != 0) {
	    read_patch (sys_default_patch, prog);
	    snprintf (buffer, sizeof (buffer), "Untitled-%04d", prog);
	    if (bank[prog].name != NULL) {
		free (bank[prog].name);
	    }
	    bank[prog].name = strdup (buffer);
	    if (bank[prog].directory != NULL) {
		free (bank[prog].directory);
	    }
	    bank[prog].directory = strdup (user_patch_dir);
	}

	/* free up memory used to piece filename together */
	free (filename);

	/* Lock some global parameters after the first patch is read */
	if (once) {
	    param[PARAM_MIDI_CHANNEL].locked = 1;
	    param[PARAM_BPM].locked          = 1;
	    param[PARAM_MASTER_TUNE].locked  = 1;
	    once = 0;
	}
    }

    /* done parsing */
    fclose (bank_f);

    /* now fill the empty bank slots with the default patch */
    for (prog = 0; prog < PATCH_BANK_SIZE; prog++) {
	if (bank[prog].name == NULL) {
	    program_number = prog;
	    patch          = &(bank[prog]);
	    read_patch (sys_default_patch, prog);
	    snprintf (buffer, sizeof (buffer), "Untitled-%04d", prog);
	    bank[prog].name = strdup (buffer);
	    //if (bank[prog].directory != NULL) {
	    //	free (bank[prog].directory);
	    //}
	    bank[prog].directory = strdup (user_patch_dir);
	}
	bank[prog].modified = 0;
    }

    patch          = &(bank[orig_prog]);
    program_number = orig_prog;

    return;
}


/*****************************************************************************
 *
 * SAVE_BANK()
 *
 *****************************************************************************/
void 
save_bank(void) {
    FILE	*bank_f;
    int		j;

    /* open the bank file */
    if ((bank_f = fopen (user_bank_file, "wt")) == NULL) {
	if (debug) {
	    fprintf (stderr, "Error opening bank file %s for write: %s\n",
		     user_bank_file, strerror (errno));
	}
	return;
    }

    /* write the bank in the easy to read format */
    fprintf (bank_f, "# PHASEX User Patch Bank\n");
    for (j = 0; j < PATCH_BANK_SIZE; j++) {
	if (bank[j].filename != NULL) {
	    fprintf (bank_f, "%04d = %s;\n", j, bank[j].filename);
	}
    }

    /* done saving */
    fclose (bank_f);
}


/*****************************************************************************
 *
 * FIND_PATCH()
 *
 *****************************************************************************/
int
find_patch(char *name) {
    int		j;

    for (j = 0; j < PATCH_BANK_SIZE; j++) {
	if (strcmp (name, bank[j].name) == 0) {
	    break;
	}
    }
    return j;
}


/*****************************************************************************
 *
 * GET_PATCH_NAME_FROM_FILENAME()
 *
 *****************************************************************************/
char *
get_patch_name_from_filename(char *filename) {
    char	*f;
    char	*tmpname;
    char	*name = NULL;

    /* missing filename gets name of "Untitled" */
    if ((filename == NULL) || (strcmp (filename, user_patchdump_file) == 0)) {
	name = strdup ("Untitled");
    }
    else {
	/* strip off leading directory components */
	if ((f = rindex (filename, '/')) == NULL) {
	    tmpname = filename;
	}
	else {
	    tmpname = f + 1;
	}

	/* make a copy that can be modified and freed safely */
	if ((name = strdup (tmpname)) == NULL) {
	    phasex_shutdown ("Out of memory!\n");
	}

	/* strip off the .phx */
	if ((f = strstr (name, ".phx\0")) != NULL) {
	    *f = '\0';
	}
    }

    /* send name back to caller */
    return name;
}


/*****************************************************************************
 *
 * GET_PATCH_FILENAME_FROM_ENTRY()
 *
 *****************************************************************************/
char * 
get_patch_filename_from_entry(GtkEntry *entry) {
    const char	*name;
    char	*tmpname;
    char	filename[PATH_MAX];

    /* get name from entry widget */
    name = gtk_entry_get_text (GTK_ENTRY (entry));

    if ((name == NULL) || (name[0] == '\0')) {
	/* if entry is empty, get current patch filename */
	tmpname = get_patch_name_from_filename (patch->filename);

	/* build the filename from the patch name */
	snprintf (filename, PATH_MAX, "%s/%s.phx", user_patch_dir, tmpname);

	/* free up mem */
	free (tmpname);
    }
    else {
	/* build the filename from the patch name */
	snprintf (filename, PATH_MAX, "%s/%s.phx", user_patch_dir, name);
    }

    /* return a copy */
    tmpname = strdup (filename);
    return tmpname;
}


/*****************************************************************************
 *
 * CHECK_PATCH_OVERWRITE()
 *
 *****************************************************************************/
int
check_patch_overwrite(char *filename) {
    struct stat		filestat;
    GtkWidget		*dialog;
    GtkWidget		*label;
    gint		response;
    static int		recurse = 0;

    if (recurse) {
	recurse = 0;
	return 0;
    }

    /* check to see if file exists */
    if (stat (filename, &filestat) == 0) {
    	recurse = 1;

	/* create dialog with buttons */
	dialog = gtk_dialog_new_with_buttons ("WARNING:  Patch file exists",
					      GTK_WINDOW (main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE_AS,
					      1,
					      GTK_STOCK_SAVE,
					      GTK_RESPONSE_YES,
					      NULL);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "phasex", "phasex-dialog");
	gtk_window_set_role (GTK_WINDOW (dialog), "verify-save");

	/* Alert message */
	label = gtk_label_new ("This operation will overwrite an existing "
			       "patch with the same filename.  Save anyway? ");
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), 8, 8);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
	//widget_set_custom_font (dialog);
	gtk_widget_show_all (dialog);

	/* Run the dialog */
	g_idle_remove_by_data ((gpointer)main_window);
	response = gtk_dialog_run (GTK_DIALOG (dialog));

	/* all finished with dialog now */
	gtk_widget_destroy (dialog);

	switch (response) {

	case GTK_RESPONSE_YES:
	    /* save */
	    on_patch_save_activate (NULL, filename);
	    break;

	case 1:
	    /* save as */
	    run_patch_save_as_dialog (NULL, filename);
	    break;

	case GTK_RESPONSE_CANCEL:
	    /* cancel */
	    break;
	}

	recurse = 0;
	g_idle_add (&param_idle_update, (gpointer)main_window);
	return 1;
    }

    return 0;
}



/*****************************************************************************
 *
 * SELECT_PROGRAM()
 *
 *****************************************************************************/
void 
select_program(GtkWidget *widget, gpointer data) {
    GtkWidget		*dialog;
    GtkWidget		*label;
    GtkWidget		*adj;
    GtkEntry		*entry;
    char		*filename;
    char		tmpname[PATH_MAX];
    gint		response;
    int			need_select = 1;
    int			prog;
    int			p;

    /* the widgets we need to reference are callback args */
    adj = widget;
    if (data == NULL) {
	entry = GTK_ENTRY (program_entry);
    }
    else {
	entry = GTK_ENTRY (data);
    }

    /* get name from entry widget or strip current patch filename */
    filename = get_patch_filename_from_entry (entry);

    /* if not called as a callback, see if there is a pending request */
    if (widget == NULL) {
	if (program_change_request != -1) {
	    prog = program_change_request;
	    program_change_request = -1;
	}
	else {
	    return;
	}
    }

    /* get program number from spin button if called from gui */
    else {
	prog = (int)(gtk_adjustment_get_value (GTK_ADJUSTMENT (adj)));
    }

    /* do nothing if the requested patch is the current patch */
    if (prog == program_number) {
	return;
    }

    /* whether or not to save depends on memory mode */
    switch (setting_bank_mem_mode) {

    case BANK_MEM_AUTOSAVE:
	/* save current patch if modified */
	if (patch->modified) {
	    on_patch_save_activate (NULL, filename);
	}

	/* no canceling in autosave mode */
	need_select = 1;

	break;

    case BANK_MEM_WARN:
	/* this is set now, and may be canceled */
	need_select = 1;

	/* if modified, warn about losing current patch and give option to save */
	if (patch->modified) {

	    /* create dialog with buttons */
	    dialog = gtk_dialog_new_with_buttons ("WARNING:  Patch Modified",
						  GTK_WINDOW (main_window),
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_CANCEL,
						  "Ignore Changes",
						  GTK_RESPONSE_NO,
						  "Save and Select New",
						  GTK_RESPONSE_YES,
						  NULL);
	    gtk_window_set_wmclass (GTK_WINDOW (dialog), "phasex", "phasex-dialog");
	    gtk_window_set_role (GTK_WINDOW (dialog), "verify-save");

	    /* Alert message */
	    label = gtk_label_new ("The current patch has not been saved since "
				   "it was last modified.  Save now before "
				   "selecting new patch?");
	    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	    gtk_misc_set_padding (GTK_MISC (label), 8, 8);
	    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
	    //widget_set_custom_font (dialog);
	    gtk_widget_show_all (dialog);

	    /* Run the dialog */
	    g_idle_remove_by_data ((gpointer)main_window);
	    response = gtk_dialog_run (GTK_DIALOG (dialog));

	    switch (response) {

	    case GTK_RESPONSE_YES:
		/* save patch and set flag to select new from bank */
		on_patch_save_activate (NULL, filename);
		need_select = 1;
		break;

	    case GTK_RESPONSE_NO:
		/* don't save, but still set flag to select new */
		need_select = 1;
		break;

	    case GTK_RESPONSE_CANCEL:
		/* set spin button back to current program and don't select new */
		gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), program_number);
		need_select = 0;
		break;
	    }

	    /* all finished with dialog now */
	    gtk_widget_destroy (dialog);
	    g_idle_add (&param_idle_update, (gpointer)main_window);
	}
	break;

    case BANK_MEM_PROTECT:
	/* explicitly don't save */
	need_select = 1;
	break;
    }

    /* free name, now that we're done with it */
    free (filename);

    /* load patch specified by spinbutton or init new patch */
    if (need_select) {
	program_number = prog;

	/* change patch modified label, if needed */
	patch = &(bank[prog]);
	if (patch->modified != show_modified) {
	    if (patch->modified) {
		gtk_label_set_text (GTK_LABEL (modified_label), "[modified]");
	    }
	    else {
		gtk_label_set_text (GTK_LABEL (modified_label), "");
	    }
	    show_modified = patch->modified;
	}

	/* run callbacks so gui gets updated */
	for (p = 0; p < NUM_PARAMS; p++) {
	    /* keep locked params at the same value */
	    if (param[p].locked) {
		param[p].cc_val[prog] = param[p].cc_cur;
		param[p].callback (main_window, (gpointer)(&(param[p])));
	    }
	    /* run callbacks only for changed params */
	    else if (param[p].cc_val[prog] != param[p].cc_cur) {
		param[p].callback (NULL, (gpointer)(&(param[p])));
	    }
	}

	/* set displayed patch name */
	if (bank[prog].name == NULL) {
	    snprintf (tmpname, PATCH_NAME_LEN, "Untitled-%04d", prog);
	    gtk_entry_set_text (entry, tmpname);
	}
	else {
	    gtk_entry_set_text (entry, bank[prog].name);
	}
    }
}


/*****************************************************************************
 *
 * SAVE_PROGRAM()
 *
 *****************************************************************************/
void 
save_program(GtkWidget *widget, gpointer data) {
    GtkEntry		*entry = GTK_ENTRY (data);
    const char		*name;
    char		filename[PATH_MAX];

    /* get name from entry widget */
    name = gtk_entry_get_text (GTK_ENTRY (entry));

    /* if this is a changed patch name, rebuild the filename */
    if ((patch->directory != NULL) && (name != NULL)) {
	if (strcmp (patch->directory, PATCH_DIR) == 0) {
	    snprintf (filename, PATH_MAX, "%s/%s.phx", user_patch_dir, name);
	}
	else {
	    snprintf (filename, PATH_MAX, "%s/%s.phx", patch->directory, name);
	}
    }

    /* otherwise, use the filename we already have */
    else {
	strncpy (filename, patch->filename, PATH_MAX);
    }

    /* if the patch is still untitled, run the save as dialog */
    if (strncmp (filename, "Untitled", 8) == 0) {
	run_patch_save_as_dialog (NULL, filename);
    }

    /* otherwise, just save it */
    else {
	on_patch_save_activate (NULL, filename);
    }
}


/*****************************************************************************
 *
 * LOAD_PROGRAM()
 *
 *****************************************************************************/
void 
load_program(GtkWidget *widget, gpointer data) {
    GtkEntry		*entry		= GTK_ENTRY (data);
    GtkWidget		*dialog;
    GtkWidget		*label;
    const char		*name;
    char		filename[PATH_MAX];
    gint		response;
    int			need_load	= 0;

    /* build filename from entry widget and current patch directory */
    name = gtk_entry_get_text (entry);
    snprintf (filename, PATH_MAX, "%s/%s.phx", patch->directory, name);

    /* handle saving of current patch based on mem mode */
    switch (setting_bank_mem_mode) {

    case BANK_MEM_AUTOSAVE:
	/* save current patch if modified */
	if (patch->modified) {
	    on_patch_save_activate (NULL, NULL);
	}
	need_load = 1;
	break;

    case BANK_MEM_WARN:
	/* if modified, warn about losing current patch */
	if (patch->modified) {

	    /* create dialog with buttons */
	    dialog = gtk_dialog_new_with_buttons ("WARNING:  Patch Modified",
						  GTK_WINDOW (main_window),
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_CANCEL,
						  "Ignore Changes",
						  GTK_RESPONSE_NO,
						  "Save and Load New",
						  GTK_RESPONSE_YES,
						  NULL);
	    gtk_window_set_wmclass (GTK_WINDOW (dialog), "phasex", "phasex-dialog");
	    gtk_window_set_role (GTK_WINDOW (dialog), "verify-save");

	    /* Alert message */
	    label = gtk_label_new ("The current patch has not been saved since "
				   "it was last modified.  Save now before "
				   "loading new patch?");
	    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	    gtk_misc_set_padding (GTK_MISC (label), 8, 8);
	    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
	    //widget_set_custom_font (dialog);
	    gtk_widget_show_all (dialog);

	    /* Run the dialog */
	    g_idle_remove_by_data ((gpointer)main_window);
	    response = gtk_dialog_run (GTK_DIALOG (dialog));
	    switch (response) {
	    case GTK_RESPONSE_YES:
		/* save patch and get ready to load a new one */
		on_patch_save_activate (NULL, NULL);
		need_load = 1;
		break;
	    case GTK_RESPONSE_NO:
		/* no save, only set flag to load new patch */
		need_load = 1;
		break;
	    case GTK_RESPONSE_CANCEL:
		/* find old patch name and revert text in entry */
		if (patch->name != NULL) {
		    gtk_entry_set_text (GTK_ENTRY (entry), patch->name);
		}
		else if (patch->filename != NULL) {
		    patch->name = get_patch_name_from_filename (patch->filename);
		    gtk_entry_set_text (GTK_ENTRY (entry), patch->name);
		}
		else {
		    gtk_entry_set_text (GTK_ENTRY (entry), "");
		}
		/* cancel out on loading new patch */
		need_load = 0;
		break;
	    }
	    gtk_widget_destroy (dialog);
	    g_idle_add (&param_idle_update, (gpointer)main_window);
	}
	/* if not modified, just load new patch */
	else {
	    need_load = 1;
	}
	break;

    case BANK_MEM_PROTECT:
	/* explicitly don't save in protect mode */
	need_load = 1;
	break;
    }

    /* load patch specified by entry or init new patch */
    if (need_load) {
	if (read_patch (filename, program_number) != 0) {
	    snprintf (filename, PATH_MAX, "%s/%s.phx", user_patch_dir, name);
	    if (read_patch (filename, program_number) != 0) {
		snprintf (filename, PATH_MAX, "%s/%s.phx", PATCH_DIR, name);
		if (read_patch (filename, program_number) != 0) {
		    read_patch (sys_default_patch, program_number);
		}
	    }
	}
	if (patch->name == NULL) {
	    sprintf (filename, "Untitled-%04d", program_number);
	    patch->name = strdup(filename);
	}
	gtk_entry_set_text (entry, patch->name);
    }
}


/*****************************************************************************
 *
 * BANK_AUTOSAVE_ACTIVATE()
 *
 *****************************************************************************/
void 
bank_autosave_activate(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	setting_bank_mem_mode = BANK_MEM_AUTOSAVE;
    }
}


/*****************************************************************************
 *
 * BANK_WARN_ACTIVATE()
 *
 *****************************************************************************/
void 
bank_warn_activate(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	setting_bank_mem_mode = BANK_MEM_WARN;
    }
}


/*****************************************************************************
 *
 * BANK_PROTECT_ACTIVATE()
 *
 *****************************************************************************/
void 
bank_protect_activate(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	setting_bank_mem_mode = BANK_MEM_PROTECT;
    }
}


/*****************************************************************************
 *
 * MIDI_LABEL_BUTTON_PRESS()
 *
 *****************************************************************************/
void
midi_label_button_press(gpointer data1, gpointer data2) {
    GdkEventAny		*event	    = (GdkEventAny *)data2;
    GdkEventButton	*button	    = (GdkEventButton *)data2;
    GdkEventScroll	*scroll	    = (GdkEventScroll *)data2;
    int			new_channel = setting_midi_channel;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
	switch (button->button) {
	case 1:		/* left button */
	    if (new_channel > 0) {
		new_channel--;
	    }
	    break;
	case 2:		/* middle button */
	    new_channel = 9;
	    break;
	case 3:		/* right button */
	    if (new_channel < 16) {
		new_channel++;
	    }
	    break;
	}
	break;
    case GDK_SCROLL:
	switch (scroll->direction) {
	case GDK_SCROLL_DOWN:
	    if (new_channel > 0) {
		new_channel--;
	    }
	    break;
	case GDK_SCROLL_UP:
	    if (new_channel < 16) {
		new_channel++;
	    }
	    break;
	default:
	    /* must handle all scroll directions to keep gtk quiet */
	    break;
	}
	break;

    default:
	/* must handle all cases for event type to keep gtk quiet */
	break;
    }

    set_midi_channel (NULL, (gpointer)(long int)new_channel, NULL);
}


/*****************************************************************************
 *
 * CREATE_BANK_GROUP()
 *
 * Create the group of controls for selecting patches from the bank.
 *
 *****************************************************************************/
void 
create_bank_group(GtkWidget *main_window, GtkWidget *parent_table,
		  int x0, int x1, int y0, int y1) {
    GtkWidget		*frame;
    GtkWidget		*hbox;
    GtkWidget		*box;
    GtkWidget		*vbox;
    GtkWidget		*event;
    GtkWidget		*label;
    GtkWidget		*button;
    GtkWidget		*knob;

    /* Create frame with label and hbox, then pack in controls */
    frame = gtk_frame_new (NULL);
    widget_set_backing_store (frame);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
    gtk_table_attach (GTK_TABLE (parent_table), frame,
		      x0, x1, y0, y1,
		      (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		      (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		      0, 0);

    /* Frame label */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    label = gtk_label_new ("<b>Patch</b>");
    widget_set_backing_store (label);
    widget_set_custom_font (label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_container_add (GTK_CONTAINER (event), label);
    gtk_frame_set_label_widget (GTK_FRAME (frame), event);

    /* Frame event box */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_container_add (GTK_CONTAINER (frame), event);

    /* VBox to keep buttons from resizing */
    vbox = gtk_vbox_new (FALSE, 0);
    label = gtk_event_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    label = gtk_event_box_new ();
    gtk_box_pack_end (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (event), vbox);

    /* HBox for controls */
    hbox = gtk_hbox_new (FALSE, 1);
    widget_set_backing_store (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

    /* Program selector box (label + spin) */
    box = gtk_hbox_new (FALSE, 0);
    widget_set_backing_store (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 1);

    /* Clickable Program selector label */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_widget_set_name (event, "program_number");
    label = gtk_label_new ("Program #:");
    widget_set_backing_store (label);
    gtk_container_add (GTK_CONTAINER (event), label);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* connect signal for clicking program number label */
    g_signal_connect (G_OBJECT (event), "button_press_event",
		      GTK_SIGNAL_FUNC (param_label_button_press),
		      (gpointer)event);

    /* Program selector adjustment */
    program_adj = gtk_adjustment_new (0, 0, (PATCH_BANK_SIZE - 1), 1, 8, 0);

    /* Program selector spin button */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    program_spin = gtk_spin_button_new (GTK_ADJUSTMENT (program_adj), 0, 0);
    widget_set_backing_store (program_spin);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (program_spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (program_spin),
				       GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (program_spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (program_spin), program_number);
    gtk_container_add (GTK_CONTAINER (event), program_spin);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* Patch name box (entry + load & save buttons) */
    box = gtk_hbox_new (FALSE, 1);
    widget_set_backing_store (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, FALSE, 1);

    /* Clickable Patch name label */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_widget_set_name (event, "patch_name");
    label = gtk_label_new ("Name:");
    widget_set_backing_store (label);
    gtk_container_add (GTK_CONTAINER (event), label);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* connect signal for clicking patch name label */
    g_signal_connect (G_OBJECT (event), "button_press_event",
		      GTK_SIGNAL_FUNC (param_label_button_press),
		      (gpointer)event);

    /* Patch name text entry */
    program_entry = gtk_entry_new_with_max_length (PATCH_NAME_LEN);
    widget_set_backing_store (program_entry);
    if (patch->name == NULL) {
	patch->name = get_patch_name_from_filename (patch->filename);
    }
    gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);

    gtk_entry_set_width_chars (GTK_ENTRY (program_entry), PATCH_NAME_LEN);
    gtk_entry_set_editable (GTK_ENTRY (program_entry), TRUE);
    gtk_box_pack_start (GTK_BOX (box), program_entry, FALSE, FALSE, 1);

    g_signal_connect (GTK_OBJECT (program_adj), "value_changed",
		      GTK_SIGNAL_FUNC (select_program),
		      (gpointer)program_entry);

    /* Load button */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    button = gtk_button_new_with_label ("Load");
    widget_set_backing_store (button);
    gtk_container_add (GTK_CONTAINER (event), button);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    g_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (load_program),
		      (gpointer)program_entry);

    /* Save button */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    button = gtk_button_new_with_label ("Save");
    widget_set_backing_store (button);
    gtk_container_add (GTK_CONTAINER (event), button);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    g_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (save_program),
		      (gpointer)program_entry);
		      
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    button = gtk_button_new_with_label ("Panic!");
    widget_set_backing_store (button);
    gtk_container_add (GTK_CONTAINER (event), button);
    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    g_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (engine_panic),
		      NULL);

    /* "Modified" label */
    if (show_modified) {
	modified_label = gtk_label_new ("[modified]");
    }
    else {
	modified_label = gtk_label_new ("");
    }
    widget_set_backing_store (modified_label);
#if GTK_MINOR_VERSION >= 6
    gtk_label_set_width_chars (GTK_LABEL (modified_label), 12);
#else
    gtk_widget_set_size_request (modified_label, 96, 12);
#endif
    gtk_box_pack_start (GTK_BOX (box), modified_label, FALSE, FALSE, 1);

    /* MIDI CH selector box (label + knob + label) */
    box = gtk_hbox_new (FALSE, 0);
    widget_set_backing_store (box);
    gtk_box_pack_end (GTK_BOX (hbox), box, FALSE, FALSE, 1);

    /* parameter name label */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_widget_set_name (event, "midi_channel");
    label = gtk_label_new ("MIDI Channel:");
    widget_set_backing_store (label);
    gtk_misc_set_padding (GTK_MISC (label), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0.8, 0.5);
    gtk_container_add (GTK_CONTAINER (event), label);

    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* connect signal for midi channel label */
    g_signal_connect (G_OBJECT (event), "button_press_event",
		      GTK_SIGNAL_FUNC (param_label_button_press),
		      (gpointer)event);

    /* create adjustment for knob widget */
    midi_channel_adj = gtk_adjustment_new (setting_midi_channel, 0, 16, 1, 1, 0);

    /* create knob widget */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    knob = gtk_knob_new (GTK_ADJUSTMENT (midi_channel_adj), knob_anim);
    widget_set_backing_store (knob);
    gtk_container_add (GTK_CONTAINER (event), knob);

    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* create label widget for text values */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    midi_channel_label = gtk_label_new (midi_ch_labels[setting_midi_channel]);
    gtk_misc_set_alignment (GTK_MISC (midi_channel_label), 0.2, 0.5);
    widget_set_backing_store (label);
#if GTK_MINOR_VERSION >= 6
    gtk_label_set_width_chars (GTK_LABEL (midi_channel_label), 5);
#else
    gtk_widget_set_size_request (midi_channel_label, 5, -1);
#endif
    gtk_container_add (GTK_CONTAINER (event), midi_channel_label);

    gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 1);

    /* connect knob signals */
    g_signal_connect (GTK_OBJECT (midi_channel_adj), "value_changed",
		      GTK_SIGNAL_FUNC (set_midi_channel),
		      (gpointer)(knob));

    /* connect value label event box signals */
    g_signal_connect_swapped (G_OBJECT (event), "button_press_event",
			      GTK_SIGNAL_FUNC (midi_label_button_press),
			      (gpointer)midi_channel_adj);
    g_signal_connect_swapped (G_OBJECT (event), "scroll_event",
			      GTK_SIGNAL_FUNC (midi_label_button_press),
			      (gpointer)midi_channel_adj);

    /* show the entire bank group at once */
    widget_set_custom_font (frame);
    gtk_widget_show_all (frame);
}


/*****************************************************************************
 *
 * SET_PATCH_LOAD_START()
 *
 *****************************************************************************/
void
set_patch_load_start(GtkWidget *widget, gpointer data) {
    patch_load_start = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
}


/*****************************************************************************
 *
 * CREATE_PATCH_LOAD_DIALOG()
 *
 *****************************************************************************/
void 	
create_patch_load_dialog(void) {
    GError		*error = NULL;
    GtkWidget		*hbox;
    GtkWidget		*label;
    GtkObject		*adj;

    /* this should only need to happen once */
    if (patch_load_dialog == NULL) {

	/* create dialog */
	patch_load_dialog = gtk_file_chooser_dialog_new ("PHASEX - Load Patch",
							 GTK_WINDOW (main_window),
							 GTK_FILE_CHOOSER_ACTION_OPEN,
							 GTK_STOCK_CANCEL,
							 GTK_RESPONSE_CANCEL,
							 GTK_STOCK_OPEN,
							 GTK_RESPONSE_ACCEPT,
							 NULL);
	gtk_window_set_wmclass (GTK_WINDOW (patch_load_dialog), "phasex", "phasex-load");
	gtk_window_set_role (GTK_WINDOW (patch_load_dialog), "patch-load");
	//gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (patch_load_dialog), TRUE);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (patch_load_dialog), FALSE);

	/* create spinbutton control for starting program number */
	hbox = gtk_hbox_new (FALSE, 8);
	label = gtk_label_new ("Load patches staring at program #:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 8);

	adj = gtk_adjustment_new (0, 0, (PATCH_BANK_SIZE - 1), 1, 8, 0);
	patch_load_start_spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (patch_load_start_spin), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (patch_load_start_spin),
					   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (patch_load_start_spin), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (patch_load_start_spin), 0);
	gtk_box_pack_start (GTK_BOX (hbox), patch_load_start_spin, FALSE, FALSE, 8);

	g_signal_connect (GTK_OBJECT (patch_load_start_spin), "value_changed",
			  GTK_SIGNAL_FUNC (set_patch_load_start),
			  (gpointer)patch_load_start_spin);

	gtk_widget_show_all (hbox);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (patch_load_dialog), hbox);

	/* realize the file chooser before telling it about files */
	gtk_widget_realize (patch_load_dialog);

	/* start in user patch dir (usually ~/.phasex/user-patches) */
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_load_dialog),
					     user_patch_dir);

	/* allow multiple patches to be selected */
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (patch_load_dialog), TRUE);

#if GTK_MINOR_VERSION >= 6
	gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (patch_load_dialog), TRUE);
#endif

	/* add system and user patch dirs as shortcut folders */
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (patch_load_dialog),
					      PATCH_DIR, &error);
	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (patch_load_dialog),
					      user_patch_dir, &error);
	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}

	/* add filename filters for .phx and all files */
	if (file_filter_all != NULL) {
	    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (patch_load_dialog), file_filter_all);
	}
	if (file_filter_patches != NULL) {
	    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (patch_load_dialog), file_filter_patches);
	    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (patch_load_dialog), file_filter_patches);
	}
    }
}


/*****************************************************************************
 *
 * RUN_PATCH_LOAD_DIALOG()
 *
 *****************************************************************************/
void
run_patch_load_dialog(GtkWidget *widget, gpointer data) {
    PATCH		*oldpatch = patch;
    PATCH_DIR_LIST	*pdir = patch_dir_list;
    char		*filename;
    GSList		*file_list;
    GSList		*cur;
    GError		*error;
    int			prog = patch_load_start;
    int			oldprog = program_number;

    /* create dialog if needed */
    if (patch_load_dialog == NULL) {
	create_patch_load_dialog ();
    }

    gtk_widget_show (patch_load_dialog);

    /* add all patch directories to shortcuts */
    while (pdir != NULL) {
	if (!pdir->load_shortcut) {
	    error = NULL;
	    gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (patch_load_dialog),
						  pdir->name, &error);
	    if (error != NULL) {
		if (debug) {
		    fprintf (stderr, "Error %d: %s\n", error->code, error->message);
		}
		g_error_free (error);
	    }
	    pdir->load_shortcut = 1;
	}
	pdir = pdir->next;
    }

    /* set filename and current directory */
    if ((patch->filename != NULL) && (*(patch->filename) != '\0') && (strncmp (patch->filename, "/tmp/patch", 10) != 0)) {
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (patch_load_dialog), patch->filename);
    }
    else if ((patch->directory != NULL) && (*(patch->directory) != '\0')) {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_load_dialog),
					     patch->directory);
    }
    else {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_load_dialog),
					     user_patch_dir);
    }

    /* set filter and hope that it takes */
    if (file_filter_patches != NULL) {
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (patch_load_dialog), file_filter_patches);
    }

    /* set position in patch bank to start loading patches */
    patch_load_start = program_number;
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (patch_load_start_spin), patch_load_start);

    /* run the dialog and load if necessary */
    g_idle_remove_by_data ((gpointer)main_window);
    if (gtk_dialog_run (GTK_DIALOG (patch_load_dialog)) == GTK_RESPONSE_ACCEPT) {

	/* get list of selected files from chooser */
	file_list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (patch_load_dialog));
	if (file_list != NULL) {
	    patch_load_start = gtk_spin_button_get_value (GTK_SPIN_BUTTON (patch_load_start_spin));

	    /* read in each patch and increment bank slot number */
	    prog = patch_load_start;
	    cur = file_list;
	    while (cur != NULL) {
		filename = (char *)cur->data;
		patch = &(bank[prog]);
		program_number = prog;
		if (read_patch (filename, prog) == 0) {
		    prog++;
		}
		if (prog >= PATCH_BANK_SIZE) {
		    break;
		}
		cur = g_slist_next (cur);
	    }

	    patch          = oldpatch;
	    program_number = oldprog;

	    g_slist_free (file_list);

	    /* display name of current patch in case it changed */
	    gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);

	    save_bank ();
	}
    }

    /* save this widget for next time */
    gtk_widget_hide (patch_load_dialog);
    g_idle_add (&param_idle_update, (gpointer)main_window);
}


/*****************************************************************************
 *
 * CREATE_PATCH_SAVE_DIALOG()
 *
 *****************************************************************************/
void
create_patch_save_dialog(void) {
    GError		*error = NULL;

    /* this should only need to happen once */
    if (patch_save_dialog == NULL) {

	/* create dialog */
	patch_save_dialog = gtk_file_chooser_dialog_new ("PHASEX - Save Patch",
					      GTK_WINDOW (main_window),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_window_set_wmclass (GTK_WINDOW (patch_save_dialog), "phasex", "phasex-save");
	gtk_window_set_role (GTK_WINDOW (patch_save_dialog), "patch-save");
	//gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (patch_save_dialog), TRUE);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (patch_save_dialog), FALSE);

	/* realize the file chooser before telling it about files */
	gtk_widget_realize (patch_save_dialog);

#if GTK_MINOR_VERSION >= 8
	/* this can go away once manual overwrite checks are proven to work properly */
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (patch_save_dialog),
							TRUE);
#endif
#if GTK_MINOR_VERSION >= 6
	gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (patch_save_dialog), TRUE);
#endif

	/* add user patch dir as shortcut folder (user cannot write to sys) */
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (patch_save_dialog),
					      user_patch_dir, &error);
	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}

	/* start in user patch dir (usually ~/.phasex/user-patches) */
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_save_dialog),
					     user_patch_dir);

	/* add filename filters for .phx and all files */
	if (file_filter_all != NULL) {
	    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (patch_save_dialog), file_filter_all);
	}
	if (file_filter_patches != NULL) {
	    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (patch_save_dialog), file_filter_patches);
	    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (patch_save_dialog), file_filter_patches);
	}
    }
}


/*****************************************************************************
 *
 * RUN_PATCH_SAVE_AS_DIALOG()
 *
 *****************************************************************************/
void
run_patch_save_as_dialog(GtkWidget *widget, gpointer data) {
    char		patchfile[PATH_MAX];
    char		*filename = (char *)data;
    PATCH_DIR_LIST	*pdir = patch_dir_list;
    GError		*error;

    /* create dialog if needed */
    if (patch_save_dialog == NULL) {
	create_patch_save_dialog ();
    }

    /* add all patch dirs as shortcut folders */
    while (pdir != NULL) {
	if ((!pdir->save_shortcut) && (strcmp (pdir->name, PATCH_DIR) != 0)) {
	    error = NULL;
	    gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (patch_save_dialog),
						  pdir->name, &error);
	    if (error != NULL) {
		if (debug) {
		    fprintf (stderr, "Error %d: %s\n", error->code, error->message);
		}
		g_error_free (error);
	    }
	    pdir->save_shortcut = 1;
	}
	pdir = pdir->next;
    }

    /* set filename and current directory */
    if ((filename == NULL) || (*filename == '\0')) {
	filename = patch->filename;
    }

    /* if we have a filename, and it's not the patchdump, set and select it */
    if ((filename != NULL) && (strcmp (filename, user_patchdump_file) != 0)) {
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (patch_save_dialog),
				       filename);
    }

    /* if there is no filename, try to set the current directory */
    else if ((patch->directory != NULL) && (*(patch->directory) != '\0')) {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_save_dialog),
					     patch->directory);
    }
    else {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (patch_save_dialog),
					     user_patch_dir);
    }

    /* set filter and hope that it takes */
    if (file_filter_patches != NULL) {
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (patch_save_dialog), file_filter_patches);
    }

    /* run the dialog and save if necessary */
    g_idle_remove_by_data ((gpointer)main_window);
    if (gtk_dialog_run (GTK_DIALOG (patch_save_dialog)) == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (patch_save_dialog));

	/* add .phx extension if necessary */
	snprintf (patchfile, PATH_MAX, "%s%s", filename,
		  (strstr (filename, ".phx\0") == NULL ? ".phx" : ""));
	g_free (filename);

	/* hide dialog and save patch */
	gtk_widget_hide (patch_save_dialog);
	switch (setting_bank_mem_mode) {
	case BANK_MEM_AUTOSAVE:
	    /* save patch without overwrite protection */
	    if (save_patch (patchfile, program_number) == 0) {
		gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);
		save_bank ();
	    }
	    break;
	case BANK_MEM_WARN:
	case BANK_MEM_PROTECT:
	    /* save patch with overwrite protection */
	    if (!check_patch_overwrite (filename)) {
		if (save_patch (patchfile, program_number) == 0) {
		    gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);
		    save_bank ();
		}
	    }
	    break;
	}
    }
    else {
	/* save this widget for next time */
	gtk_widget_hide (patch_save_dialog);
    }
    g_idle_add (&param_idle_update, (gpointer)main_window);
}


/*****************************************************************************
 *
 * ON_PATCH_SAVE_ACTIVATE()
 *
 *****************************************************************************/
void 
on_patch_save_activate(GtkWidget *widget, gpointer data) {
    char	*filename = (char *)data;

    /* if no filename was provided, use the current patch filename */
    if ((filename == NULL) || (filename[0] == '\0')) {
	filename = patch->filename;
    }

    /* if we still don't have a filename, run the save-as dialog */
    if (filename == NULL) {
	run_patch_save_as_dialog (NULL, NULL);
    }

    /* save the patch with the given filename */
    else {
	switch (setting_bank_mem_mode) {
	case BANK_MEM_AUTOSAVE:
	    /* save patch without overwrite protection */
	    if (save_patch (filename, program_number) == 0) {
		gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);
		save_bank ();
	    }
	    break;
	case BANK_MEM_WARN:
	case BANK_MEM_PROTECT:
	    /* save patch with overwrite protection */
	    if (!check_patch_overwrite (filename)) {
		if (save_patch (filename, program_number) == 0) {
		    gtk_entry_set_text (GTK_ENTRY (program_entry), patch->name);
		    save_bank ();
		}
	    }
	    break;
	}
    }
}
