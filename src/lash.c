/*****************************************************************************
 *
 * lash.c
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
 
#include "lash.h"

int lash_clinit(int argc, char** argv, jack_client_t *jack_client, snd_seq_t *alsa_handle)
{
    lash_jackname = jack_get_client_name(jack_client);
    lash_client = lash_init(lash_extract_args(&argc, &argv), lash_jackname, LASH_Config_File, LASH_PROTOCOL(2, 0));
    if (lash_enabled(lash_client)) 
    {
        lash_jack_client_name(lash_client, lash_jackname);
        lash_event_t *event = lash_event_new_with_type(LASH_Client_Name);
        lash_event_set_string(event, lash_jackname);
        lash_send_event(lash_client, event);
        lash_alsaid = alsa_handle;
        lash_alsa_client_id(lash_client, (unsigned char)snd_seq_client_id(alsa_handle));
        return 0;
    }
    return -1;
}

int lash_pollevent()
{
    if (!lash_enabled(lash_client))
        return -1;

    lash_event_t *event;
    char *l_path;
    while (event = lash_get_event(lash_client)) {
        if (lash_event_get_type(event) == LASH_Save_File) {
            lash_buffer = (char *)lash_event_get_string(event);
            l_path = (char *)malloc(strlen(lash_buffer) + 13);
            sprintf(l_path, "%s/phasex.phx", lash_buffer);
            save_patch(l_path, 0);
            sprintf(l_path, "%s/phasex.map", lash_buffer);
            save_midimap(l_path);
            lash_send_event(lash_client, lash_event_new_with_type(LASH_Save_File));
            free(l_path);
            break;
        }
        else if (lash_event_get_type(event) == LASH_Restore_File) {
            lash_buffer = (char *)lash_event_get_string(event);
            l_path = (char *)malloc(strlen(lash_buffer) + 13);
            sprintf(l_path, "%s/phasex.phx", lash_buffer);
            read_patch(l_path, 0);
            sprintf(l_path, "%s/phasex.map", lash_buffer);
            read_midimap(l_path);
            lash_send_event(lash_client, lash_event_new_with_type(LASH_Restore_File));
            free(l_path);
            break;
        }
        else if (lash_event_get_type(event) == LASH_Quit) {
            shutdown = 1;
            break;
        }
        lash_event_destroy(event);
    }
    return 0;
}
