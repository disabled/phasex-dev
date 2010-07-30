/*****************************************************************************
 *
 * filter.h
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
#ifndef _PHASEX_FILTER_H_
#define _PHASEX_FILTER_H_

#include "phasex.h"


#define FILTER_TYPE_DIST	0
#define FILTER_TYPE_RETRO	1

#define NUM_FILTER_TYPES	2


#define FILTER_MODE_LP		0
#define FILTER_MODE_HP		1
#define FILTER_MODE_BP		2
#define FILTER_MODE_BS		3
#define FILTER_MODE_LP_PLUS	4
#define FILTER_MODE_HP_PLUS	5
#define FILTER_MODE_BP_PLUS	6
#define FILTER_MODE_BS_PLUS	7

#define NUM_FILTER_MODES	8


extern sample_t	filter_res[NUM_FILTER_TYPES][128];
extern sample_t	filter_table[TUNING_RESOLUTION * 648];
extern sample_t	filter_dist_1[16];
extern sample_t	filter_dist_2[16];
extern int	filter_limit;


void build_filter_tables(void);
void filter_wave_table(int wave_num, int num_cycles, double octaves);


#endif /* _PHASEX_FILTER_H_ */
