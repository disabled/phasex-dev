/*****************************************************************************
 *
 * gtkui.c
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
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "phasex.h"
#include "config.h"
#include "patch.h"
#include "param.h"
#include "midi.h"
#include "jack.h"
#include "gtkui.h"
#include "gtkknob.h"
#include "callback.h"
#include "bank.h"
#include "settings.h"
#include "help.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif


pthread_mutex_t		gtkui_ready_mutex		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		gtkui_ready_cond		= PTHREAD_COND_INITIALIZER;

int			gtkui_ready			= 0;

GtkFileFilter		*file_filter_all		= NULL;
GtkFileFilter		*file_filter_patches		= NULL;
GtkFileFilter		*file_filter_map		= NULL;

GtkWidget		*main_window			= NULL;
GtkWidget		*main_menubar			= NULL;
GtkWidget		*midimap_load_dialog		= NULL;
GtkWidget		*midimap_save_dialog		= NULL;
GtkWidget		*cc_edit_dialog			= NULL;
GtkWidget		*cc_edit_spin			= NULL;

GtkObject		*cc_edit_adj			= NULL;

GtkKnobAnim		*knob_anim			= NULL;

GtkCheckMenuItem	*menu_item_fullscreen		= NULL;
GtkCheckMenuItem	*menu_item_notebook		= NULL;
GtkCheckMenuItem	*menu_item_one_page		= NULL;
GtkCheckMenuItem	*menu_item_autoconnect		= NULL;
GtkCheckMenuItem	*menu_item_manualconnect	= NULL;
GtkCheckMenuItem	*menu_item_autosave		= NULL;
GtkCheckMenuItem	*menu_item_warn			= NULL;
GtkCheckMenuItem	*menu_item_protect		= NULL;

GtkCheckMenuItem	*menu_item_midi_ch[18] =	/* 16 ch + omni + NULL */
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int			cc_edit_active			= 0;
int			cc_edit_ignore_midi		= 0;
int			cc_edit_cc_num			= -1;
int			cc_edit_param_id		= -1;

int			forced_quit			= 0;
int			gtkui_restarting		= 0;


/*****************************************************************************
 *
 * GTKUI_THREAD()
 *
 * The entire GTK UI operates in this thread.
 *
 *****************************************************************************/
void *
gtkui_thread(void *arg) {
    char		*filename;

    gtkui_restarting = 0;

    // set up environment for loading gtk theme before gtk_init ()
    set_theme_env ();

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    gtk_set_locale ();
    gtk_init (NULL, NULL);

    /* everything gets created from here */
    create_file_filters ();
    create_main_window ();
    create_patch_load_dialog ();
    create_patch_save_dialog ();
    create_midimap_load_dialog ();
    create_midimap_save_dialog ();

    /* add idle handler to update widgets */
    g_idle_add (&param_idle_update, (gpointer)main_window);

    /* broadcast the gtkui ready condition */
    pthread_mutex_lock (&gtkui_ready_mutex);
    gtkui_ready = 1;
    pthread_cond_broadcast (&gtkui_ready_cond);
    pthread_mutex_unlock (&gtkui_ready_mutex);

    /* reload first patch, now that widgets can respond */
    if (bank[0].filename != NULL) {
	filename = strdup (bank[0].filename);
	read_patch (filename, 0);
    }

    /* gtkui thread sits and runs here */
    gtk_main ();

    /* cleanup and shut everything else down */
    phasex_shutdown ("Thank you for using PHASEX!\n"
             "(C) 2010 Anton Kormakov <assault64@gmail.com>\n"
		     "(C) 1999-2009 William Weston <weston@sysex.net>\n"
		     "Released under the GNU Public License, Ver. 2\n");

    /* end of gtkui thread */
    return NULL;
}


/*****************************************************************************
 *
 * START_GTKUI_THREAD()
 *
 *****************************************************************************/
void
start_gtkui_thread(void) {
    if (pthread_create (&gtkui_thread_p, NULL, &gtkui_thread, NULL) != 0) {
	phasex_shutdown ("Unable to start gtkui thread.\n");
    }

}


/*****************************************************************************
 *
 * CREATE_FILE_FILTERS()
 *
 *****************************************************************************/
void
create_file_filters (void) {
    if (file_filter_all != NULL) {
	g_object_unref (G_OBJECT (file_filter_all));
    }
    file_filter_all = gtk_file_filter_new ();
    gtk_file_filter_set_name (file_filter_all, "[*] All Files");
    gtk_file_filter_add_pattern (file_filter_all, "*");
    g_object_ref_sink (G_OBJECT (file_filter_all));

    if (file_filter_patches != NULL) {
	g_object_unref (G_OBJECT (file_filter_patches));
    }
    file_filter_patches = gtk_file_filter_new ();
    gtk_file_filter_set_name (file_filter_patches, "[*.phx] PHASEX Patches");
    gtk_file_filter_add_pattern (file_filter_patches, "*.phx");
    g_object_ref_sink (G_OBJECT (file_filter_patches));

    if (file_filter_map != NULL) {
	g_object_unref (GTK_OBJECT (file_filter_map));
    }
    file_filter_map = gtk_file_filter_new ();
    gtk_file_filter_set_name (file_filter_map, "[*.map] PHASEX MIDI Maps");
    gtk_file_filter_add_pattern (file_filter_map, "*.map");
    g_object_ref_sink (G_OBJECT (file_filter_map));
}


/*****************************************************************************
 *
 * PARAM_IDLE_UPDATE()
 *
 *****************************************************************************/
gboolean 
param_idle_update(gpointer data) {
    static int		counter		= 0;
    static int		walking		= 0;
    int			num_updated	= 0;
    int			j;

    /* watch for global shutdown */
    if (shutdown) {
	gdk_threads_leave ();
	gtk_main_quit ();
	return FALSE;
    }

    /* check for pending program change request */
    if (program_change_request != -1) {
	j = program_change_request;
	program_change_request = -1;

	/* setting the adjustment value will trigger the callback */
	if (program_change_request != program_number) {
	    gtk_adjustment_set_value (GTK_ADJUSTMENT (program_adj), (gfloat)j);
	}
    }

    /* change patch modified label, if needed */
    if (patch->modified != show_modified) {
	if (patch->modified) {
	    gtk_label_set_text (GTK_LABEL (modified_label), "[modified]");
	}
	else {
	    gtk_label_set_text (GTK_LABEL (modified_label), "");
	}
	show_modified = patch->modified;
    }

    /* update widgets for up to 48 parameters per cycle */
    for (j = 0; j < NUM_PARAMS; j++) {
	if (num_updated >= 48) {
	    break;
	}

	/* only perform gui updates for updated parameters */
	if (param[j].updated) {
	    num_updated++;
	    switch (param[j].type) {
	    case PARAM_TYPE_INT:
	    case PARAM_TYPE_REAL:
		gtk_adjustment_set_value (GTK_ADJUSTMENT (param[j].adj),
					  param[j].int_val[program_number]);
		break;
	    case PARAM_TYPE_RATE:
		gtk_entry_set_text (GTK_ENTRY (param[j].text),
				    (param[j].list_labels[param[j].cc_val[program_number]]));
		gtk_adjustment_set_value (GTK_ADJUSTMENT (param[j].adj),
					  param[j].int_val[program_number]);
		break;
	    case PARAM_TYPE_DTNT:
		gtk_label_set_text (GTK_LABEL (param[j].text),
				    (param[j].list_labels[param[j].cc_val[program_number]]));
		gtk_adjustment_set_value (GTK_ADJUSTMENT (param[j].adj),
					  param[j].int_val[program_number]);
		break;
	    case PARAM_TYPE_BOOL:
		if ((param[j].cc_val[program_number] == 0) && (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param[j].button[0])))) {
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (param[j].button[0]), TRUE);
		}
		else if ((param[j].cc_val[program_number] == 1) && (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param[j].button[1])))) {
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (param[j].button[1]), TRUE);
		}
		break;
	    case PARAM_TYPE_BBOX:
		if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (param[j].button[param[j].cc_val[program_number]]))) {
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (param[j].button[param[j].cc_val[program_number]]), TRUE);
		}
		break;
	    }
	    param[j].cc_prev = param[j].cc_cur;
	    param[j].cc_cur  = param[j].cc_val[program_number];
	    param[j].updated = 0;
	}
    }

    /* walking parameter update -- queue up one parameter per cycle */
    /* currently, this is an ugly hack for parameter updates that happen */
    /* while window is not visible and backing store is enabled */
    walking++;
    if (walking >= NUM_PARAMS) {
	walking = 0;
    }
    if (param[walking].knob != NULL) {
	gtk_widget_queue_draw (GTK_WIDGET (param[walking].knob));
	if (param[walking].spin != NULL) {
	    gtk_widget_queue_draw (GTK_WIDGET (param[walking].spin));
	}
	else if (param[walking].text != NULL) {
	    gtk_widget_queue_draw (GTK_WIDGET (param[walking].text));
	}
    }
    else if (param[walking].button[0] != NULL) {
	gtk_widget_queue_draw (GTK_WIDGET (param[walking].button[0]));
	if (param[walking].button[1] != NULL) {
	    gtk_widget_queue_draw (GTK_WIDGET (param[walking].button[1]));
	    if (param[walking].button[2] != NULL) {
		gtk_widget_queue_draw (GTK_WIDGET (param[walking].button[2]));
		if (param[walking].button[3] != NULL) {
		    gtk_widget_queue_draw (GTK_WIDGET (param[walking].button[3]));
		}
	    }
	}
    }

    /* check for active cc edit */
    if (cc_edit_active) {
	if (cc_edit_cc_num > -1) {
	    if (cc_edit_spin != NULL) {
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (cc_edit_spin), cc_edit_cc_num);
		cc_edit_cc_num = -1;
	    }

	}
    }

    /* dump the patch and midimap every ~ 5-20 sec */
    counter++;
    if (counter == 50) {
	save_midimap (user_midimap_dump_file);
    }
    else if (counter == 100) {
	if (setting_bank_mem_mode == BANK_MEM_AUTOSAVE) {
	    save_patch (patch->filename, program_number);
	    patch->modified = 0;
	}
	else {
	    save_patch (user_patchdump_file, program_number);
	}

	/* undocumented: new patches can be fed in through /tmp/patchload */
	if (read_patch ("/tmp/patchload", program_number) == 0) {
	    rename ("/tmp/patchload", "/tmp/patchlast");
	}
	counter = 0;
    }

    /* sleep for time 20000 to 220000 us */
    usleep (setting_gui_idle_sleep);

    return TRUE;
}


/*****************************************************************************
 *
 * UPDATE_PARAM_CC_NUM()
 *
 *****************************************************************************/
void
update_param_cc_num(GtkWidget *widget, gpointer data) {
    PARAM	*param = (PARAM *)data;
    int		old_cc;
    int		new_cc;
    int		j;
    int		k;
    int		id;

    if (param != NULL) {
	id     = param->id;
	old_cc = param->cc_num;
	new_cc = (int)(floor (GTK_ADJUSTMENT (widget)->value));

#ifdef EXTRA_DEBUG
	if (debug) {
	    fprintf (stderr, "Updating MIDI CC # (old=%03d new=%03d) for <%s>\n",
		     old_cc, new_cc, param->name);
	}
#endif

	/* unmap old cc num from ccmatrix */
	j = 0;
	while ((ccmatrix[old_cc][j] >= 0) && (j < 16)) {
	    if (ccmatrix[old_cc][j] == id) {
		for (k = j; k < 16; k++) {
		    ccmatrix[old_cc][k] = ccmatrix[old_cc][k+1];
		}
	    }
	    j++;
	}

	/* map new cc num into ccmatrix */
	j = 0;
	while ((ccmatrix[new_cc][j] >= 0) && (j < 16)) {
	    j++;
	}
	if (j < 16) {
	    ccmatrix[new_cc][j] = id;
	    j++;
	}
	if (j < 16) {
	    ccmatrix[new_cc][j] = -1;
	}

	/* keep track of cc num in param too */
	if (param->cc_num != new_cc) {
	    midimap_modified = 1;
	    param->cc_num    = new_cc;
	}
    }
}


/*****************************************************************************
 *
 * UPDATE_PARAM_LOCKED()
 *
 *****************************************************************************/
void
update_param_locked(GtkWidget *widget, gpointer data) {
    PARAM	*param = (PARAM *)data;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	param->locked = 1;
    }
    else {
	param->locked = 0;
    }
}


/*****************************************************************************
 *
 * UPDATE_PARAM_IGNORE()
 *
 *****************************************************************************/
void
update_param_ignore(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
	cc_edit_ignore_midi = 1;
    }
    else {
	cc_edit_ignore_midi = 0;
    }
}


/*****************************************************************************
 *
 * CLOSE_CC_EDIT_DIALOG()
 *
 *****************************************************************************/
void 
close_cc_edit_dialog(GtkWidget *widget, gpointer data) {
    cc_edit_active      = 0;
    cc_edit_ignore_midi = 0;
    cc_edit_cc_num      = -1;
    cc_edit_param_id    = -1;
    gtk_widget_destroy (cc_edit_dialog);
    cc_edit_dialog      = NULL;
    cc_edit_adj         = NULL;
    cc_edit_spin        = NULL;
}


/*****************************************************************************
 *
 * PARAM_LABEL_BUTTON_PRESS()
 *
 * Callback for a button press event on a param label event box.
 *
 *****************************************************************************/
void
param_label_button_press(GtkWidget *widget, GdkEventButton *event) {
    GtkWidget		*hbox;
    GtkWidget		*label;
    GtkWidget		*separator;
    GtkWidget		*lock_button;
    GtkWidget		*ignore_button;
    int			j;
    int			id		= -1;
    int			same_param_id	= 0;
    const char		*widget_name;

    /* find param id by looking up name from widget in param table */
    widget_name = gtk_widget_get_name (widget);
    for (j = 0; j < NUM_HELP_PARAMS; j++) {
	if (strcmp (widget_name, param[j].name) == 0) {
	    id = j;
	    break;
	}
    }

    /* return now if matching parameter is not found */
    if (id == -1) {
	return;
    }

    /* check which button got clicked */
    switch (event->button) {
    case 1:
	/* nothing for left button */
	break;

    case 2:
	/* display parameter help for middle click */
	display_param_help (id);
	break;

    case 3:
	/* only midi-map real paramaters */
	if (id >= NUM_PARAMS) {
	    return;
	}

	/* destroy current edit window */
	if (cc_edit_dialog != NULL) {
	    if (id == cc_edit_param_id) {
		same_param_id = 1;
	    }
	    gtk_widget_destroy (cc_edit_dialog);
	}

	/* only create new edit window if it's a new or different parameter */
	if (!same_param_id) {
	    cc_edit_dialog = gtk_dialog_new_with_buttons ("Update MIDI Controller",
							  GTK_WINDOW (main_window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_STOCK_CLOSE,
							  GTK_RESPONSE_CLOSE,
							  NULL);
	    gtk_window_set_wmclass (GTK_WINDOW (cc_edit_dialog), "phasex", "phasex-edit");
	    gtk_window_set_role (GTK_WINDOW (cc_edit_dialog), "controller-edit");

	    label = gtk_label_new("Enter a new MIDI controller number "
				  "below or simply touch the desired "
				  "controller to map the parameter "
				  "automatically.  Locked parameters "
				  "allow only explicit updates from the "
				  "user interface.");
	    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	    gtk_misc_set_padding (GTK_MISC (label), 8, 0);
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				label, TRUE, FALSE, 8);

	    separator = gtk_hseparator_new ();
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				separator, TRUE, FALSE, 0);

	    /* parameter name */
	    label = gtk_label_new (param_help[id].label);
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				label, TRUE, FALSE, 8);

	    separator = gtk_hseparator_new ();
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				separator, TRUE, FALSE, 0);

	    /* select midi controller */
	    hbox = gtk_hbox_new (FALSE, 0);
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				hbox, TRUE, FALSE, 8);

	    cc_edit_adj = gtk_adjustment_new (param[id].cc_num, -1, 127,
					      1, 10, 0);

	    cc_edit_spin = gtk_spin_button_new (GTK_ADJUSTMENT (cc_edit_adj),
						0, 0);
	    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (cc_edit_spin),
					 TRUE);
	    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (cc_edit_spin),
					       GTK_UPDATE_IF_VALID);
	    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (cc_edit_spin),
					       TRUE);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (cc_edit_spin),
				       param[id].cc_num);
	    gtk_box_pack_end (GTK_BOX (hbox), cc_edit_spin, FALSE, FALSE, 8);

	    label = gtk_label_new ("MIDI Controller #");
	    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 8);

	    /* parameter locking */
	    hbox = gtk_hbox_new (FALSE, 0);
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				hbox, TRUE, TRUE, 8);

	    lock_button = gtk_check_button_new ();
	    if (param[id].locked) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lock_button), TRUE);
	    }
	    else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lock_button), FALSE);
	    }
	    gtk_box_pack_end (GTK_BOX (hbox), lock_button, FALSE, FALSE, 8);

	    label = gtk_label_new ("Lock parameter (manual adjustment only)?");
	    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 8);

	    /* midi ignore */
	    hbox = gtk_hbox_new (FALSE, 0);
	    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cc_edit_dialog)->vbox),
				hbox, TRUE, TRUE, 8);

	    ignore_button = gtk_check_button_new ();
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ignore_button), FALSE);
	    gtk_box_pack_end (GTK_BOX (hbox), ignore_button, FALSE, FALSE, 8);

	    label = gtk_label_new ("Ignore inbound MIDI for this dialog?");
  	    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 8);

	    //widget_set_custom_font (cc_edit_dialog);
	    gtk_widget_show_all (cc_edit_dialog);

	    /* connect signals */
	    g_signal_connect (GTK_OBJECT (cc_edit_adj), "value_changed",
			      GTK_SIGNAL_FUNC (update_param_cc_num),
			      (gpointer)&(param[id]));

	    g_signal_connect (GTK_OBJECT (lock_button), "toggled",
			      GTK_SIGNAL_FUNC (update_param_locked),
			      (gpointer)&(param[id]));

	    g_signal_connect (GTK_OBJECT (ignore_button), "toggled",
			      GTK_SIGNAL_FUNC (update_param_ignore),
			      (gpointer)&(param[id]));

	    g_signal_connect_swapped (G_OBJECT (cc_edit_dialog), "response",
				      GTK_SIGNAL_FUNC (close_cc_edit_dialog),
				      (gpointer)cc_edit_dialog);

	    g_signal_connect (G_OBJECT (cc_edit_dialog), "destroy",
			      GTK_SIGNAL_FUNC (close_cc_edit_dialog),
			      (gpointer)cc_edit_dialog);

	    /* set internal info */
	    cc_edit_cc_num   = -1;
	    cc_edit_param_id = id;
	    cc_edit_active   = 1;
	}

	/* otherwise, just get out of edit mode completely */
	else {
	    /* set internal info */
	    cc_edit_cc_num   = -1;
	    cc_edit_param_id = -1;
	    cc_edit_active   = 0;
	}
    }
}


/*****************************************************************************
 *
 * DETENT_LABEL_BUTTON_PRESS()
 *
 * Callback for a button press event on a detent parameter value label.
 *
 *****************************************************************************/
void
detent_label_button_press(gpointer data1, gpointer data2) {
    PARAM		*param	= (PARAM *)data1;
    GdkEventAny		*event	= (GdkEventAny *)data2;
    GdkEventButton	*button	= (GdkEventButton *)data2;
    GdkEventScroll	*scroll	= (GdkEventScroll *)data2;
    int			p	= program_number;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
	switch (button->button) {
	case 1:		/* left button */
	    if (param->cc_val[p] > 0) {
		param->cc_val[p]--;
		if (param->cc_val[p] != param->cc_cur) {
		    param->callback ((gpointer)(main_window), (gpointer)(param));
		    patch->modified = 1;
		}
	    }
	    break;
	case 2:		/* middle button */
	    param->cc_val[p] = (param->cc_limit + 1) / 2;
	    if (param->cc_val[p] != param->cc_cur) {
		param->callback ((gpointer)(main_window), (gpointer)(param));
		patch->modified = 1;
	    }
	    break;
	case 3:		/* right button */
	    if ((param->cc_val[p]) < (param->cc_limit)) {
		param->cc_val[p]++;
		if (param->cc_val[p] != param->cc_cur) {
		    param->callback ((gpointer)(main_window), (gpointer)(param));
		    patch->modified = 1;
		}
	    }
	    break;
	}
	break;
    case GDK_SCROLL:
	switch (scroll->direction) {
	case GDK_SCROLL_DOWN:
	    if (param->cc_val[p] > 0) {
		param->cc_val[p]--;
		if (param->cc_val[p] != param->cc_cur) {
		    param->callback ((gpointer)(main_window), (gpointer)(param));
		    patch->modified = 1;
		}
	    }
	    break;
	case GDK_SCROLL_UP:
	    if ((param->cc_val[p]) < (param->cc_limit)) {
		param->cc_val[p]++;
		if (param->cc_val[p] != param->cc_cur) {
		    param->callback ((gpointer)(main_window), (gpointer)(param));
		    patch->modified = 1;
		}
	    }
	    break;
	default:
	    /* must handle default case for scroll direction to keep gtk quiet */
	    break;
	}
	break;
    default:
	/* must handle default case for event type to keep gtk quiet */
	break;
    }
}


/*****************************************************************************
 *
 * CREATE_PARAM_INPUT()
 *
 * Create a parameter input widget based on PARAM entry.
 * Designed to run out of a loop.
 *
 *****************************************************************************/
void 
create_param_input(GtkWidget *main_window, GtkWidget *table,
		   gint col, gint row, PARAM *param) {
    GtkWidget		*event;
    GtkWidget		*label;
    GtkWidget		*knob;
    GtkWidget		*spin;
    GtkWidget		*hbox;
    GtkWidget		*button_table;
    GtkWidget		*button;
    GtkWidget		*entry;
    GtkObject		*adj;
    int			j;

    /* event box for clickable param label */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_widget_set_name (event, param->name);
    gtk_table_attach (GTK_TABLE (table), event,
		      col, col + 1, row, row + 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      0, 0);

    /* parameter name label */
    label = gtk_label_new (param->label);
    widget_set_custom_font (label);
    widget_set_backing_store (label);
    gtk_misc_set_padding (GTK_MISC (label), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_container_add (GTK_CONTAINER (event), label);

    /* connect signal for clicking param name label */
    g_signal_connect (G_OBJECT (event), "button_press_event",
		      GTK_SIGNAL_FUNC (param_label_button_press),
		      (gpointer)event);

    /* create different widget combos depending on parameter type */
    switch (param->type) {
    case PARAM_TYPE_INT:
    case PARAM_TYPE_REAL:
	/* create adjustment for real/integer knob and spin widgets */
	adj = gtk_adjustment_new (param->int_val[0],
				  (0 + param->pre_inc),
				  (param->cc_limit + param->pre_inc),
				  1, param->leap, 0);
	param->adj = adj;

	/* create knob widget */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	knob = gtk_knob_new (GTK_ADJUSTMENT (adj), knob_anim);
	widget_set_backing_store (knob);
	param->knob = GTK_KNOB (knob);
	gtk_container_add (GTK_CONTAINER (event), knob);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 1, col + 2, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* create spin button */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	widget_set_custom_font (spin);
	widget_set_backing_store (spin);
	param->spin = spin;
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
					   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spin), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), param->int_val[0]);
	gtk_container_add (GTK_CONTAINER (event), spin);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 2, col + 3, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* connect signals */
	g_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (param->callback),
			  (gpointer)param);
	break;
    case PARAM_TYPE_RATE:
	/* create adjustment for knob widget */
	adj = gtk_adjustment_new (param->int_val[0], 0, param->cc_limit,
				  1, param->leap, 0);
	param->adj = adj;

	/* create knob widget */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	knob = gtk_knob_new (GTK_ADJUSTMENT (adj), knob_anim);
	widget_set_backing_store (knob);
	param->knob = GTK_KNOB (knob);
	gtk_container_add (GTK_CONTAINER (event), knob);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 1, col + 2, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* create entry widget for text values */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	entry = gtk_entry_new_with_max_length (6);
	widget_set_custom_font (entry);
	widget_set_backing_store (entry);
	param->text = entry;
	gtk_entry_set_text (GTK_ENTRY (entry),
			    param->list_labels[param->cc_val[0]]);
	gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);
	gtk_entry_set_editable (GTK_ENTRY (entry), TRUE);
	gtk_container_add (GTK_CONTAINER (event), entry);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 2, col + 3, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* connect signals */
	g_signal_connect (GTK_OBJECT (entry), "activate",
			  GTK_SIGNAL_FUNC (param->callback),
			  (gpointer)param);

	g_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (param->callback),
			  (gpointer)param);

	g_signal_connect_swapped (G_OBJECT (event), "scroll_event",
				  GTK_SIGNAL_FUNC (detent_label_button_press),
				  (gpointer)param);
	break;
    case PARAM_TYPE_DTNT:
	/* create adjustment for knob widget */
	adj = gtk_adjustment_new (param->int_val[0], 0, param->cc_limit,
				  1, param->leap, 0);
	param->adj = adj;

	/* create knob widget */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	knob = gtk_knob_new (GTK_ADJUSTMENT (adj), knob_anim);
	widget_set_backing_store (knob);
	param->knob = GTK_KNOB (knob);
	gtk_container_add (GTK_CONTAINER (event), knob);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 1, col + 2, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* create label widget for text values */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	label = gtk_label_new (param->list_labels[param->cc_val[0]]);
	widget_set_custom_font (label);
	widget_set_backing_store (label);
	param->text = label;
#if GTK_MINOR_VERSION >= 6
	gtk_label_set_width_chars (GTK_LABEL (label), 10);
#else
	gtk_widget_set_size_request (label, 80, -1);
#endif
	gtk_container_add (GTK_CONTAINER (event), label);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 2, col + 3, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  0, 0);

	/* connect knob signals */
	g_signal_connect (GTK_OBJECT (adj), "value_changed",
			  GTK_SIGNAL_FUNC (param->callback),
			  (gpointer)param);

	/* connect value label event box signals */
	g_signal_connect_swapped (G_OBJECT (event), "button_press_event",
				  GTK_SIGNAL_FUNC (detent_label_button_press),
				  (gpointer)param);
	g_signal_connect_swapped (G_OBJECT (event), "scroll_event",
				  GTK_SIGNAL_FUNC (detent_label_button_press),
				  (gpointer)param);
	break;
    case PARAM_TYPE_BOOL:
	/* create button box */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	hbox = gtk_hbox_new (FALSE, 0);
	widget_set_backing_store (hbox);
	gtk_container_add (GTK_CONTAINER (event), hbox);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 1, col + 3, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* create buttons */
	button = NULL;
	j = 0;
	while (param->list_labels[j] != NULL) {
	    /* create individual button widget */
	    event = gtk_event_box_new ();
	    widget_set_backing_store (event);
	    button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
	    widget_set_backing_store (button);
	    widget_set_backing_store (GTK_WIDGET (GTK_TOGGLE_BUTTON (GTK_CHECK_BUTTON (GTK_RADIO_BUTTON (button)))));
	    gtk_container_add (GTK_CONTAINER (event), button);
	    gtk_box_pack_start (GTK_BOX (hbox),	event,
				FALSE, FALSE, 0);

	    /* create label to sit next to button */
	    event = gtk_event_box_new ();
	    widget_set_backing_store (event);
	    label = gtk_label_new (param->list_labels[j]);
	    widget_set_custom_font (label);
	    widget_set_backing_store (label);
	    gtk_container_add (GTK_CONTAINER (event), label);
	    gtk_box_pack_start (GTK_BOX (hbox),	event,
				FALSE, FALSE, 0);

	    param->button[j] = button;
	    j++;
	}

	/* set active button before connecting signals */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (param->button[param->cc_val[0]]), TRUE);

	/* connect toggled signal for each button */
	j = 0;
	while (param->list_labels[j] != NULL) {
	    g_signal_connect (GTK_OBJECT (button), "toggled",
			      GTK_SIGNAL_FUNC (param->callback),
			      (gpointer)param);
	    j++;
	}

	break;
    case PARAM_TYPE_BBOX:
	/* create button box */
	event = gtk_event_box_new ();
	widget_set_backing_store (event);
	button_table = gtk_table_new (2, (param->cc_limit + 1), FALSE);
	widget_set_backing_store (button_table);
	gtk_container_add (GTK_CONTAINER (event), button_table);
	gtk_table_attach (GTK_TABLE (table), event,
			  col + 1, col + 3, row, row + 1,
			  (GtkAttachOptions) (GTK_EXPAND),
			  (GtkAttachOptions) (GTK_EXPAND),
			  0, 0);

	/* create buttons */
	button = NULL;
	j = 0;
	while (param->list_labels[j] != NULL) {
	    /* create label to sit over the button */
	    event = gtk_event_box_new ();
	    widget_set_backing_store (event);
	    label = gtk_label_new (param->list_labels[j]);
	    widget_set_custom_font (label);
	    widget_set_backing_store (label);
	    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
	    gtk_container_add (GTK_CONTAINER (event), label);
	    gtk_table_attach (GTK_TABLE (button_table), event,
			      j, (j + 1), 0, 1,
			      (GtkAttachOptions) (GTK_EXPAND),
			      (GtkAttachOptions) (GTK_EXPAND),
			      0, 0);

	    /* create individual button widget */
	    event = gtk_event_box_new ();
	    widget_set_backing_store (event);
	    button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
	    widget_set_backing_store (button);
	    widget_set_backing_store (GTK_WIDGET (GTK_TOGGLE_BUTTON (GTK_CHECK_BUTTON (GTK_RADIO_BUTTON (button)))));
	    gtk_container_add (GTK_CONTAINER (event), button);
	    gtk_table_attach (GTK_TABLE (button_table), event,
			      j, (j + 1), 1, 2,
			      (GtkAttachOptions) (GTK_EXPAND),
			      (GtkAttachOptions) (GTK_EXPAND),
			      0, 0);

	    param->button[j] = button;
	    j++;
	}

	/* set active button before connecting signals */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (param->button[param->cc_val[0]]), TRUE);

	/* connect toggled signal for each button */
	j = 0;
	while (param->list_labels[j] != NULL) {
	    g_signal_connect (GTK_OBJECT (param->button[j]), "toggled",
			      GTK_SIGNAL_FUNC (param->callback),
			      (gpointer)param);
	    j++;
	}

	break;
    }
}


/*****************************************************************************
 *
 * CREATE_PARAM_GROUP()
 *
 * Create a group of parameter input widgets complete with a frame,
 * and attach to a vbox.
 *
 *****************************************************************************/
void 
create_param_group(GtkWidget *main_window, GtkWidget *parent_table,
		   PARAM_GROUP *param_group, int x, int y0, int y1) {
    GtkWidget		*frame;
    GtkWidget		*table;
    GtkWidget		*label;
    GtkWidget		*event;
    int			num_params = 0;
    int			j;

    /* Find number of parameters */
    while ((num_params < 15) && (param_group->param_list[num_params] > -1)) {
	num_params++;
    }

    /* Create frame with label, alignment, and table */
    frame = gtk_frame_new (NULL);
    widget_set_backing_store (frame);

    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    label = gtk_label_new (param_group->label);
    widget_set_custom_font (label);
    widget_set_backing_store (label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_container_add (GTK_CONTAINER (event), label);
    gtk_frame_set_label_widget (GTK_FRAME (frame), event);

    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_container_add (GTK_CONTAINER (frame), event);

    table = gtk_table_new (num_params, 3, FALSE);
    widget_set_backing_store (table);
    gtk_table_set_col_spacings (GTK_TABLE (table), 0);
    gtk_table_set_row_spacings (GTK_TABLE (table), 0);
    gtk_container_add (GTK_CONTAINER (event), table);

    /* Create each parameter input within the group */
    for (j = 0; j < num_params; j++) {
	create_param_input (main_window, table, 0, j,
			    &(param[param_group->param_list[j]]));
    }

    /* Attach whole frame to parent */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);
    gtk_container_add (GTK_CONTAINER (event), frame);
    gtk_table_attach (GTK_TABLE (parent_table), event,
		      x, (x + 1), y0, y1,
		      (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		      (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		      0, 0);
}


/*****************************************************************************
 *
 * CREATE_PARAM_PAGE()
 *
 * Create a page of parameter input widgets.
 * Designed to be run out of a loop.
 *
 *****************************************************************************/
void 
create_param_page(GtkWidget *main_window, GtkWidget *notebook,
		  PARAM_PAGE *param_page, int page_num) {
    GtkWidget		*label;
    GtkWidget		*table;
    GtkWidget		*page;
    GtkWidget		*event;
    int			j = 0;
    int			x = 0;

    /* find out how many columns */
    for (j = 0; j < NUM_PARAM_GROUPS; j++) {
	if (param_group[j].notebook_x > x) {
	    x = param_group[j].notebook_x;
	}
    }

    /* Start with a table, so param groups can be attached, same as in one_page mode */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);

    table = gtk_table_new ((x + 1), 79, FALSE);
    widget_set_backing_store (table);
    gtk_container_add (GTK_CONTAINER (event), table);

    /* Run through all param groups, and add the ones for this page */
    for (j = 0; j < NUM_PARAM_GROUPS; j++) {
	if (param_group[j].notebook_page == page_num) {
	    create_param_group (main_window, table, &(param_group[j]),
				param_group[j].notebook_x,
				param_group[j].notebook_y0,
				param_group[j].notebook_y1);
	}
    }
    gtk_container_add (GTK_CONTAINER (notebook), event);

    /* Set the label */
    label = gtk_label_new (param_page->label);
    widget_set_custom_font (label);
    widget_set_backing_store (label);

    page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num);

#if GTK_MINOR_VERSION >= 10
    gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (notebook), page, FALSE);
#endif
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), page, label);
}


/*****************************************************************************
 *
 * CREATE_PARAM_NOTEBOOK()
 *
 * Create notebook of parameters, all ready to go.
 *
 *****************************************************************************/
void 
create_param_notebook(GtkWidget *main_window, GtkWidget *box, PARAM_PAGE *param_page) {
    GtkWidget		*table;
    GtkWidget		*notebook;
    int			j;

    /* create a notebook and attach it to the main window */
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);

    /* build the pages for the notebook */
    for (j = 0; j < NUM_PARAM_PAGES; j++) {
	create_param_page (main_window, notebook, &(param_page[j]), j);
    }

    gtk_box_pack_start (GTK_BOX (box), notebook, TRUE, TRUE, 0);

    /* create the patch bank group at the bottom */
    table = gtk_table_new (7, 7, FALSE);
    widget_set_backing_store (table);
    create_bank_group (main_window, table, 0, 7, 0, 7);
    gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
}


/*****************************************************************************
 *
 * CREATE_PARAM_ONE_PAGE()
 *
 * Create one full screen of parameters (currently ~1169x841).
 *
 *****************************************************************************/
void 
create_param_one_page(GtkWidget *main_window, GtkWidget *box, PARAM_PAGE *param_page) {
    GtkWidget		*table;
    int			j;

    /* Start with a table, so param groups can be attached, same as for notebook pages */
    table = gtk_table_new (6, 109, FALSE);
    widget_set_backing_store (table);
    gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);

    /* create the param groups that will be attached to this table */
    for (j = 0; j < NUM_PARAM_GROUPS; j++) {
	create_param_group (main_window, table, &(param_group[j]),
			    param_group[j].full_x,
			    (param_group[j].full_y0 + 7),
			    (param_group[j].full_y1 + 7));
    }

    /* create the patch bank group at the top */
    create_bank_group (main_window, table, 0, 7, 0, 7);
}


/*****************************************************************************
 *
 * CREATE_MIDIMAP_LOAD_DIALOG()
 *
 *****************************************************************************/
void
create_midimap_load_dialog(void) {
    GError		*error = NULL;

    /* create new dialog only if it doesn't already exist */
    if (midimap_load_dialog == NULL) {
	midimap_load_dialog = gtk_file_chooser_dialog_new ("PHASEX - Load Midimap",
							   GTK_WINDOW (main_window),
							   GTK_FILE_CHOOSER_ACTION_OPEN,
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL,
							   GTK_STOCK_OPEN,
							   GTK_RESPONSE_ACCEPT,
							   NULL);
	//widget_set_custom_font (midimap_load_dialog);
	gtk_window_set_wmclass (GTK_WINDOW (midimap_load_dialog), "phasex", "phasex-dialog");
	gtk_window_set_role (GTK_WINDOW (midimap_load_dialog), "midimap-load");

	/* realize widget before making it deal with actual file trees */
	gtk_widget_realize (midimap_load_dialog);

#if GTK_MINOR_VERSION >= 6
	/* show hidden files, if we can */
	gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (midimap_load_dialog), TRUE);
#endif

	/* add system and user midimap dirs as shortcuts */
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (midimap_load_dialog),
					      MIDIMAP_DIR, &error);
	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (midimap_load_dialog),
					      user_midimap_dir, &error);

	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}
	if (file_filter_map != NULL) {
	    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (midimap_load_dialog), file_filter_map);
	    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (midimap_load_dialog), file_filter_map);
	}
    }
}


/*****************************************************************************
 *
 * RUN_MIDIMAP_LOAD_DIALOG()
 *
 *****************************************************************************/
void 
run_midimap_load_dialog(void) {
    char		*filename;

    /* create dialog if needed */
    if (midimap_load_dialog == NULL) {
	create_midimap_load_dialog ();
    }

    /* set filename and current directory */
    if ((midimap_filename != NULL) && (strncmp (midimap_filename, user_midimap_dump_file, 10) != 0)) {
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (midimap_load_dialog),
				       midimap_filename);
    }
    else {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (midimap_load_dialog),
					     user_midimap_dir);
    }

    /* run the dialog and load if necessary */
    g_idle_remove_by_data ((gpointer)main_window);
    if (gtk_dialog_run (GTK_DIALOG (midimap_load_dialog)) == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (midimap_load_dialog));
	read_midimap (filename);
	if (setting_midimap_file != NULL) {
	    free (setting_midimap_file);
	}
	setting_midimap_file = strdup (filename);
	save_settings ();
	g_free (filename);
    }
    g_idle_add (&param_idle_update, (gpointer)main_window);

    /* save this widget for next time */
    gtk_widget_hide (midimap_load_dialog);
}


/*****************************************************************************
 *
 * CREATE_MIDIMAP_SAVE_DIALOG()
 *
 *****************************************************************************/
void
create_midimap_save_dialog(void) {
    GError		*error = NULL;

    /* create new dialog only if it doesn't already exist */
    if (midimap_save_dialog == NULL) {
	midimap_save_dialog = gtk_file_chooser_dialog_new ("PHASEX - Save Midimap",
							   GTK_WINDOW (main_window),
							   GTK_FILE_CHOOSER_ACTION_SAVE,
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL,
							   GTK_STOCK_SAVE,
							   GTK_RESPONSE_ACCEPT,
							   NULL);
	//widget_set_custom_font (midimap_save_dialog);
	gtk_window_set_wmclass (GTK_WINDOW (midimap_save_dialog), "phasex", "phasex-dialog");
	gtk_window_set_role (GTK_WINDOW (midimap_save_dialog), "midimap-save");

#if GTK_MINOR_VERSION >= 8
	/* perform overwrite confirmation, if we can */
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (midimap_save_dialog),
							TRUE);
#endif
#if GTK_MINOR_VERSION >= 6
	/* show hidden files, if we can */
	gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (midimap_save_dialog), TRUE);
#endif

	/* add user midimap dir as shortcut (users cannot write to sys anyway) */
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (midimap_save_dialog),
					      user_midimap_dir, &error);

	/* handle errors */
	if (error != NULL) {
	    if (debug) {
		fprintf (stderr, "Error %d: %s\n", error->code, error->message);
	    }
	    g_error_free (error);
	}
    }
}


/*****************************************************************************
 *
 * RUN_MIDIMAP_SAVE_AS_DIALOG()
 *
 *****************************************************************************/
void 
run_midimap_save_as_dialog(void) {
    char		*filename;

    /* create dialog if needed */
    if (midimap_save_dialog == NULL) {
	create_midimap_save_dialog ();
    }

    /* set filename and current directory */
    if ((midimap_filename != NULL) && (strcmp (midimap_filename, user_midimap_dump_file) != 0)) {
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (midimap_save_dialog),
				       midimap_filename);
    }
    else {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (midimap_save_dialog),
					     user_midimap_dir);
    }

    /* run the dialog and save if necessary */
    g_idle_remove_by_data ((gpointer)main_window);
    if (gtk_dialog_run (GTK_DIALOG (midimap_save_dialog)) == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (midimap_save_dialog));
	save_midimap (filename);
	if (setting_midimap_file != NULL) {
	    free (setting_midimap_file);
	}
	setting_midimap_file = strdup (filename);
	save_settings ();
	g_free (filename);
    }
    g_idle_add (&param_idle_update, (gpointer)main_window);

    /* save this widget for next time */
    gtk_widget_hide (midimap_save_dialog);
}


/*****************************************************************************
 *
 * ON_MIDIMAP_SAVE_ACTIVATE()
 *
 *****************************************************************************/
void 
on_midimap_save_activate(void) {
    if ((midimap_filename != NULL) && (strcmp (midimap_filename, user_midimap_dump_file) != 0)) {
	save_midimap (midimap_filename);
    }
    else {
	run_midimap_save_as_dialog ();
    }
}


/*****************************************************************************
 *
 * PHASEX_GTKUI_QUIT()
 *
 *****************************************************************************/
void 
phasex_gtkui_quit(void) {
    GtkWidget	*dialog;
    GtkWidget	*label;
    gint	response;
    int		need_quit = 1;

    /* check for modified patch, and save if desired */
    if (patch->modified) {

	/* create patch modified dialog */
	dialog = gtk_dialog_new_with_buttons ("WARNING:  Patch Modified",
					      GTK_WINDOW (main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      "Ignore Changes",
					      GTK_RESPONSE_NO,
					      "Save and Quit",
					      GTK_RESPONSE_YES,
					      NULL);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "phasex", "phasex-dialog");
	gtk_window_set_role (GTK_WINDOW (dialog), "verify-quit");

	label = gtk_label_new ("The current patch has not been saved since "
			       "it was last modified.  Save now?");
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
	//widget_set_custom_font (dialog);
	gtk_widget_show_all (dialog);

	/* run patch modified dialog */
	g_idle_remove_by_data ((gpointer)main_window);
 	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_YES:
	    on_patch_save_activate (NULL, NULL);
	    need_quit = 1;
	    break;
	case GTK_RESPONSE_NO:
	    need_quit = 1;
	    break;
	case GTK_RESPONSE_CANCEL:
	    need_quit = 0;
	    break;
	}
	gtk_widget_destroy (dialog);
	g_idle_add (&param_idle_update, (gpointer)main_window);
    }

    /* check for modified midimap, and save if desired */
    if (midimap_modified) {

	/* create midimap modified dialog */
	dialog = gtk_dialog_new_with_buttons ("WARNING:  MIDI Map Modified",
					      GTK_WINDOW (main_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      "Ignore Changes",
					      GTK_RESPONSE_NO,
					      "Save and Quit",
					      GTK_RESPONSE_YES,
					      NULL);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "phasex", "phasex-dialog");
	gtk_window_set_role (GTK_WINDOW (dialog), "verify-save");

	label = gtk_label_new ("The current MIDI map has not been saved since "
			       "it was last modified.  Save now?");
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
	//widget_set_custom_font (dialog);
	gtk_widget_show_all (dialog);

	/* run midimap modified dialog */
	g_idle_remove_by_data ((gpointer)main_window);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_YES:
	    on_midimap_save_activate ();
	    need_quit = 1;
	    break;
	case GTK_RESPONSE_NO:
	    need_quit = 1;
	    break;
	case GTK_RESPONSE_CANCEL:
	    need_quit = 0;
	    break;
	}
	gtk_widget_destroy (dialog);
	g_idle_add (&param_idle_update, (gpointer)main_window);
    }

    /* quit only if cancel wasn't pressed */
    if (need_quit) { // || forced_quit) {
	gtk_main_quit ();
    }
}


/*****************************************************************************
 *
 * PHASEX_GTKUI_FORCED_QUIT()
 *
 *****************************************************************************/
void 
phasex_gtkui_forced_quit(void) {
    forced_quit = 1;
    phasex_gtkui_quit ();
}


/*****************************************************************************
 *
 * HANDLE_WINDOW_STATE_EVENT()
 *
 *****************************************************************************/
void
handle_window_state_event(GtkWidget *widget, GdkEventWindowState *state) {
    if (state->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
	if (state->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
	    setting_maximize = MAXIMIZE_ON;
	}
	else {
	    setting_maximize = MAXIMIZE_OFF;
	}
    }
    if (state->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
	if (state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
	    setting_fullscreen = FULLSCREEN_ON;
	    if ((menu_item_fullscreen != NULL)) {
		gtk_check_menu_item_set_active (menu_item_fullscreen, TRUE);
	    }
	}
	else {
	    setting_fullscreen = FULLSCREEN_OFF;
	    if ((menu_item_fullscreen != NULL)) {
		gtk_check_menu_item_set_active (menu_item_fullscreen, FALSE);
	    }
	}
    }
    close_config_dialog ();
}


/*****************************************************************************
 *
 * Menubar / Menu Items
 *
 *****************************************************************************/
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",                 NULL,         NULL,                        0,                 "<Branch>" },
  { "/File/_Open",            "<CTRL>O",    run_patch_load_dialog,       0,                 "<Item>" },
  { "/File/_Save",            "<CTRL>S",    on_patch_save_activate,      0,                 "<Item>" },
  { "/File/Save _As",         NULL,         run_patch_save_as_dialog,    0,                 "<Item>" },
  { "/File/sep1",             NULL,         NULL,                        0,                 "<Separator>" },
  { "/File/_Preferences",     "<CTRL>P",    create_config_dialog,        0,                 "<Item>" },
  { "/File/sep2",             NULL,         NULL,                        0,                 "<Separator>" },
  { "/File/_Quit",            "<CTRL>Q",    phasex_gtkui_quit,           0,                 "<Item>" },
  { "/_View",		      NULL,         NULL,                        0,                 "<Branch>" },
  { "/_View/_Notebook",       NULL,         set_window_layout,           LAYOUT_NOTEBOOK,   "<RadioItem>" },
  { "/_View/_One Page",       NULL,         set_window_layout,           LAYOUT_ONE_PAGE,   "/View/Notebook" },
  { "/_View/sep1",            NULL,         NULL,                        0,                 "<Separator>" },
  { "/_View/_Fullscreen",     "F11",        set_fullscreen_mode,         0,                 "<CheckItem>" },
  { "/_Patch",                NULL,         NULL,                        0,                 "<Branch>" },
  { "/Patch/_Load",           "<CTRL>L",    run_patch_load_dialog,       0,                 "<Item>" },
  { "/Patch/_Save",           "<CTRL>S",    on_patch_save_activate,      0,                 "<Item>" },
  { "/Patch/Save _As",        NULL,         run_patch_save_as_dialog,    0,                 "<Item>" },
  { "/Patch/sep1",            NULL,         NULL,                        0,                 "<Separator>" },
#ifdef BANK_MEM_MODE_IN_SUBMENU
  { "/Patch/Bank _Memory Mode",           NULL, NULL,                    0,                 "<Branch>" },
  { "/Patch/Bank _Memory Mode/_Autosave", NULL, set_bank_mem_mode,       BANK_MEM_AUTOSAVE, "<RadioItem>" },
  { "/Patch/Bank _Memory Mode/_Warn",     NULL, set_bank_mem_mode,       BANK_MEM_WARN,     "/Patch/Bank Memory Mode/Autosave" },
  { "/Patch/Bank _Memory Mode/_Protect",  NULL, set_bank_mem_mode,       BANK_MEM_PROTECT,  "/Patch/Bank Memory Mode/Warn" },
#else
  { "/Patch/Bank Memory Autosa_ve", NULL, set_bank_mem_mode,       BANK_MEM_AUTOSAVE, "<RadioItem>" },
  { "/Patch/Bank Memory _Warn",     NULL, set_bank_mem_mode,       BANK_MEM_WARN,     "/Patch/Bank Memory Autosave" },
  { "/Patch/Bank Memory _Protect",  NULL, set_bank_mem_mode,       BANK_MEM_PROTECT,  "/Patch/Bank Memory Warn" },
#endif
  { "/_MIDI",                 NULL,         NULL,                        0,                 "<Branch>" },
#ifdef MIDI_CHANNEL_IN_MENU
  { "/MIDI/_Channel",         NULL,         NULL,                        0,                 "<Branch>" },
  { "/MIDI/Channel/_1",       NULL,         set_midi_channel,            0,                 "<RadioItem>" },
  { "/MIDI/Channel/_2",       NULL,         set_midi_channel,            1,                 "/MIDI/Channel/1" },
  { "/MIDI/Channel/_3",       NULL,         set_midi_channel,            2,                 "/MIDI/Channel/2" },
  { "/MIDI/Channel/_4",       NULL,         set_midi_channel,            3,                 "/MIDI/Channel/3" },
  { "/MIDI/Channel/_5",       NULL,         set_midi_channel,            4,                 "/MIDI/Channel/4" },
  { "/MIDI/Channel/_6",       NULL,         set_midi_channel,            5,                 "/MIDI/Channel/5" },
  { "/MIDI/Channel/_7",       NULL,         set_midi_channel,            6,                 "/MIDI/Channel/6" },
  { "/MIDI/Channel/_8",       NULL,         set_midi_channel,            7,                 "/MIDI/Channel/7" },
  { "/MIDI/Channel/_9",       NULL,         set_midi_channel,            8,                 "/MIDI/Channel/8" },
  { "/MIDI/Channel/_10",      NULL,         set_midi_channel,            9,                 "/MIDI/Channel/9" },
  { "/MIDI/Channel/_11",      NULL,         set_midi_channel,            10,                "/MIDI/Channel/10" },
  { "/MIDI/Channel/_12",      NULL,         set_midi_channel,            11,                "/MIDI/Channel/11" },
  { "/MIDI/Channel/_13",      NULL,         set_midi_channel,            12,                "/MIDI/Channel/12" },
  { "/MIDI/Channel/_14",      NULL,         set_midi_channel,            13,                "/MIDI/Channel/13" },
  { "/MIDI/Channel/_15",      NULL,         set_midi_channel,            14,                "/MIDI/Channel/14" },
  { "/MIDI/Channel/_16",      NULL,         set_midi_channel,            15,                "/MIDI/Channel/15" },
  { "/MIDI/Channel/_Omni",    NULL,         set_midi_channel,            16,                "/MIDI/Channel/16" },
  { "/MIDI/sep1",             NULL,         NULL,                        0,                 "<Separator>" },
#endif
  { "/MIDI/_Load MIDI Map",   NULL,         run_midimap_load_dialog,     0,                 "<Item>" },
  { "/MIDI/_Save MIDI Map",   NULL,         on_midimap_save_activate,    0,                 "<Item>" },
  { "/MIDI/Save MIDI Map _As",NULL,         run_midimap_save_as_dialog,  0,                 "<Item>" },
  { "/_JACK",                 NULL,         NULL,                        0,                 "<Branch>" },
  { "/JACK/Autoconnect",      NULL,         set_jack_autoconnect,        1,                 "<RadioItem>" },
  { "/JACK/Manual Connect",   NULL,         set_jack_autoconnect,        0,                 "/JACK/Autoconnect" },
  { "/JACK/sep1",             NULL,         NULL,                        0,                 "<Separator>" },
  { "/JACK/Reconnect",        NULL,         jack_restart,                0,                 "<Item>" },
  { "/_Help",                 NULL,         NULL,                        0,                 "<LastBranch>" },
  { "/_Help/About PHASEX",    NULL,         about_phasex_dialog,         0,                 "<Item>" },
  { "/_Help/Using PHASEX",    NULL,         display_phasex_help,         0,                 "<Item>" },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


/*****************************************************************************
 *
 * CREATE_MENUBAR()
 *
 *****************************************************************************/
void
create_menubar(GtkWidget *main_window, GtkWidget *box) {
    GtkAccelGroup	*accel_group;
    GtkItemFactory	*item_factory;
#ifdef CUSTOM_FONTS_IN_MENUS
    GtkWidget		*widget;
#endif
    int			mem_mode = setting_bank_mem_mode;

    /* create item factory with associated accel group */
    accel_group = gtk_accel_group_new ();
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
					 accel_group);

    /* build menubar and menus with the item factory */
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

    /* handle check and radio items */

    /* fullscreen checkbox */
    menu_item_fullscreen = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/View/Fullscreen"));
    gtk_check_menu_item_set_active (menu_item_fullscreen, setting_fullscreen);

    /* window layout radio group */
    menu_item_notebook = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/View/Notebook"));
    menu_item_one_page = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/View/One Page"));

    if (setting_window_layout == LAYOUT_NOTEBOOK) {
	if (!gtk_check_menu_item_get_active (menu_item_notebook)) {
	    gtk_check_menu_item_set_active (menu_item_notebook, TRUE);
	}
    }
    else {
	if (!gtk_check_menu_item_get_active (menu_item_one_page)) {
	    gtk_check_menu_item_set_active (menu_item_one_page, TRUE);
	}
    }

    /* jack connect type radio group */
    menu_item_autoconnect   = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/JACK/Autoconnect"));
    menu_item_manualconnect = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/JACK/Manual Connect"));

    if (setting_jack_autoconnect) {
	if (!gtk_check_menu_item_get_active (menu_item_autoconnect)) {
	    gtk_check_menu_item_set_active (menu_item_autoconnect, TRUE);
	}
    }
    else {
	if (!gtk_check_menu_item_get_active (menu_item_manualconnect)) {
	    gtk_check_menu_item_set_active (menu_item_manualconnect, TRUE);
	}
    }

#ifdef MIDI_CHANNEL_IN_MENU
    /* midi channel radio group */
    menu_item_midi_ch[0]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/1"));
    menu_item_midi_ch[1]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/2"));
    menu_item_midi_ch[2]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/3"));
    menu_item_midi_ch[3]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/4"));
    menu_item_midi_ch[4]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/5"));
    menu_item_midi_ch[5]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/6"));
    menu_item_midi_ch[6]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/7"));
    menu_item_midi_ch[7]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/8"));
    menu_item_midi_ch[8]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/9"));
    menu_item_midi_ch[9]  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/10"));
    menu_item_midi_ch[10] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/11"));
    menu_item_midi_ch[11] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/12"));
    menu_item_midi_ch[12] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/13"));
    menu_item_midi_ch[13] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/14"));
    menu_item_midi_ch[14] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/15"));
    menu_item_midi_ch[15] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/16"));
    menu_item_midi_ch[16] = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/MIDI/Channel/Omni"));

    if ((setting_midi_channel >= 0) && (setting_midi_channel <= 16)) {
	gtk_check_menu_item_set_active (menu_item_midi_ch[setting_midi_channel], TRUE);
    }
#endif

    /* Bank memory mode radio group */
#ifdef BANK_MEM_MODE_IN_SUBMENU
    menu_item_autosave = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Mode/Autosave"));
    menu_item_warn     = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Mode/Warn"));
    menu_item_protect  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Mode/Protect"));
#else
    menu_item_autosave = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Autosave"));
    menu_item_warn     = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Warn"));
    menu_item_protect  = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Patch/Bank Memory Protect"));
#endif

    switch (mem_mode) {
    case BANK_MEM_AUTOSAVE:
	gtk_check_menu_item_set_active (menu_item_autosave, TRUE);
	break;
    case BANK_MEM_PROTECT:
	gtk_check_menu_item_set_active (menu_item_protect, TRUE);
	break;
    case BANK_MEM_WARN:
	gtk_check_menu_item_set_active (menu_item_warn, TRUE);
	break;
    }

    /* attach menubar's accel group to main window */
    gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);

    /* add the menubar to the box passed in to this function */
    main_menubar = gtk_item_factory_get_widget (item_factory, "<main>");
    widget_set_custom_font (main_menubar);
    widget_set_backing_store (main_menubar);
    gtk_box_pack_start (GTK_BOX (box), main_menubar, FALSE, FALSE, 0);

#ifdef CUSTOM_FONTS_IN_MENUS
    /* add custom fonts and backing store to the menus */
    widget = gtk_item_factory_get_widget (item_factory, "/File");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/View");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/Patch");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/Patch/Bank Memory Mode");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/MIDI");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/MIDI/Channel");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/JACK");
    widget_set_custom_font (widget);

    widget = gtk_item_factory_get_widget (item_factory, "/Help");
    widget_set_custom_font (widget);
#endif
}


/*****************************************************************************
 *
 * CREATE_MAIN_WINDOW()
 *
 *****************************************************************************/
void
create_main_window(void) {
    GtkWidget			*main_vbox;
    GtkWidget			*event;
    char			knob_file[PATH_MAX];
    char			window_title[32];

    /* create main window */
    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* handle maximize and fullscreen modes before anything else gets a sense of geometry */
    if (setting_maximize) {
	gtk_window_set_decorated (GTK_WINDOW (main_window), TRUE);
	gtk_window_maximize (GTK_WINDOW (main_window));
    }
    if (setting_fullscreen) {
	gtk_window_set_decorated (GTK_WINDOW (main_window), FALSE);
	gtk_window_fullscreen (GTK_WINDOW (main_window));
    }
    if (!setting_maximize && !setting_fullscreen) {
	gtk_window_set_decorated (GTK_WINDOW (main_window), TRUE);
	gtk_window_unfullscreen (GTK_WINDOW (main_window));
	gtk_window_unmaximize (GTK_WINDOW (main_window));
    }

    /* set class, role, name, title, and icon */
    gtk_window_set_wmclass (GTK_WINDOW (main_window), "phasex", "phasex");
    gtk_window_set_role (GTK_WINDOW (main_window), "main");
    widget_set_backing_store (main_window);
    gtk_object_set_data (GTK_OBJECT(main_window), "main_window", main_window);
    gtk_widget_set_name (main_window, "main_window");
    if(!phasex_instance) {
        snprintf (window_title, sizeof (window_title), "%s", phasex_title);
    }
    else {
        snprintf (window_title, sizeof (window_title), "%s-%02d", phasex_title, phasex_instance);
    }
    gtk_window_set_title (GTK_WINDOW (main_window), window_title);
    gtk_window_set_icon_from_file (GTK_WINDOW (main_window),
				   PIXMAP_DIR"/phasex-icon.png", NULL);
    gtk_window_set_default_icon_from_file (PIXMAP_DIR"/phasex-icon.png", NULL);

    /* put a vbox in it */
    event = gtk_event_box_new ();
    widget_set_backing_store (event);

    main_vbox = gtk_vbox_new (FALSE, 0);
    widget_set_backing_store (main_vbox);

    gtk_container_add (GTK_CONTAINER (event), main_vbox);
    gtk_container_add (GTK_CONTAINER (main_window), event);

    /* put the menu in the vbox first */
    create_menubar (main_window, main_vbox);

    /* preload knob animation image */
    snprintf (knob_file, PATH_MAX, "%s/knob-%dx%d.png", PIXMAP_DIR, KNOB_WIDTH, KNOB_HEIGHT);
    knob_anim = gtk_knob_animation_new_from_file (knob_file);

    /* put parameter table or notebook in the vbox next */
    if (setting_window_layout == LAYOUT_ONE_PAGE) {
	create_param_one_page (main_window, main_vbox, param_page);
    }
    else {
	create_param_notebook (main_window, main_vbox, param_page);
    }

    /* window doesn't appear until now */
    gtk_widget_show_all (main_window);

    /* connect delete-event (but not destroy!) signal */
    g_signal_connect (G_OBJECT (main_window), "delete-event",
    		      G_CALLBACK (phasex_gtkui_forced_quit), NULL);

    /* connect (un)maximize events */
    g_signal_connect (G_OBJECT (main_window), "window-state-event",
    		      G_CALLBACK (handle_window_state_event),
		      (gpointer)NULL);
}


/*****************************************************************************
 *
 * RESTART_GTKUI()
 *
 *****************************************************************************/
void
restart_gtkui (void) {
/* shut down the gui thread and let the watchdog recreate it */
#ifdef RESTART_THREAD_WITH_GUI
    gtkui_restarting = 1;

    /* destroy windows */
    close_config_dialog ();
    if ((patch_load_dialog != NULL) && GTK_IS_DIALOG (patch_load_dialog)) {
	gtk_widget_destroy (patch_load_dialog);
	patch_load_dialog = NULL;
    }
    if ((patch_save_dialog != NULL) && GTK_IS_DIALOG (patch_save_dialog)) {
	gtk_widget_destroy (patch_save_dialog);
	patch_save_dialog = NULL;
    }
    if ((midimap_load_dialog != NULL) && GTK_IS_DIALOG (midimap_load_dialog)) {
	gtk_widget_destroy (midimap_load_dialog);
	midimap_load_dialog = NULL;
    }
    if ((midimap_save_dialog != NULL) && GTK_IS_DIALOG (midimap_save_dialog)) {
	gtk_widget_destroy (midimap_save_dialog);
	midimap_save_dialog = NULL;
    }
    if ((main_window != NULL) && GTK_IS_WINDOW (main_window)) {
	gtk_widget_hide (main_window);
	gtk_widget_destroy (main_window);
	main_window = NULL;
    }

    /* last step to shutting down gtkui */
    gtk_main_quit ();

    pthread_exit (NULL);

/* shut down gtk_main(), re-init, and rebuild everything */
#else
    /* shutdown idle handler that expects widgets to cooperate */
    g_idle_remove_by_data ((gpointer)main_window);

    /* destroy windows */
    close_config_dialog ();
    if ((patch_load_dialog != NULL) && GTK_IS_DIALOG (patch_load_dialog)) {
	gtk_widget_destroy (patch_load_dialog);
	patch_load_dialog = NULL;
    }
    if ((patch_save_dialog != NULL) && GTK_IS_DIALOG (patch_save_dialog)) {
	gtk_widget_destroy (patch_save_dialog);
	patch_save_dialog = NULL;
    }
    if ((midimap_load_dialog != NULL) && GTK_IS_DIALOG (midimap_load_dialog)) {
	gtk_widget_destroy (midimap_load_dialog);
	midimap_load_dialog = NULL;
    }
    if ((midimap_save_dialog != NULL) && GTK_IS_DIALOG (midimap_save_dialog)) {
	gtk_widget_destroy (midimap_save_dialog);
	midimap_save_dialog = NULL;
    }
    if ((main_window != NULL) && GTK_IS_WINDOW (main_window)) {
	gtk_widget_hide (main_window);
	gtk_widget_destroy (main_window);
	main_window = NULL;
    }

    /* last step to shutting down gtkui */
    gtk_main_quit ();

    /* re-initialize GTK */
    set_theme_env ();
    gtk_init (NULL, NULL);

    /* recreate windows */
    create_main_window ();
    create_patch_load_dialog ();
    create_patch_save_dialog ();
    create_midimap_load_dialog ();
    create_midimap_save_dialog ();

    /* add idle handler to update widgets */
    g_idle_add (&param_idle_update, (gpointer)main_window);

    /* back in business */
    gtk_main ();
#endif
}


/*****************************************************************************
 *
 * WIDGET_SET_BACKING_STORE()
 *
 * enable backing store on the selected widget (if possible)
 *
 *****************************************************************************/
void
widget_set_backing_store (GtkWidget *widget) {
#ifdef GDK_WINDOWING_X11
    if (setting_backing_store) {
	g_signal_connect (widget, "realize",
			  G_CALLBACK (widget_set_backing_store_callback),
			  NULL);
    }
#endif
}


/*****************************************************************************
 *
 * WIDGET_SET_BACKING_STORE_CALLBACK()
 *
 * callback to enable backing store on the selected widget (if possible)
 * connected to a widget's realize signal by widget_set_backing_store()
 *
 *****************************************************************************/
#ifdef GDK_WINDOWING_X11
void
widget_set_backing_store_callback(GtkWidget *widget, void *data) {
    GList			*list;
    GList			*cur;
    GdkWindow			*window		= NULL;
#if GTK_MINOR_VERSION > 14
    GtkWindowGroup		*group;
#endif
    XSetWindowAttributes	attributes;
    unsigned long		attr_mask	= (CWBackingStore);


    if (widget->window == NULL) {
	return;
    }

    window = widget->window;

    attributes.backing_store  = Always;
    XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
			     GDK_WINDOW_XWINDOW (window),
			     attr_mask, &attributes);

    /* special case for the spin button which has a giganormitude of windows */
    if (GTK_IS_SPIN_BUTTON (widget)) {
        window = GTK_SPIN_BUTTON (widget)->panel;
	if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}

        window = GTK_ENTRY (widget)->text_area;
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
    }
    /* special case for label mnemonic window and selection window */
    else if (GTK_IS_LABEL (widget)) {
        window = GDK_WINDOW (GTK_LABEL (widget)->mnemonic_window);
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}

        window = GDK_WINDOW (GTK_LABEL (widget)->select_info);
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
    }
    /* special case for entry text area window */
    else if (GTK_IS_ENTRY (widget)) {
        window = GTK_ENTRY (widget)->text_area;
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
    }
    /* special case for gtk event box */
    else if (GTK_IS_EVENT_BOX (widget)) {
	window = GTK_WIDGET (gtk_bin_get_child (GTK_BIN (GTK_WIDGET (&(GTK_EVENT_BOX (widget))->bin))))->window;
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
    }
    /* special case for notebook event window */
    else if (GTK_IS_NOTEBOOK (widget)) {
	window = GTK_NOTEBOOK (widget)->event_window;
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
    }
    /* special case for normal gtk window */
    else if (GTK_IS_WINDOW (widget)) {
        window = GTK_WINDOW (widget)->frame;
        if (window != NULL) {
            XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
				     GDK_WINDOW_XWINDOW (window),
				     attr_mask, &attributes);
	}
#if GTK_MINOR_VERSION >= 14
	group = gtk_window_get_group (GTK_WINDOW (widget));
	list = gtk_window_group_list_windows (group);
	cur = g_list_first (list);
	while (cur != NULL) {
	    if (GTK_IS_WIDGET (cur->data)) {
		window = GTK_WIDGET (cur->data)->window;
		if (window != NULL) {
		    XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
					     GDK_WINDOW_XWINDOW (window),
					     attr_mask, &attributes);
		}
	    }
	    cur = g_list_next (cur);
	}
#endif
    }
}
#endif


/*****************************************************************************
 *
 * WIDGET_SET_CUSTOM_FONT()
 *
 * override widget's font with a custom font (from prefs setting)
 *
 * *NOTE: this function recurses over containers
 *
 *****************************************************************************/
void 
widget_set_custom_font (GtkWidget *widget) {
    GList	*list;
    GList	*cur;

    if (phasex_font_desc != NULL) {
	/* modify label font directly */
	if (GTK_IS_LABEL (widget)) {
	    gtk_widget_modify_font (GTK_WIDGET (widget), phasex_font_desc);
	}
	/* modify font of label widget inside button */
	else if (GTK_IS_BUTTON (widget)) {
	    list = gtk_container_get_children (GTK_CONTAINER (widget));
	    cur = g_list_first (list);
	    while (cur != NULL) {
		if (GTK_IS_LABEL (cur->data)) {
		    gtk_widget_modify_font (GTK_WIDGET (cur->data), phasex_font_desc);
		}
		cur = g_list_next (cur);
	    }
	}
	/* recurse through menubar and menushell */
	else if (GTK_IS_MENU_BAR (widget) || GTK_IS_MENU_SHELL (widget)) {
	    list = GTK_MENU_SHELL (widget)->children;
	    cur = g_list_first (list);
	    while (cur != NULL) {
		widget_set_custom_font (GTK_WIDGET (cur->data));
		cur = g_list_next (cur);
	    }
	}
	/* recurse to bin's child */
	else if (GTK_IS_BIN (widget)) {
	    widget_set_custom_font (gtk_bin_get_child (GTK_BIN (widget)));
	}
	/* recurse through children */
	else if (GTK_IS_CONTAINER (widget)) {
	    list = gtk_container_get_children (GTK_CONTAINER (widget));
	    cur = g_list_first (list);
	    while (cur != NULL) {
		if (GTK_IS_LABEL (cur->data)) {
		    gtk_widget_modify_font (GTK_WIDGET (cur->data), phasex_font_desc);
		}
		else {
		    widget_set_custom_font (GTK_WIDGET (cur->data));
		}
		cur = g_list_next (cur);
	    }
	}
	/* recurse through event box's bin and child */
	else if (GTK_IS_EVENT_BOX (widget)) {
	    widget = gtk_bin_get_child (GTK_BIN (GTK_WIDGET (&(GTK_EVENT_BOX (widget))->bin)));
	    if (GTK_IS_WIDGET (widget) || GTK_IS_CONTAINER (widget)) {
		widget_set_custom_font (GTK_WIDGET (widget));
	    }
	}
	/* modify fonts of any other widget directly */
	else if (GTK_IS_WIDGET (widget)) {
	    gtk_widget_modify_font (widget, phasex_font_desc);
	}
    }
}
