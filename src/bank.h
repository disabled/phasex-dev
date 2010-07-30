/*****************************************************************************
 *
 * bank.h
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
#ifndef _PHASEX_BANK_H_
#define _PHASEX_BANK_H_

#include <gtk/gtk.h>
#include "patch.h"


#define BANK_MEM_AUTOSAVE	0
#define BANK_MEM_WARN		1
#define BANK_MEM_PROTECT	2

#define PATCH_BANK_SIZE		1024


extern PATCH		bank[PATCH_BANK_SIZE];

extern int		program_number;
extern int		program_change_request;

extern GtkObject	*program_adj;
extern GtkWidget	*program_spin;
extern GtkWidget	*program_entry;

extern GtkWidget	*midi_channel_label;
extern GtkObject	*midi_channel_adj;

extern GtkWidget	*bank_autosave_button;
extern GtkWidget	*bank_warn_button;
extern GtkWidget	*bank_protect_button;

extern GtkWidget	*modified_label;

extern GtkWidget	*patch_load_dialog;
extern GtkWidget	*patch_save_dialog;

extern GtkWidget	*patch_load_start_spin;

extern int		patch_load_start;

extern int		show_modified;


void init_patch_bank(void);
void load_bank(void);
void save_bank(void);

int find_patch(char *name);

char *get_patch_name_from_filename(char *filename);
char *get_patch_filename_from_entry(GtkEntry *entry);

int check_patch_overwrite(char *filename);

void select_program(GtkWidget *widget, gpointer data);
void save_program(GtkWidget *widget, gpointer data);
void load_program(GtkWidget *widget, gpointer data);

void bank_autosave_activate(GtkWidget *widget, gpointer data);
void bank_warn_activate(GtkWidget *widget, gpointer data);
void bank_protect_activate(GtkWidget *widget, gpointer data);
void midi_label_button_press(gpointer data1, gpointer data2);

void create_bank_group(GtkWidget *main_window, GtkWidget *parent_table,
		       int x0, int x1, int y0, int y1);

void set_patch_load_start(GtkWidget *widget, gpointer data);
void create_patch_load_dialog(void);
void run_patch_load_dialog(GtkWidget *widget, gpointer data);
void create_patch_save_dialog(void);
void run_patch_save_as_dialog(GtkWidget *widget, gpointer data);
void on_patch_save_activate(GtkWidget *widget, gpointer data);


#endif /* _PHASEX_BANK_H_ */
