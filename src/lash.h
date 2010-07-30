/*****************************************************************************
 *
 * lash.h
 *
 * PHASEX:  [P]hase [H]armonic [A]dvanced [S]ynthesis [EX]periment
 *
 * Copyright (C) 2010 Anton Kormakov <assault64@gmail.com>
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
#ifndef _PHASEX_LASH_H_
#define _PHASEX_LASH_H_
 
#include <jack/jack.h>
#include <lash/lash.h>
#include <asoundlib.h>
#include "phasex.h"
#include "patch.h"
#include "param.h"
 
lash_client_t *lash_client;
char *lash_buffer;

char *lash_jackname;
snd_seq_t *lash_alsaid;

int lash_clinit(int argc, char** argv, jack_client_t *jack_client, snd_seq_t *alsa_handle);
int lash_pollevent();

#endif /* _PHASEX_LASH_H_ */
