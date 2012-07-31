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

#include <jack/jack.h>
#ifdef HAVE_JACK_SESSION
#include <jack/session.h>
#endif

#include "jack_rack.h"
#include "plugin_mgr.h"
#include "midi.h"
#include "midi_window.h"

typedef struct _ui ui_t;

enum _ui_state
{
  /* nothing's happening */
  STATE_NORMAL,
  
  /* the gui is waiting for the process callback to do something */
  STATE_RACK_CHANGE,
  
  /* we're closing down */
  STATE_QUITTING
};
typedef enum _ui_state ui_state_t;

struct _ui
{
  plugin_mgr_t *    plugin_mgr;
  process_info_t *  procinfo;
  jack_rack_t *     jack_rack;
#ifdef HAVE_ALSA
  midi_info_t       *midi_info;
  midi_window_t     *midi_window;
#endif

  lff_t *           ui_to_process;
  lff_t *           process_to_ui;
  
  lff_t             *ui_to_midi;
  lff_t             *midi_to_ui;

  char *            filename;
  
  gboolean          shutdown;
  ui_state_t        state;

  GtkWidget *       main_window;
  GtkWidget *       plugin_box;
  GtkWidget *       add_menu;
  GtkWidget *       toolbar_add_menu;

/* all the widgets that can be desensitized */
  GtkToolItem *     add;
  GtkWidget *       add_menuitem;
  GtkWidget *       channels_menuitem;
  GtkToolItem *     channels;
#ifdef HAVE_ALSA
  GtkWidget         *midi_menuitem;
#endif
  GtkWidget *       new_menuitem;
  GtkToolItem *     new;
  GtkWidget *       open_menuitem;
  GtkToolItem *     open;

#ifdef HAVE_LASH
  GtkToolItem *     lash_save;
  GtkWidget *       lash_save_menu_item;
#endif

  GtkWidget *       splash_screen;
  GtkWidget *       splash_screen_text;
  
#ifdef HAVE_ALSA
  GtkWidget         *midi_menu;
  GtkWidget         *midi_menu_item;
#endif
#ifdef HAVE_JACK_SESSION
  jack_session_event_t *js_event;
#endif
};

ui_t * ui_new     (unsigned long channels);
void   ui_destroy (ui_t* ui);

void ui_set_filename (ui_t* ui, const char* filename);
void ui_set_state    (ui_t* ui, ui_state_t state);
void ui_set_channels (ui_t* ui, unsigned long channels);

ui_state_t ui_get_state (ui_t* ui);

plugin_t * ui_instantiate_plugin (ui_t* ui, plugin_desc_t* desc);

void     ui_display_error       (void* data, error_severity_t severity, const char* format, ...);
void     ui_display_splash_text (void* data, const char* format, ...);
gboolean ui_get_ok              (ui_t* ui, const char* format, ...);

int ui_save_file (ui_t * ui, const char * filename);
int ui_open_file (ui_t * ui, const char * filename);

#endif /* __JR_UI_H__ */
