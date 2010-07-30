/*****************************************************************************
 *
 * settings.h
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
#ifndef _PHASEX_SETTINGS_H_
#define _PHASEX_SETTINGS_H_

#include <gtk/gtk.h>
#include "phasex.h"


#define MAXIMIZE_OFF		0
#define MAXIMIZE_ON		1

#define FULLSCREEN_OFF		0
#define FULLSCREEN_ON		1

#define LAYOUT_NOTEBOOK		0
#define LAYOUT_ONE_PAGE		1

#define PHASEX_THEME_DARK	0
#define PHASEX_THEME_LIGHT	1
#define PHASEX_THEME_SYSTEM	2
#define PHASEX_THEME_CUSTOM	3


extern int			setting_window_layout;
extern int			setting_fullscreen;
extern int			setting_maximize;
extern int			setting_backing_store;
extern int			setting_jack_autoconnect;
extern int			setting_sample_rate_mode;
extern int			setting_bank_mem_mode;
extern int			setting_gui_idle_sleep;
extern int			setting_midi_priority;
extern int			setting_engine_priority;
extern int			setting_sched_policy;
extern int			setting_polyphony;
extern int			setting_theme;
extern int			setting_midi_channel;

extern double			setting_tuning_freq;

extern char			*setting_midimap_file;
extern char			*setting_custom_theme;
extern char			*setting_font;

extern char			*sample_rate_mode_names[];
extern char			*bank_mem_mode_names[];
extern char			*layout_names[];
extern char			*theme_names[];


extern PangoFontDescription	*phasex_font_desc;


void read_settings(void);
void save_settings(void);
void set_midi_channel(GtkWidget *widget, gpointer data, GtkWidget *widget2);
void set_fullscreen_mode(GtkWidget *widget, gpointer data1, gpointer data2);
void set_maximize_mode(GtkWidget *widget, gpointer data1);
void set_window_layout(GtkWidget *widget, gpointer data, GtkWidget *widget2);
void set_backing_store(GtkWidget *widget, gpointer data);
void set_gui_idle_sleep(GtkWidget *widget, gpointer data);
void set_midi_priority(GtkWidget *widget, gpointer data);
void set_engine_priority(GtkWidget *widget, gpointer data);
void set_sched_policy(GtkWidget *widget, gpointer data);
void set_jack_autoconnect(GtkWidget *widget, gpointer data, GtkWidget *widget2);
void set_tuning_freq(GtkWidget *widget, gpointer data);
void set_sample_rate_mode(GtkWidget *widget, gpointer data);
void set_bank_mem_mode(GtkWidget *widget, gpointer data, GtkWidget *widget2);
void set_polyphony(GtkWidget *widget, gpointer data);
void set_theme(GtkWidget *widget, gpointer data);
void set_custom_theme(GtkWidget *widget, gpointer data);
void set_theme_env();
void set_font(GtkFontButton *button, GtkWidget *widget);
void close_config_dialog(void);
void create_config_dialog(void);


#endif /* _PHASEX_SETTINGS_H_ */
