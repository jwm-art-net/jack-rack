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
  xmlNodePtr tree, jackrack, controlrow;
  xmlDtdPtr dtd;
  char num[32];
  GList * list;
  plugin_slot_t * plugin_slot;
  unsigned long control;
  gint copy;
  
  doc = xmlNewDoc(XML_DEFAULT_VERSION);

  /* dtd */
  dtd = xmlNewDtd (doc, "jackrack", NULL, "http://purge.bash.sh/~rah/jack_rack_1.0.dtd");
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
      xmlNewChild (tree, NULL, "enabled", plugin_slot->enabled ? "true" : "false");
      
      /* lockall */
      if (plugin_slot->plugin->copies > 1)
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
              sprintf (num, "%g", settings_get_control_value (plugin_slot->settings, copy, control));
              xmlNewChild (controlrow, NULL, "value", num);
            }
        }
    }
  
  return doc;
}

int
ui_save_file (ui_t * ui, const char * filename)
{
  xmlDocPtr doc;
  int err;
  
  doc = jack_rack_create_doc (ui->jack_rack);
  
  err = xmlSaveFile (filename, doc);
  if (err == -1)
    ui_display_error (ui, _("Could not save file '%s'"), filename);
  else
    err = 0;
  
  xmlFreeDoc (doc);
  
  return err;
}

static saved_rack_t *
saved_rack_new (ui_t * ui, const char * filename, xmlDocPtr doc)
{
  xmlNodePtr jackrack, plugin, controlrow, value;
  saved_rack_t *saved_rack;
  xmlChar *content;
  settings_t * settings;
  unsigned long num;
  plugin_desc_t * desc;
  guint copy;
  unsigned long control;
  
  /* create the saved rack */
  saved_rack = g_malloc (sizeof (saved_rack_t));
  saved_rack->settings = NULL;
  
  jackrack = doc->children->next;
  
  content = xmlNodeGetContent (jackrack->children);
  saved_rack->channels = strtoul (content, NULL, 10);
  xmlFree (content);
  
  content = xmlNodeGetContent (jackrack->children->next);
  saved_rack->sample_rate = strtoul (content, NULL, 10);
  xmlFree (content);
  
  plugin = jackrack->children->next->next;
  
  if (!plugin)
    {
      g_free (saved_rack);
      return NULL;
    }
  
  do
    {
    
      /* id */
      content = xmlNodeGetContent (plugin->children);
      num = strtoul (content, NULL, 10);
      xmlFree (content);
      
      desc = plugin_mgr_get_any_desc (ui->plugin_mgr, num);
      if (!desc)
        {
          ui_display_error (ui, _("The file '%s' contains an unknown plugin with ID '%ld'; skipping"), filename, num);
          continue;
        }
      
      settings = settings_new (desc, plugin_desc_get_copies (desc, saved_rack->channels), saved_rack->sample_rate);
      
      /* enabled */
      content = xmlNodeGetContent (plugin->children->next);
      settings_set_enabled (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
      xmlFree (content);
      
      /* lock all */
      if (settings_get_copies (settings) > 1)
        {
          content = xmlNodeGetContent (plugin->children->next->next);
          settings_set_lock_all (settings, strcmp (content, "true") == 0 ? TRUE : FALSE);
          xmlFree (content);
          
          controlrow = plugin->children->next->next->next;
        }
      else
        controlrow = plugin->children->next->next;
      
      if (!controlrow)
        continue;
      
      control = 0;
      do
        {
          /* lock */
          if (settings_get_copies (settings) > 1)
            {
              content = xmlNodeGetContent (controlrow->children);
              settings_set_lock (settings, control, strcmp (content, "true") == 0 ? TRUE : FALSE);
              xmlFree (content);
              
              value = controlrow->children->next;
            }
          else
            value = controlrow->children;
          
          copy = 0;
          do
            {
              content = xmlNodeGetContent (value);
              settings_set_control_value (settings, copy, control, strtof (content, NULL));
              xmlFree (content);
              
              copy++;
            }
          while ( (value = value->next) );
          
          control++;
        }
      while ( (controlrow = controlrow->next) );
      
      saved_rack->settings = g_slist_append (saved_rack->settings, settings);
    }
  while ( (plugin = plugin->next) );
  
  return saved_rack;
}

static void
saved_rack_destroy (saved_rack_t * saved_rack)
{
  GSList * list;
  
  for (list = saved_rack->settings; list; list = g_slist_next (list))
    settings_destroy ((settings_t *) list->data);
  g_slist_free (saved_rack->settings);
  
  g_free (saved_rack);
}

int
ui_open_file (ui_t * ui, const char * filename)
{
  xmlDocPtr doc;
  saved_rack_t * saved_rack;
  ctrlmsg_t ctrlmsg;
  GSList * list;
  settings_t * settings;

  doc = xmlParseFile (filename);
  if (!doc)
    {
      ui_display_error (ui, _("Could not parse file '%s'"), filename);
      return 1;
    }
  
  if (strcmp ( ((xmlDtdPtr)doc->children)->name, "jackrack") != 0)
    {
      ui_display_error (ui, _("The file '%s' is not a JACK Rack settings file"), filename);
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
  
  ctrlmsg.type = CTRLMSG_CLEAR;
  lff_write (ui->ui_to_process, &ctrlmsg);
  
  for (list = saved_rack->settings; list; list = g_slist_next (list))
    {
      settings = (settings_t *) list->data;
      
      settings_set_sample_rate (settings, sample_rate);
      
      jack_rack_add_plugin (ui->jack_rack, settings->desc);
    }
  
  ui->jack_rack->saved_settings = saved_rack->settings;
  
  g_free (saved_rack);
  
  return 0;
}


#endif /* HAVE_XML */

/* EOF */


