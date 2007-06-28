/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
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

#ifndef __JR_MIDI_WINDOW_H__
#define __JR_MIDI_WINDOW_H__

#include "ac_config.h"

#ifdef HAVE_ALSA

#include <gtk/gtk.h>

#include "midi_control.h"

typedef struct _midi_table_row midi_table_row_t;
typedef struct _midi_window    midi_window_t;

enum _midi_window_columns
{
  PLUGIN_COLUMN,
  CONTROL_COLUMN,
  INDEX_COLUMN,
  CHANNEL_COLUMN,
  PARAM_COLUMN,
  MINVALUE_COLUMN,
  MAXVALUE_COLUMN,
  MIDI_CONTROL_POINTER,
  N_COLUMNS
};


struct _ui;

struct _midi_window
{
  struct _ui   *ui;
  
  GtkWidget    *window;
  GtkWidget    *main_box;
  GtkListStore *controls;
  GtkWidget    *controls_view;
};

midi_window_t *midi_window_new     (struct _ui *ui);
void           midi_window_destroy (midi_window_t * midi_window);

void midi_window_add_control (midi_window_t *mwin, midi_control_t *midi_ctrl);
void midi_window_remove_control (midi_window_t *mwin, midi_control_t *midi_ctrl);

#endif /* HAVE_ALSA */

#endif /* __JR_MIDI_WINDOW_H__ */
