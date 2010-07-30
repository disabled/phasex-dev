/*****************************************************************************
 *
 * gtkui.h
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
#ifndef _PHASEX_GTKUI_H_
#define _PHASEX_GTKUI_H_

#include <pthread.h>
#include <gtk/gtk.h>
#ifndef _PHASEX_PARAM_H_
#include "param.h"
#endif


extern pthread_mutex_t	gtkui_ready_mutex;
extern pthread_cond_t	gtkui_ready_cond;

extern int		gtkui_ready;

extern GtkFileFilter	*file_filter_all;
extern GtkFileFilter	*file_filter_patches;
extern GtkFileFilter	*file_filter_mod;

extern GtkWidget	*main_window;
extern GtkWidget	*main_menubar;
extern GtkWidget	*midimap_load_dialog;
extern GtkWidget	*midimap_save_dialog;
extern GtkWidget	*cc_edit_dialog;
extern GtkWidget	*cc_edit_spin;

extern GtkObject	*cc_edit_adj;

extern GtkKnobAnim	*knob_anim;

extern GtkCheckMenuItem	*menu_item_fullscreen;
extern GtkCheckMenuItem	*menu_item_notebook;
extern GtkCheckMenuItem	*menu_item_one_page;
extern GtkCheckMenuItem	*menu_item_autoconnect;
extern GtkCheckMenuItem	*menu_item_manualconnect;
extern GtkCheckMenuItem	*menu_item_autosave;
extern GtkCheckMenuItem	*menu_item_warn;
extern GtkCheckMenuItem	*menu_item_protect;

extern GtkCheckMenuItem	*menu_item_midi_ch[18];

extern int		cc_edit_active;
extern int		cc_edit_ignore_midi;
extern int		cc_edit_cc_num;
extern int		cc_edit_param_id;

extern int		forced_quit;
extern int		gtkui_restarting;


void * gtkui_thread(void *arg);
void start_gtkui_thread(void);

void create_file_filters(void);

gboolean param_idle_update(gpointer data);
void update_param_cc_num(GtkWidget *widget, gpointer data);
void update_param_locked(GtkWidget *widget, gpointer data);
void update_param_ignore(GtkWidget *widget, gpointer data);
void close_cc_edit_dialog(GtkWidget *widget, gpointer data);

void param_label_button_press(GtkWidget *widget, GdkEventButton *event);
void detent_label_button_press(gpointer data1, gpointer data2);

void create_param_input(GtkWidget *main_window, GtkWidget *table,
			gint col, gint row, PARAM *param);
void create_param_group(GtkWidget *main_window, GtkWidget *parent_table,
			PARAM_GROUP *param_group, int x, int y0, int y1);
void create_param_page(GtkWidget *main_window, GtkWidget *notebook,
		       PARAM_PAGE *param_page, int page_num);
void create_param_notebook(GtkWidget *main_window, GtkWidget *box,
			   PARAM_PAGE *param_page);
void create_param_widescreen(GtkWidget *main_window, GtkWidget *box,
			     PARAM_PAGE *param_page);

void create_midimap_load_dialog(void);
void run_midimap_load_dialog(void);
void create_midimap_save_dialog(void);
void run_midimap_save_as_dialog(void);
void on_midimap_save_activate(void);

void phasex_gtkui_quit(void);
void phasex_gtkui_force_quit(void);
void handle_window_state_event(GtkWidget *widget, GdkEventWindowState *state);

void create_menubar(GtkWidget *main_window, GtkWidget *box);

void create_main_window(void);

void restart_gtkui(void);

void widget_set_backing_store (GtkWidget *widget);
void widget_set_backing_store_callback (GtkWidget *widget, void *data);

void widget_set_custom_font (GtkWidget *widget);


#endif /* _PHASEX_GTKUI_H_ */
