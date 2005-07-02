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

#include "ac_config.h"

#ifdef HAVE_XML

#include <gtk/gtk.h>
#include <libxml/tree.h>

#include "file.h"
#include "jack_rack.h"
#include "globals.h"
#include "plugin_settings.h"
#include "control_message.h"

xmlDocPtr
jack_rack_create_doc (jack_rack_t * jack_rack)
{
  xmlDocPtr doc;
  xmlNodePtr tree, jackrack, controlrow, wet_dry_values, midi_control;
  xmlDtdPtr dtd;
  plugin_slot_t * plugin_slot;
  GList * list;
  GSList *mlist;
  unsigned long control;
  unsigned long channel;
  gint copy;
  char num[32];
#ifdef HAVE_ALSA
  midi_control_t *midi_ctrl;
#endif
  
  doc = xmlNewDoc(XML_DEFAULT_VERSION);

  /* dtd */
  dtd = xmlNewDtd (doc, "jackrack", NULL, "http://purge.bash.sh/~rah/jack_rack_1.2.dtd");
  doc->intSubset = dtd;
  xmlAddChild ((xmlNodePtr)doc, (xmlNodePtr)dtd);
  
  jackrack = xmlNewDocNode(doc, NULL, "jackrack", NULL);
  xmlAddChild ((xmlNodePtr)doc, jackrack);
  
  /* channels */
  sprintf (num, "%ld", jack_rack->channels);
  xmlNewChild (jackrack, NULL, "channels", num);
  
  /* samplerate */
  sprintf (num, "%ld", sample_rate);
  xmlNewChild (jackrack, NULL, "samplerate", num);
  
  for (list = jack_rack->slots; list; list = g_list_next (list))
    {
      plugin_slot = (plugin_slot_t *) list->data;
      
      tree = xmlNewChild (jackrack, NULL, "plugin", NULL);
      
      /* id */
      sprintf (num, "%ld", plugin_slot->plugin->desc->id);
      xmlNewChild (tree, NULL, "id", num);
      
      /* enabled */
      xmlNewChild (tree, NULL, "enabled", settings_get_enabled (plugin_slot->settings) ? "true" : "false");
      
      /* wet/dry stuff */
      xmlNewChild (tree, NULL, "wet_dry_enabled", settings_get_wet_dry_enabled (plugin_slot->settings) ? "true" : "false");
      if (jack_rack->channels > 1)
        xmlNewChild (tree, NULL, "wet_dry_locked", settings_get_wet_dry_locked (plugin_slot->settings) ? "true" : "false");
      
      /* wet/dry values */
      wet_dry_values = xmlNewChild (tree, NULL, "wet_dry_values", NULL);
      for (channel = 0; channel < jack_rack->channels; channel++)
        {
          sprintf (num, "%f", settings_get_wet_dry_value (plugin_slot->settings, channel));
          xmlNewChild (wet_dry_values, NULL, "value", num);
        }
      
      /* lockall */
      if (plugin_slot->plugin->copies > 1 )
        xmlNewChild (tree, NULL, "lockall", settings_get_lock_all (plugin_slot->settings) ? "true" : "false");
      
      /* control rows */
      for (control = 0; control < plugin_slot->plugin->desc->control_port_count; control++)
        {
          controlrow = xmlNewChild (tree, NULL, "controlrow", NULL);
          
          /* locked */
          if (plugin_slot->plugin->copies > 1)
            xmlNewChild (controlrow, NULL, "lock", settings_get_lock (plugin_slot->settings, control) ? "true" : "false" );
          
          /* plugin values */
          for (copy = 0; copy < plugin_slot->plugin->copies; copy++)
            {
              sprintf (num, "%f", settings_get_control_value (plugin_slot->settings, copy, control));
              xmlNewChild (controlrow, NULL, "value", num);
            }
        }
      
      /* midi control */
#ifdef HAVE_ALSA
      for (mlist = plugin_slot->midi_controls; mlist; mlist = g_slist_next (mlist))
        {
          midi_ctrl = mlist->data;
          
          midi_control = xmlNewChild (tree, NULL, "midi_control", NULL);
          
          /* channel */
          sprintf (num, "%d", midi_control_get_midi_channel (midi_ctrl));
          xmlNewChild (midi_control, NULL, "midi_channel", num);

          /* param */
          sprintf (num, "%d", midi_control_get_midi_param (midi_ctrl));
          xmlNewChild (midi_control, NULL, "midi_param", num);
          
          if (midi_ctrl->ladspa_control)
            {
              xmlNodePtr ladspa;
              
              ladspa = xmlNewChild (midi_control, NULL, "ladspa", NULL);
              
              /* copy */
              sprintf (num, "%d", midi_ctrl->control.ladspa.copy);
              xmlNewChild (ladspa, NULL, "copy", num);

              /* control */
              sprintf (num, "%ld", midi_ctrl->control.ladspa.control);
              xmlNewChild (ladspa, NULL, "control", num);
            }
          else
            {
              xmlNodePtr wet_dry;
              
              wet_dry = xmlNewChild (midi_control, NULL, "wet_dry", NULL);
              
              /* channel */
              sprintf (num, "%ld", midi_ctrl->control.wet_dry.channel);
              xmlNewChild (wet_dry, NULL, "channel", num);
            }
        }
#endif /* HAVE_ALSA */

    }
  
  return doc;
}

int
ui_save_file (ui_t * ui, const char * filename)
{
  xmlDocPtr doc;
  int err;
  
  doc = jack_rack_create_doc (ui->jack_rack);
  
  err = xmlSaveFormatFile (filename, doc, TRUE);
  if (err == -1)
    ui_display_error (ui, E_ERROR, _("Could not save file '%s'"), filename);
  else
    err = 0;
  
  xmlFreeDoc (doc);
  
  return err;
}

static void
saved_rack_parse_plugin (saved_rack_t * saved_rack, saved_plugin_t * saved_plugin,
                         ui_t * ui, const char * filename, xmlNodePtr plugin)
{
  plugin_desc_t * desc;
  settings_t * settings = NULL;
  xmlNodePtr controlrow;
  xmlNodePtr value;
  xmlNodePtr node;
  xmlNodePtr sub_node;
  xmlChar *content;
  unsigned long num;
  unsigned long control = 0;
#ifdef HAVE_ALSA
  midi_control_t * midi_ctrl;
#endif


  for (node = plugin->children; node; node = node->next)
    {
      if (strcmp (node->name, "id") == 0)
        {
          content = xmlNodeGetContent (node);
          num = strtoul (content, NULL, 10);
          xmlFree (content);

          desc = plugin_mgr_get_any_desc (ui->plugin_mgr, num);
          if (!desc)
            {
              ui_display_error (ui, E_WARNING, _("The file '%s' contains an unknown plugin with ID '%ld'; skipping"), filename, num);
              return;
            }
          
          settings = settings_new (desc, saved_rack->channels, saved_rack->sample_rate);
        }
      else if (strcmp (node->name, "enabled") == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_enabled (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (strcmp (node->name, "wet_dry_enabled") == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_wet_dry_enabled (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (strcmp (node->name, "wet_dry_locked") == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_wet_dry_locked (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (strcmp (node->name, "wet_dry_values") == 0)
        {
          unsigned long channel = 0;
          
          for (sub_node = node->children; sub_node; sub_node = sub_node->next)
            {
              if (strcmp (sub_node->name, "value") == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_wet_dry_value (settings, channel, strtof (content, NULL));
                  xmlFree (content);
                  
                  channel++;
                }
            }
        }
      else if (strcmp (node->name, "lockall") == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_lock_all (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (strcmp (node->name, "controlrow") == 0)
        {
          gint copy = 0;

          for (sub_node = node->children; sub_node; sub_node = sub_node->next)
            {
              if (strcmp (sub_node->name, "lock") == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_lock (settings, control, strcmp (content, "true") == 0 ? TRUE : FALSE);
                  xmlFree (content);
                }
              else if (strcmp (sub_node->name, "value") == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_control_value (settings, copy, control, strtof (content, NULL));
                  xmlFree (content);
                  copy++;
                }
            }
          
          control++;
        }

#ifdef HAVE_ALSA
      else if (strcmp (node->name, "midi_control") == 0)
        {
          xmlNodePtr control_node;
          
          midi_ctrl = g_malloc (sizeof (midi_control_t));
          
          for (sub_node = node->children; sub_node; sub_node = sub_node->next)
            {
              if (strcmp (sub_node->name, "midi_channel") == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  midi_ctrl->midi_channel = atoi (content);
                  xmlFree (content);
                }
              else if (strcmp (sub_node->name, "midi_param") == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  midi_ctrl->midi_param = atoi (content);
                  xmlFree (content);
                }
              else if (strcmp (sub_node->name, "ladspa") == 0)
                {
                  midi_ctrl->ladspa_control = TRUE;
                  for (control_node = sub_node->children;
                       control_node;
                       control_node = control_node->next)
                    {
                      if (strcmp (control_node->name, "copy") == 0)
                        {
                          content = xmlNodeGetContent (control_node);
                          midi_ctrl->control.ladspa.copy = atoi (content);
                          xmlFree (content);
                        }
                      else if (strcmp (control_node->name, "control") == 0)
                        {
                          content = xmlNodeGetContent (control_node);
                          midi_ctrl->control.ladspa.control = strtoul (content, NULL, 10);
                          xmlFree (content);
                        }
                    }
                }
              else if (strcmp (sub_node->name, "wet_dry") == 0)
                {
                  midi_ctrl->ladspa_control = FALSE;
                  for (control_node = sub_node->children;
                       control_node;
                       control_node = control_node->next)
                    {
                      if (strcmp (control_node->name, "channel") == 0)
                        {
                          content = xmlNodeGetContent (control_node);
                          midi_ctrl->control.wet_dry.channel = strtoul (content, NULL, 10);
                          xmlFree (content);
                        }
                    }
                }
            }
          
          saved_plugin->midi_controls =
            g_slist_append (saved_plugin->midi_controls, midi_ctrl);
        }
#endif /* HAVE_ALSA */

    }
  
  if (settings)
    saved_plugin->settings = settings;
}

static void
saved_rack_parse_jackrack (saved_rack_t * saved_rack, ui_t * ui, const char * filename, xmlNodePtr jackrack)
{
  xmlNodePtr node;
  xmlChar *content;
  saved_plugin_t * saved_plugin;

  for (node = jackrack->children; node; node = node->next)
    {
      if (strcmp (node->name, "channels") == 0)
        {
          content = xmlNodeGetContent (node);
          saved_rack->channels = strtoul (content, NULL, 10);
          xmlFree (content);
        }
      else if (strcmp (node->name, "samplerate") == 0)
        {
          content = xmlNodeGetContent (node);
          saved_rack->sample_rate = strtoul (content, NULL, 10);
          xmlFree (content);
        }
      else if (strcmp (node->name, "plugin") == 0)
        {
          saved_plugin = g_malloc0 (sizeof (saved_plugin_t));
          saved_rack->plugins = g_slist_append (saved_rack->plugins, saved_plugin);
          saved_rack_parse_plugin (saved_rack, saved_plugin, ui, filename, node);
        }
    }
}

static saved_rack_t *
saved_rack_new (ui_t * ui, const char * filename, xmlDocPtr doc)
{
  xmlNodePtr node;
  saved_rack_t *saved_rack;
  
  /* create the saved rack */
  saved_rack = g_malloc (sizeof (saved_rack_t));
  saved_rack->plugins = NULL;
  saved_rack->sample_rate = 48000;
  saved_rack->channels = 2;
  
  for (node = doc->children; node; node = node->next)
    {
      if (strcmp (node->name, "jackrack") == 0)
        saved_rack_parse_jackrack (saved_rack, ui, filename, node);
    }
  
  return saved_rack;
}

static void
saved_rack_destroy (saved_rack_t * saved_rack)
{
  GSList * list;
  
/*  for (list = saved_rack->settings; list; list = g_slist_next (list))
    settings_destroy ((settings_t *) list->data); */
/*  g_slist_free (saved_rack->settings); */
  
  g_free (saved_rack);
}


int
ui_open_file (ui_t * ui, const char * filename)
{
  xmlDocPtr doc;
  saved_rack_t * saved_rack;
  ctrlmsg_t ctrlmsg;
  GSList * list;
  saved_plugin_t * saved_plugin;

  doc = xmlParseFile (filename);
  if (!doc)
    {
      ui_display_error (ui, E_ERROR, _("Could not parse file '%s'"), filename);
      return 1;
    }
  
  if (strcmp ( ((xmlDtdPtr)doc->children)->name, "jackrack") != 0)
    {
      ui_display_error (ui, E_ERROR, _("The file '%s' is not a JACK Rack settings file"), filename);
      return 1;
    }
  
  saved_rack = saved_rack_new (ui, filename, doc);
  xmlFreeDoc (doc);
  
  if (!saved_rack)
    return 1;

  if (ui->jack_rack->slots)
    {
      gboolean ok;
      
      ok = ui_get_ok (ui, _("The current rack will be cleared.\n\nAre you sure you want to continue?"));
      
      if (!ok)
        {
          saved_rack_destroy (saved_rack);
          return 1;
        }
    }
  
  if (saved_rack->channels != ui->jack_rack->channels)
    ui_set_channels (ui, saved_rack->channels);
  
  jack_rack_send_clear_plugins (ui->jack_rack);
  
  for (list = saved_rack->plugins; list; list = g_slist_next (list))
    {
      saved_plugin = list->data;
      
      settings_set_sample_rate (saved_plugin->settings, sample_rate);
      
      jack_rack_add_saved_plugin (ui->jack_rack, saved_plugin);
    }
  
  g_slist_free (saved_rack->plugins);  
  g_free (saved_rack);
  
  return 0;
}


#endif /* HAVE_XML */

/* EOF */


