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

#ifndef __JR_UI_H__
#define __JR_UI_H__

#include "ac_config.h"

#include <gtk/gtk.h>

#include "jack_rack.h"
#include "plugin_mgr.h"

typedef struct _ui ui_t;

enum _ui_state
{
  /* nothing's happening */
  STATE_NORMAL,
  
  /* the gui is waiting for the process callback to do something */
  STATE_RACK_CHANGE
};
typedef enum _ui_state ui_state_t;

struct _ui
{
  char *            jack_client_name;
  plugin_mgr_t *    plugin_mgr;
  process_info_t *  procinfo;
  jack_rack_t *     jack_rack;

  lff_t *           ui_to_process;
  lff_t *           process_to_ui;

  char *            filename;
  
  gboolean          shutdown;
  ui_state_t        state;

  GtkWidget *       main_window;
  GtkWidget *       plugin_box;
  GtkWidget *       add;
  GtkWidget *       add_menuitem;
  GtkWidget *       add_menu;
  GtkWidget *       channel_spin;
#ifdef HAVE_LADCCA
  GtkWidget *       cca_save;
  GtkWidget *       cca_save_menu_item;
#endif
  GtkWidget *       splash_screen;
  GtkWidget *       splash_screen_text;
  
};

ui_t * ui_new     (const char * jack_client_name, unsigned long channels);
void   ui_destroy (ui_t * ui);

void ui_set_filename (ui_t * ui, const char * filename);
void ui_set_state    (ui_t * ui, ui_state_t state);
void ui_set_channels (ui_t * ui, unsigned long channels);

ui_state_t ui_get_state (ui_t * ui);

plugin_t * ui_instantiate_plugin (ui_t * ui, plugin_desc_t * desc);

void ui_display_error       (ui_t * ui, const char * format, ...);
void ui_display_splash_text (ui_t * ui, const char * format, ...);
gboolean ui_get_ok (ui_t * ui, const char * format, ...);


#endif /* __JR_UI_H__ */
