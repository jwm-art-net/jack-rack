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

#ifndef __JLH_GLOBALS_H__
#define __JLH_GLOBALS_H__

#include <glib.h>

#include "ac_config.h"


#ifdef HAVE_LADCCA
#   ifdef HAVE_LASH
       #include <lash/lash.h>
       #define cca_client_t lash_client_t
       #define cca_event_t lash_event_t
       #define cca_enabled lash_enabled

       #define CCA_Save LASH_Save
       #define CCA_Save_File LASH_Save_File
       #define CCA_Restore_File LASH_Restore_File
       #define CCA_Quit LASH_Quit
       #define CCA_Server_Lost LASH_Server_Lost

       #define cca_args_t lash_args_t
       #define CCA_Config_File LASH_Config_File
       #define CCA_Client_Name LASH_Client_Name
       #define CCA_PROTOCOL LASH_PROTOCOL
       
       #define cca_alsa_client_id lash_alsa_client_id
       #define cca_jack_client_name lash_jack_client_name
       #define cca_event_new_with_type lash_event_new_with_type
       #define cca_get_event lash_get_event
       #define cca_event_get_type lash_event_get_type
       #define cca_event_destroy lash_event_destroy
       #define cca_event_get_string lash_event_get_string
       #define cca_get_fqn lash_get_fqn
       #define cca_send_event lash_send_event
       #define cca_extract_args lash_extract_args
       #define cca_init lash_init
       #define cca_event_new_with_type lash_event_new_with_type
       #define cca_event_set_string lash_event_set_string
#   else
       #include <ladcca/ladcca.h>
#   endif
    extern cca_client_t * global_cca_client;
#endif


#define JACK_RACK_LOGO_FILE "jack-rack-logo.png"
#define JACK_RACK_ICON_FILE "jack-rack-icon.png"
#define JACK_RACK_CHANNELS_ICON_FILE "gnome-mixer-small.png"
#define JACK_RACK_URL "http://jack-rack.sourceforge.net/"


#define set_string_property(property, value) \
  \
  if (property) \
    g_free (property); \
  \
  if (value) \
    (property) = g_strdup (value); \
  else \
    (property) = NULL;

#ifdef ENABLE_NLS
#       include <libintl.h>
#       define _(x) gettext(x)
#else
#       define _(x) x
#endif
#define N_(x) x

extern struct _ui *global_ui;
extern gboolean   connect_inputs;
extern gboolean   connect_outputs;
extern gboolean   time_runs;
extern GString    *client_name;
extern GString    *initial_filename;

#endif /* __JLH_GLOBALS_H__ */

