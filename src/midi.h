/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2003 (node@users.sourceforge.net)
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ac_config.h"

#ifdef HAVE_ALSA

#ifndef __JR_MIDI_H__
#define __JR_MIDI_H__

#include <glib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "lock_free_fifo.h"
#include "midi_control.h"

typedef struct _midi_info midi_info_t;

struct _ui;

struct _midi_info
{
  pthread_t     thread;
  gboolean      quit;
  
  struct _ui    *ui;
  
  snd_seq_t     *seq;
  int           seq_nfds;
  struct pollfd *pfds;
  
  lff_t         *midi_to_ui;
  lff_t         *ui_to_midi;

  GSList        *midi_controls;
};

struct _ui;

midi_info_t * midi_info_new     (struct _ui * ui);
void          midi_info_destroy (midi_info_t * minfo);

#endif /* __JR_MIDI_H__ */

#endif /* HAVE_ALSA */


