/*
 *   jack-ladspa-host
 *    
 *   Copyright (C) Robert Ham 2002 (node@users.sourceforge.net)
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

#include "ac_config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <math.h>
#include <strings.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include <ladspa.h>

#ifdef HAVE_LRDF
#include <lrdf.h>
#endif

#include "plugin_mgr.h"
#include "plugin_desc.h"
#include "ui.h"

static void plugin_mgr_init_lrdf (ui_t * ui, plugin_mgr_t * plugin_mgr);


static gboolean
plugin_is_valid (const LADSPA_Descriptor * descriptor)
{
  unsigned long i;
  unsigned long icount = 0;
  unsigned long ocount = 0;
  
  for (i = 0; i < descriptor->PortCount; i++)
    {
      if (!LADSPA_IS_PORT_AUDIO (descriptor->PortDescriptors[i]))
        continue;
      
      if (LADSPA_IS_PORT_INPUT (descriptor->PortDescriptors[i]))
        icount++;
      else
        ocount++;
    }
  
  if (icount != ocount || icount == 0)
    return FALSE;
  
  return TRUE;
}

static void
plugin_mgr_get_object_file_plugins (ui_t * ui, plugin_mgr_t * plugin_mgr, const char * filename)
{
  const char * dlerr;
  void * dl_handle;
  LADSPA_Descriptor_Function get_descriptor;
  const LADSPA_Descriptor * descriptor;
  unsigned long plugin_index;
  plugin_desc_t * desc, * other_desc = NULL;
  GSList * list;
  gboolean exists;
  int err;
  
  /* open the object file */
  dl_handle = dlopen (filename, RTLD_NOW|RTLD_GLOBAL);
  if (!dl_handle)
    {
      fprintf (stderr, "%s: error opening shared object file '%s': %s\n",
               __FUNCTION__, filename, dlerror());
      return;
    }
  
  
  /* get the get_descriptor function */
  dlerror (); /* clear the error report */
  
  get_descriptor = (LADSPA_Descriptor_Function)
    dlsym (dl_handle, "ladspa_descriptor");
  
  dlerr = dlerror();
  if (dlerr) {
    fprintf (stderr, "%s: error finding ladspa_descriptor symbol in object file '%s': %s\n",
             __FUNCTION__, filename, dlerr);
    dlclose (dl_handle);
    return;
  }
  
  plugin_index = 0;
  while ( (descriptor = get_descriptor (plugin_index)) )
    {
      if (!plugin_is_valid (descriptor))
        {
          plugin_index++;
          continue;
        }

      
      /* check it doesn't already exist */
      exists = FALSE;
      for (list = plugin_mgr->all_plugins; list; list = g_slist_next (list))
        {
          other_desc = (plugin_desc_t *) list->data;
          
          if (other_desc->id == descriptor->UniqueID)
            {
              exists = TRUE;
              break;
            }
        }
      
      if (exists)
        {
          printf ("Plugin %ld exists in both '%s' and '%s'; using version in '%s'\n",
                  descriptor->UniqueID, other_desc->object_file, filename, other_desc->object_file);
          plugin_index++;
          continue;
        }

      
      desc = plugin_desc_new_with_descriptor (filename, plugin_index, descriptor);
      plugin_mgr->all_plugins = g_slist_append (plugin_mgr->all_plugins, desc);
      plugin_index++;
      plugin_mgr->plugin_count++;
      
      /* print in the splash screen */
      ui_display_splash_text (ui, "Loaded plugin '%s'", desc->name);
    }
  
  err = dlclose (dl_handle);
  if (err)
    {
      fprintf (stderr, "%s: error closing object file '%s': %s\n",
               __FUNCTION__, filename, dlerror ());
    }
}

static void
plugin_mgr_get_dir_plugins (ui_t * ui, plugin_mgr_t * plugin_mgr, const char * dir)
{
  DIR * dir_stream;
  struct dirent * dir_entry;
  char * file_name;
  int err;
  size_t dirlen;
  
  dir_stream = opendir (dir);
  if (!dir_stream)
    {
/*      fprintf (stderr, "%s: error opening directory '%s': %s\n",
               __FUNCTION__, dir, strerror (errno)); */
      return;
    }
  
  dirlen = strlen (dir);
  
  while ( (dir_entry = readdir (dir_stream)) )
    {
      if (strcmp (dir_entry->d_name, ".") == 0 ||
          strcmp (dir_entry->d_name, "..") == 0)
        continue;
  
      file_name = g_malloc (dirlen + 1 + strlen (dir_entry->d_name) + 1);
    
      strcpy (file_name, dir);
      if (file_name[dirlen - 1] == '/')
        strcpy (file_name + dirlen, dir_entry->d_name);
      else
        {
          file_name[dirlen] = '/';
          strcpy (file_name + dirlen + 1, dir_entry->d_name);
        }
    
      plugin_mgr_get_object_file_plugins (ui, plugin_mgr, file_name);
      
      g_free (file_name);
    }

  err = closedir (dir_stream);
  if (err)
    fprintf (stderr, "%s: error closing directory '%s': %s\n",
             __FUNCTION__, dir, strerror (errno));
}

static void
plugin_mgr_get_path_plugins (ui_t * ui, plugin_mgr_t * plugin_mgr)
{
  char * ladspa_path, * dir;
  
  ladspa_path = g_strdup (getenv ("LADSPA_PATH"));
  if (!ladspa_path)
    ladspa_path = g_strdup ("/usr/local/lib/ladspa:/usr/lib/ladspa");
  
  dir = strtok (ladspa_path, ":");
  do
    plugin_mgr_get_dir_plugins (ui, plugin_mgr, dir);
  while ((dir = strtok (NULL, ":")));

  g_free (ladspa_path);
}

static gint
plugin_mgr_sort (gconstpointer a, gconstpointer b)
{
  const plugin_desc_t * da;
  const plugin_desc_t * db;
  da = (const plugin_desc_t *) a;
  db = (const plugin_desc_t *) b;
  
  return strcasecmp (da->name, db->name);
}

plugin_mgr_t *
plugin_mgr_new (ui_t * ui)
{
  plugin_mgr_t * pm;
  
  pm = g_malloc (sizeof (plugin_mgr_t));
  pm->all_plugins = NULL;  
  pm->plugins = NULL;
  pm->plugin_count = 0;
  
#ifdef HAVE_LRDF  
  pm->lrdf_uris = NULL;
  plugin_mgr_init_lrdf (ui, pm);
#endif

  plugin_mgr_get_path_plugins (ui, pm);
  
  pm->all_plugins = g_slist_sort (pm->all_plugins, plugin_mgr_sort);
  
  return pm;
}

void
plugin_mgr_destroy (plugin_mgr_t * plugin_mgr)
{
  GSList * list;
  
  for (list = plugin_mgr->all_plugins; list; list = g_slist_next (list))
    plugin_desc_destroy ((plugin_desc_t *) list->data);
  
  g_slist_free (plugin_mgr->plugins);
  g_slist_free (plugin_mgr->all_plugins);
  free (plugin_mgr);
}


void
plugin_mgr_set_plugins (plugin_mgr_t * plugin_mgr, unsigned long rack_channels)
{
  GSList * list;
  plugin_desc_t * desc;

  /* clear the current plugins */
  g_slist_free (plugin_mgr->plugins);
  plugin_mgr->plugins = NULL;
  
  for (list = plugin_mgr->all_plugins; list; list = g_slist_next (list))
    {
      desc = (plugin_desc_t *) list->data;
      
      if (plugin_desc_get_copies (desc, rack_channels) != 0)
        plugin_mgr->plugins = g_slist_append (plugin_mgr->plugins, desc);
    }
}

static plugin_desc_t *
plugin_mgr_find_desc (plugin_mgr_t * plugin_mgr, GSList * plugins, unsigned long id)
{
  GSList * list;
  plugin_desc_t * desc;
  
  for (list = plugins; list; list = g_slist_next (list))
    {
      desc = (plugin_desc_t *) list->data;
      
      if (desc->id == id)
        return desc;
    }
  
  return NULL;
}

plugin_desc_t *
plugin_mgr_get_desc (plugin_mgr_t * plugin_mgr, unsigned long id)
{
  return plugin_mgr_find_desc (plugin_mgr, plugin_mgr->plugins, id);
}

plugin_desc_t *
plugin_mgr_get_any_desc (plugin_mgr_t * plugin_mgr, unsigned long id)
{
  return plugin_mgr_find_desc (plugin_mgr, plugin_mgr->all_plugins, id);
}


static GtkWidget *
plugin_mgr_create_menu_item (plugin_desc_t * plugin, GCallback callback, gpointer data)
{
  GtkWidget * menu_item;
  char * filename;
  char * str;
  
  filename = strrchr (plugin->object_file, '/');
  if (filename)
    filename++;
  else
    filename = plugin->object_file;
  
  str = g_strdup_printf ("%s (%s, %ld %s%s)",
                         plugin->name,
                         filename,
                         plugin->channels,
                         _("ch"),
                         plugin->rt ? "" : _(", NOT RT"));
    
  menu_item = gtk_menu_item_new_with_label (str);
  
  g_free (str);
  
  /* use the glib object data stuff to carry the plugin */
  g_object_set_data (G_OBJECT (menu_item), "jack-rack-plugin-desc", plugin);
  g_object_set_data (G_OBJECT (menu_item), "jack-rack-plugin-id", GINT_TO_POINTER(plugin->id));
  
  g_signal_connect (G_OBJECT (menu_item), "activate", callback, data);
  gtk_widget_show (menu_item);
  
  return menu_item;
}


static GtkWidget *
plugin_mgr_create_alphabet_menu (GSList * plugins,
                      GCallback callback,
                      gpointer data)
{
  GtkWidget * menu, * submenu, * menu_item, * submenu_item;
  plugin_desc_t * plugin;
  GSList * list;
  const char * last_name;
  char letter_str[2];
  
  letter_str[1] = '\0';
  last_name = NULL;
  submenu = NULL;

  menu = gtk_menu_new ();
  
  for (list = plugins; list; list = g_slist_next (list))
    {
      plugin = list->data;
      
      if (!last_name ||
          (isdigit(*last_name) && !isdigit(*plugin->name)) ||
          (!isdigit(*last_name) && strncasecmp (last_name, plugin->name, 1) != 0)) /* letter boundary */
        {
          letter_str[0] = toupper (plugin->name[0]);
          menu_item = gtk_menu_item_new_with_label (letter_str);
          gtk_widget_show (menu_item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      
          submenu = gtk_menu_new ();
          gtk_widget_show (submenu);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
        }
    
      submenu_item = plugin_mgr_create_menu_item (plugin, callback, data);
      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), submenu_item);
    
      last_name = plugin->name;
    }
  
  return menu;
}


#ifdef HAVE_LRDF

static GtkWidget *
plugin_mgr_menu_item (GSList ** plugins,
                      unsigned long id,
                      GCallback callback,
                      gpointer data)
{
  GSList * list;
  plugin_desc_t * plugin;
  
  for (list = *plugins; list; list = g_slist_next (list))
    {
      plugin = (plugin_desc_t *) list->data;
      if (plugin->id == id)
        break;
    }
  
  if (!list)
    return NULL;
  
  *plugins = g_slist_remove (*plugins, plugin);
  
  return plugin_mgr_create_menu_item (plugin, callback, data);
}
                      

static GtkWidget *
plugin_mgr_menu_descend (GSList ** plugins,
                         const char * uri,
                         const char * base,
                         GCallback callback,
                         gpointer data)
{
  GtkWidget * menu = NULL;
  GtkWidget * menu_item;
  lrdf_uris * uris;
  unsigned int i;
  
  uris = lrdf_get_subclasses(uri);
  if (uris)
    {
      GtkWidget * sub_menu;
      const char * label;
      char * newbase;

      for (i = 0; i < uris->count; i++)
        {
          /* get the sub menu */
          label = lrdf_get_label (uris->items[i]);
          newbase = g_strdup_printf ("%s/%s", base, label);
          
          sub_menu = plugin_mgr_menu_descend (plugins, uris->items[i], newbase, callback, data);
          
          /* add it to the menu */
          if (sub_menu)
            {
              if (!menu)
                {
                  menu = gtk_menu_new ();
                  gtk_widget_show (menu);
                }

              menu_item = gtk_menu_item_new_with_label (label);
              gtk_widget_show (menu_item);
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), sub_menu);
            }
          
          g_free (newbase);
        }
    }
    
  
  uris = lrdf_get_instances(uri);
  if (uris)
    for (i = 0; i < uris->count; i++)
      {
        menu_item = plugin_mgr_menu_item (plugins, (unsigned long) lrdf_get_uid (uris->items[i]),
                                          callback, data);
        
        if (menu_item)
          {
            if (!menu)
              {
                menu = gtk_menu_new ();
                gtk_widget_show (menu);
              }
            
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
          }
      }
    
  return menu;
}

GtkWidget *
plugin_mgr_get_menu (plugin_mgr_t * plugin_mgr,  
                     GCallback callback,
                     gpointer data)
{
  GSList * plugins;
  GtkWidget * main_menu;
  GtkWidget * alphabet_menu = NULL;
  GtkWidget * menu_item;
  
  plugins = g_slist_copy (plugin_mgr->plugins);
  
  main_menu = plugin_mgr_menu_descend (&plugins, LADSPA_BASE "Plugin", "", callback, data);
  
  if (g_slist_length (plugins) > 0)
    {
      alphabet_menu = plugin_mgr_create_alphabet_menu (plugins, callback, data);
      g_slist_free (plugins);
    }
  
  if (!main_menu)
    return alphabet_menu;
  
  gtk_widget_hide (main_menu);
  
  if (!alphabet_menu)
    return main_menu;

  /* add the alphabet menu to the bottom of the lrdf menu */
  menu_item = gtk_menu_item_new_with_label ("Uncategorised");
  gtk_widget_show (menu_item);
  gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), menu_item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), alphabet_menu);
  gtk_widget_show (alphabet_menu);
  
  
  return main_menu;
}

#else /* !HAVE_LRDF */


GtkWidget *
plugin_mgr_get_menu (plugin_mgr_t * plugin_mgr,
                     GCallback callback,
                     gpointer data)
{
  printf ("getting menu\n");
  return plugin_mgr_create_alphabet_menu (plugin_mgr->plugins, callback, data);
}

#endif /* HAVE_LRDF */


/* lrdf path stuff and whatnot */
#ifdef HAVE_LRDF

static void
plugin_mgr_get_dir_uris (ui_t * ui, plugin_mgr_t * plugin_mgr, const char * dir)
{
  DIR * dir_stream;
  struct dirent * dir_entry;
  char * file_name;
  int err;
  size_t dirlen;
  char * extension;
  
  dir_stream = opendir (dir);
  if (!dir_stream)
    {
/*      fprintf (stderr, "%s: error opening directory '%s': %s\n",
               __FUNCTION__, dir, strerror (errno));*/
      return;
    }
  
  dirlen = strlen (dir);
  
  while ( (dir_entry = readdir (dir_stream)) )
    {
      /* check if it's a .rdf or .rdfs */
      extension = strrchr (dir_entry->d_name, '.');
      if (!extension)
        continue;
      if (strcmp (extension, ".rdf") != 0 &&
          strcmp (extension, ".rdfs") != 0)
        continue;
  
      file_name = g_malloc (dirlen + 1 + strlen (dir_entry->d_name) + 1 + 7);
    
      strcpy (file_name, "file://");
      strcpy (file_name + 7, dir);
      if ((file_name + 7)[dirlen - 1] == '/')
        strcpy (file_name + 7 + dirlen, dir_entry->d_name);
      else
        {
          (file_name + 7)[dirlen] = '/';
          strcpy (file_name + 7 + dirlen + 1, dir_entry->d_name);
        }
    
      plugin_mgr->lrdf_uris = g_slist_append (plugin_mgr->lrdf_uris, file_name);

      
      ui_display_splash_text (ui, "Found LRDF description file '%s'", file_name);
    }
  
  err = closedir (dir_stream);
  if (err)
    fprintf (stderr, "%s: error closing directory '%s': %s\n",
             __FUNCTION__, dir, strerror (errno));
}

static void
plugin_mgr_get_path_uris (ui_t * ui, plugin_mgr_t * plugin_mgr)
{
  char * lrdf_path, * dir;
  
  lrdf_path = g_strdup (getenv ("LADSPA_RDF_PATH"));
  if (!lrdf_path)
    lrdf_path = g_strdup ("/usr/local/share/ladspa/rdf:/usr/share/ladspa/rdf");
  
  dir = strtok (lrdf_path, ":");
  do
    plugin_mgr_get_dir_uris (ui, plugin_mgr, dir);
  while ((dir = strtok (NULL, ":")));

  g_free (lrdf_path);
}

static void
plugin_mgr_init_lrdf (ui_t * ui, plugin_mgr_t * plugin_mgr)
{
  GSList * list;
  int err;
  
  plugin_mgr_get_path_uris (ui, plugin_mgr);

  lrdf_init ();  
  
  for (list = plugin_mgr->lrdf_uris; list; list = g_slist_next (list))
    {
      err = lrdf_read_file ((const char *) list->data);
      if (err)
        fprintf (stderr, "%s: could not parse lrdf file '%s'\n", __FUNCTION__, (const char *) list->data);
    }
}

#endif /* HAVE_LRDF */

/* EOF */



