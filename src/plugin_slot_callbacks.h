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

#ifndef __JR_PLUGIN_SLOT_CALLBACKS_H__
#define __JR_PLUGIN_SLOT_CALLBACKS_H__

#include "ac_config.h"

#include <gtk/gtk.h>

void     slot_change_cb          (GtkMenuItem * menuitem, gpointer user_data);
void     slot_move_cb            (GtkButton * button, gpointer user_data);
void     slot_remove_cb          (GtkButton * button, gpointer user_data);
gboolean slot_ablise_cb          (GtkWidget * button, GdkEventButton *event, gpointer user_data);
void     slot_lock_all_cb        (GtkToggleButton * button, gpointer user_data);
gboolean slot_wet_dry_cb         (GtkWidget * button, GdkEventButton *event, gpointer user_data);
void     slot_wet_dry_lock_cb    (GtkToggleButton * button, gpointer user_data);
void     slot_wet_dry_control_cb (GtkRange * range, gpointer user_data);


#endif /* __JR_PLUGIN_SLOT_CALLBACKS_H__ */
