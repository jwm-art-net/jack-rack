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

#ifndef __JR_PORT_CONTROLS_H__
#define __JR_PORT_CONTROLS_H__

#include <gtk/gtk.h>
#include <ladspa.h>

typedef struct _controls controls_t;
typedef struct _port_controls port_controls_t;

enum _control_type
{
  JR_CTRL_FLOAT,
  /* JR_CTRL_LOG, */
  JR_CTRL_INT,
  JR_CTRL_BOOL
};

typedef enum _control_type control_type_t;


struct _controls
{
  GtkWidget    *control;
  GtkWidget    *text;
  lff_t        *control_fifo;
};

/** the widgets for controlling an ladspa control port.  this is what is
    passed to the callbacks for the widgets, allowing them to send data
    down the fifo, and update the values in the different widgets */
struct _port_controls
{
  control_type_t type;
  gboolean       logarithmic;
  controls_t *   controls;

  /* single copy only stuff */
  GtkWidget *    lock;
  gboolean       locked;
  gint           lock_copy;
  
  struct _plugin_slot * plugin_slot;

  unsigned long  port_index;
  unsigned long  control_index;
};

port_controls_t * port_controls_new     (struct _plugin_slot * plugin_slot);

void port_control_set_locked (port_controls_t *port_controls, gboolean locked);
/*void port_control_set_value  (port_controls_t *port_controls, guint copy,
                              LADSPA_Data value, gboolean block_fifo); */

void gtk_widget_block_signal(GtkWidget *widget, const char *signal, GCallback callback);
void gtk_widget_unblock_signal(GtkWidget *widget, const char *signal, GCallback callback);
void port_controls_block_float_callback (port_controls_t * port_controls, guint copy);
void port_controls_block_int_callback (port_controls_t * port_controls, guint copy);
void port_controls_block_bool_callback (port_controls_t * port_controls, guint copy);
void port_controls_unblock_float_callback (port_controls_t * port_controls, guint copy);
void port_controls_unblock_int_callback (port_controls_t * port_controls, guint copy);
void port_controls_unblock_bool_callback (port_controls_t * port_controls, guint copy);

#endif /* __JR_PORT_CONTROLS_H__ */
