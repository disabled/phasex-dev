/*****************************************************************************
 *
 * help.h
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
#ifndef _PHASEX_HELP_H_
#define _PHASEX_HELP_H_

#include <gtk/gtk.h>
#include "param.h"


typedef struct param_help {
    char	*label;		/* Full parameter name		*/
    char	*text;		/* Parameter help text		*/
} PARAM_HELP;


extern PARAM_HELP	param_help[NUM_HELP_PARAMS];

extern GtkWidget	*param_help_dialog;
extern GtkWidget	*about_dialog;

extern int		param_help_param_id;


void about_phasex_dialog(void);
void init_help(void);
void display_param_help(int param_id);
void display_phasex_help(void);


#endif /* _PHASEX_HELP_H_ */
