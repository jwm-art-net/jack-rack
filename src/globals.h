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

#ifdef HAVE_LASH
    #include <lash/lash.h>
    extern lash_client_t * global_lash_client;
#endif

#ifdef HAVE_LO
    #include "nsm.h"
    extern nsm_client_t* global_nsm_client;
    extern GString*      global_nsm_filename;
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
#       define _(x) dgettext(PACKAGE, x)
#else
#       define _(x) x
#endif
#define N_(x) x

extern struct _ui *global_ui;
extern gboolean   connect_inputs;
extern gboolean   connect_outputs;
extern gboolean   time_runs;
extern GString    *client_name;
extern GString    *session_uuid;
extern GString    *initial_filename;
extern int        global_nsm_state;


typedef enum MIDI_CONTROL_TYPE 
{
   LADSPA_CONTROL,
   WET_DRY_CONTROL,
   PLUGIN_ENABLE_CONTROL
} midi_control_type_t;

#define COPYRIGHT_MESSAGE \
	"Copyright (C) 2002, 2003 Robert Ham <node@users.sourceforge.net>\n" \
	"Copyright (C) 2005, 2006 Leslie P. Polzer <leslie.polzer@gmx.net>\n" \
	"Copyright (C) 2006, 2007 Adam Sampson <ats@offog.org>"

#endif /* __JLH_GLOBALS_H__ */

