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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "file.h"
#include "plugin_slot.h"
#include "plugin_desc.h"
#include "plugin_settings.h"
#include "process.h"
#include "callbacks.h"
#include "ui.h"

/*static size_t
getlinenn (char **lineptr, size_t *n, FILE *stream)
{
  size_t size;
  char * ptr;
  
  size = getline (lineptr, n, stream);
  
  ptr = strchr (*lineptr, '\n');
  if (ptr)
    *ptr = '\0';
  
  return size;
}*/

static int
jack_rack_read_file (jack_rack_t * jack_rack, FILE * file, const char * filename, GSList ** saved_plugins)
{
  return 1;
}

static int
jack_rack_read_filea (jack_rack_t * jack_rack, FILE * file, const char * filename, GSList ** saved_plugins)
{
  size_t err;
  char * line = NULL;
  size_t linesize = 0;
  unsigned long lineno = 1;
  char * endptr;
  unsigned long plugin_count, i;
  unsigned long plugin_id;
  plugin_desc_t * desc;
  settings_t * settings;
  saved_plugin_t * saved_plugin = NULL;
  
  /* header */
  err = getline (&line, &linesize, file);
  if (err == -1)
    {
      char * error;
      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      if (line)
        free (line);
      return 1;
    }
  
  if (strcmp ("JACK Rack Configuration File - Format 0\n", line) != 0)
    {
      char * error;
      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      free (line);
      return 1;
    }
  
  
  /* plugin count */
  lineno++;
  err = getline (&line, &linesize, file);
  if (err == -1)
    {
      char * error;
      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      free (line);
      return 1;
    }
  
  plugin_count = strtoul (line, &endptr, 10);
  
  if (plugin_count == ULONG_MAX || endptr == line)
    {
      char * error;
      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      free (line);
      return 1;
    }
  
  /* plugins */
  for (i = 0; i < plugin_count; i++)
    {
      /* plugin id */
      lineno++;
      err = getline (&line, &linesize, file);
      if (err == -1)
        {
          char * error;
          error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          free (line);
          return 1;
        }
      
      plugin_id = strtoul (line, &endptr, 10);
      if (plugin_id == ULONG_MAX || endptr == line)
        {
          char * error;
          error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          free (line);
          return 1;
        }
      
      desc = plugin_mgr_get_desc (jack_rack->ui->plugin_mgr, plugin_id);
      if (!desc)
        {
          char * error;
          error = g_strdup_printf ("Unknown plugin id %ld ('%s:%ld')", plugin_id, filename, lineno);
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          free (line);
          return 1;
        }
        
      settings = settings_new (desc, sample_rate);
      saved_plugin = saved_plugin_new ();
      saved_plugin->desc = desc;
      saved_plugin->settings = settings;
      
      /* enabled */
      lineno++;
      err = getline (&line, &linesize, file);
      if (err == -1)
        {
          char * error;
          error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          free (line);
          return 1;
        }
      
      saved_plugin->enabled = atoi (line);
      
      
      /* lock all */
/*      if (jack_rack-> && desc->control_port_count > 0)
        {
          lineno++;
          err = getline (&line, &linesize, file);
          if (err == -1)
            {
              char * error;
              error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
              jack_rack_display_error (jack_rack, error);
              g_free (error);
              free (line);
              return 1;
            }
          
          settings_set_lock_all (settings, atoi (line));
        } */
      
      
      
      /* controls */
      if (desc->control_port_count > 0)
        {
          unsigned long control, copy;
          for (control = 0; control < desc->control_port_count; control++)
            {
              /* locks */
              /*if (desc->mono)
                {
                  lineno++;
                  err = getline (&line, &linesize, file);
                  if (err == -1)
                    {
                      char * error;
                      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
                      jack_rack_display_error (jack_rack, error);
                      g_free (error);
                      free (line);
                      return 1;
                    }
                  
                  settings_set_lock (settings, control, atoi (line));
                }*/
              
              /* control values */
/*              for (copy = 0; copy < plugin_copies (desc); copy++)
                {
                  lineno++;
                  err = getline (&line, &linesize, file);
                  if (err == -1)
                    {
                      char * error;
                      error = g_strdup_printf ("Malformed rack configuration file '%s:%ld'", filename, lineno);
                      jack_rack_display_error (jack_rack, error);
                      g_free (error);
                      free (line);
                      return 1;
                    }
                  
                  settings_set_control_value (settings, control, copy, atof (line));
                }*/
            }
        }

      *saved_plugins = g_slist_append (*saved_plugins, saved_plugin);
    }
  
  return 0;
}

void
jack_rack_open_file (jack_rack_t * jack_rack, const char * filename)
{
  FILE * file;
  int err;
  char * error;
  GSList * saved_plugins = NULL;
  GSList * list;
  saved_plugin_t * saved_plugin;

  file = fopen (filename, "r");
  if (!file)
    {
      error = g_strdup_printf ("Error opening file '%s' for reading:\n\n%s", filename, strerror (errno));
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      return;
    }
  
  err = jack_rack_read_file (jack_rack, file, filename, &saved_plugins);
  if (err)
    {
      for (list = saved_plugins; list; list = g_slist_next (list))
        {
          saved_plugin = (saved_plugin_t *) list->data;
          settings_destroy (saved_plugin->settings);
          saved_plugin_destroy (saved_plugin);
        }
      g_slist_free (saved_plugins);
    }
  
  new_cb (NULL, jack_rack);
  
  for (list = saved_plugins; list; list = g_slist_next (list))
    {
      saved_plugin = (saved_plugin_t *) list->data;
      
      jack_rack->saved_settings = g_slist_append (jack_rack->saved_settings, saved_plugin->settings);
      
      jack_rack_add_plugin (jack_rack, saved_plugin->desc);
      
      saved_plugin_destroy (saved_plugin);
    }
  g_slist_free (saved_plugins);
  
  jack_rack_set_filename (jack_rack, filename);
}

static void
jack_rack_write_file (jack_rack_t * jack_rack, FILE * file, const char * filename)
{
  int err;
  plugin_slot_t * plugin_slot;
  plugin_desc_t * desc;
  unsigned long control, copy;
  settings_t * settings = NULL;
  GList * list;

  /* header */
  err = fprintf (file, "JACK Rack Configuration File - Format 0\n");
  if (err < 0)
    {
      char * error;
      error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      return;
    }
  
  /* plugin count */
  err = fprintf (file, "%ld\n", g_list_length (jack_rack->slots));
  if (err < 0)
    {
      char * error;
      error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      return;
    }
  
  for (list = jack_rack->slots; list; list = g_list_next (list))
    {
      plugin_slot = (plugin_slot_t *) list->data;
      desc = plugin_slot->plugin->desc;
      settings = plugin_slot->settings;

      /* plugin id */    
      err = fprintf (file, "%ld\n", desc->id);
      if (err < 0)
        {
          char * error;
          error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          return;
        }
      
      /* enable */
      err = fprintf (file, "%d\n", plugin_slot->enabled);
      if (err < 0)
        {
          char * error;
          error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
          jack_rack_display_error (jack_rack, error);
          g_free (error);
          return;
        }
      
      /* lock all */
/*      if (desc->mono && desc->control_port_count > 0)
        {
          err = fprintf (file, "%d\n", settings_get_lock_all (settings));
          if (err < 0)
            {
              char * error;
              error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
              jack_rack_display_error (jack_rack, error);
              g_free (error);
              return;
            }
        }*/
      
      
      if (desc->control_port_count > 0)
        {
          for (control = 0; control < desc->control_port_count; control++)
            {
              /* locks */
/*              if (desc->mono)
                {
                  err = fprintf (file, "%d\n", settings_get_lock (settings, control));
                  if (err < 0)
                    {
                      char * error;
                      error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
                      jack_rack_display_error (jack_rack, error);
                      g_free (error);
                      return;
                    }
                }*/
              
              /* control values */
/*              for (copy = 0; copy < plugin_copies (desc); copy++)
                {
                  err = fprintf (file, "%f\n", settings_get_control_value (settings, control, copy));
                  if (err < 0)
                    {
                      char * error;
                      error = g_strdup_printf ("Error writing to file '%s':\n\n%s", filename, strerror (errno));
                      jack_rack_display_error (jack_rack, error);
                      g_free (error);
                      return;
                    }
                } */
            }
        }
    }
  
  jack_rack_set_filename (jack_rack, filename);
}

void
jack_rack_save_file (jack_rack_t * jack_rack, const char * filename)
{
  FILE * file;
  int err;
  
  file = fopen (filename, "w");
  if (!file)
    {
      char * error;
      error = g_strdup_printf ("Error opening file '%s' for writing:\n\n%s", filename, strerror (errno));
      jack_rack_display_error (jack_rack, error);
      g_free (error);
      return;
    }
  
  jack_rack_write_file (jack_rack, file, filename);
  
  err = fclose (file);
  if (!file)
    {
      char * error;
      error = g_strdup_printf ("Error closing file '%s' after writing:\n\n%s", filename, strerror (errno));
      jack_rack_display_error (jack_rack, error);
      g_free (error);
    }
}


saved_plugin_t *
saved_plugin_new ()
{
  saved_plugin_t * sp;
  
  sp = g_malloc (sizeof (saved_plugin_t));
  
  sp->enabled = FALSE;
  sp->desc = NULL;
  sp->settings = NULL;
  
  return sp;
}

void
saved_plugin_destroy (saved_plugin_t * saved_plugin)
{
  g_free (saved_plugin);
}
