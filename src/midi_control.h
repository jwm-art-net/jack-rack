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

#ifndef __JR_MIDI_CONTROL_H__
#define __JR_MIDI_CONTROL_H__

#include <glib.h>
#include <ladspa.h>
#include <pthread.h>

#include "lock_free_fifo.h"

typedef struct _midi_control midi_control_t;

struct _plugin_slot;

struct _midi_control
{
  unsigned char       midi_channel;
  pthread_mutex_t     midi_channel_lock;
  unsigned int        midi_param;
  pthread_mutex_t     midi_param_lock;
  
  gboolean            locked;
  pthread_mutex_t     locked_lock;
  
  LADSPA_Data         min;
  LADSPA_Data         max;
  
  lff_t               *fifo;
  lff_t               *fifos;

  struct _plugin_slot *plugin_slot;
  gboolean            ladspa_control;
  union
  {
    struct
    {
      unsigned long       control;
      guint               copy;
    } ladspa;
    
    struct
    {
      unsigned long       channel;
    } wet_dry;
  } control;

};

midi_control_t *ladspa_midi_control_new (struct _plugin_slot * plugin_slot,
                                         guint copy, unsigned long control);
midi_control_t *wet_dry_midi_control_new (struct _plugin_slot * plugin_slot,
                                          unsigned long channel);
void            midi_control_destroy (midi_control_t * midi_ctrl);

void midi_control_set_midi_channel (midi_control_t * midi_ctrl, unsigned char channel);
void midi_control_set_midi_param   (midi_control_t * midi_ctrl, unsigned int param);
void midi_control_set_locked       (midi_control_t * midi_ctrl, gboolean locked);

unsigned char midi_control_get_midi_channel    (midi_control_t * midi_ctrl);
unsigned int  midi_control_get_midi_param      (midi_control_t * midi_ctrl);
unsigned long midi_control_get_wet_dry_channel (midi_control_t * midi_ctrl);
unsigned long midi_control_get_ladspa_control  (midi_control_t * midi_ctrl);
guint         midi_control_get_ladspa_copy     (midi_control_t * midi_ctrl);
gboolean      midi_control_get_locked          (midi_control_t * midi_ctrl);

const char *  midi_control_get_control_name    (midi_control_t * midi_ctrl);

#endif /* __JR_MIDI_CONTROL_H__ */

#endif /* HAVE_ALSA */


