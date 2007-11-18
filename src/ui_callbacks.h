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

#ifndef __JR_UI_CALLBACKS_H__
#define __JR_UI_CALLBACKS_H__

#include "ac_config.h"

#include <gtk/gtk.h>

void     add_cb            (GtkMenuItem * menuitem, gpointer user_data);
void     channel_cb        (GtkWidget * button, gpointer user_data);
void     new_cb            (GtkWidget * button, gpointer user_data);
void     quit_cb           (GtkButton * button, gpointer user_data);
gboolean window_destroy_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data);                                                  

#ifdef HAVE_XML
void     open_cb           (GtkButton * button, gpointer user_data);
void     save_cb           (GtkButton * button, gpointer user_data);
void     save_as_cb        (GtkButton * button, gpointer user_data);
#endif

#ifdef HAVE_LASH
void     lash_save_cb       (GtkButton * button, gpointer user_data);
#endif

#ifdef HAVE_GNOME
void     about_cb          (GtkWidget * widget, gpointer user_data);
#endif

#ifdef HAVE_ALSA
void     midi_cb           (GtkWidget * button, gpointer user_data);
#endif

gint plugin_button_cb (GtkWidget *widget, GdkEvent *event);
void plugin_add_button_cb (GtkWidget *widget, gpointer user_data);

gboolean idle_cb (gpointer data);

void jack_shutdown_cb (void * data);

#endif /* __JR_UI_CALLBACKS_H__ */
