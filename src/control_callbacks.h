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

#ifndef __JR_CONTROL_CALLBACKS_H__
#define __JR_CONTROL_CALLBACKS_H__

#include "ac_config.h"

#include <gtk/gtk.h>

gboolean control_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data);
void     control_lock_cb         (GtkToggleButton * button, gpointer user_data);
void     control_float_cb        (GtkRange * range, gpointer user_data);
void     control_float_text_cb   (GtkEntry * entry, gpointer user_data);
void     control_bool_cb         (GtkToggleButton * button, gpointer user_data);
void     control_int_cb          (GtkSpinButton * spinbutton, gpointer user_data);
void     control_points_cb       (GtkComboBox * combo, gpointer user_data);

#ifdef HAVE_ALSA
gboolean wet_dry_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data);
void     control_add_midi_cb     (GtkMenuItem * menuitem, gpointer user_data);
#endif

#endif /* __JR_CONTROL_CALLBACKS_H__ */
