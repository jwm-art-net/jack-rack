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

#include "ac_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#ifdef HAVE_LADCCA
#include <ladcca/ladcca.h>
#endif

#include "ui.h"
#include "control_message.h"
#include "ui_callbacks.h"
#include "control_callbacks.h"
#include "globals.h"

#define PROCESS_FIFO_SIZE 64
#define MIDI_FIFO_SIZE 256

static void
ui_init_gui_menu (ui_t * ui, GtkWidget * main_box)
{
  GtkWidget *menubar_handle;
  GtkWidget *menubar;
  GtkWidget *file_menuitem;
  
  GtkWidget *file_menu;
#ifdef HAVE_XML
  GtkWidget *save;
  GtkWidget *save_as;
#endif /* HAVE_XML */
  GtkWidget *separator;
  GtkWidget *quit;
  GtkWidget *rack_menuitem;
  GtkWidget *rack_menu;
  
#ifdef HAVE_GNOME
  GtkWidget *help_menuitem;
  GtkWidget *help_menu;
  GtkWidget *about;
#endif

  /* the menu bar */
  menubar_handle = gtk_handle_box_new ();
  gtk_widget_show (menubar_handle);
  gtk_box_pack_start (GTK_BOX (main_box), menubar_handle, FALSE, TRUE, 0);
  
  menubar = gtk_menu_bar_new ();
  gtk_widget_show (menubar);
  gtk_container_add (GTK_CONTAINER (menubar_handle), menubar);
  
  file_menuitem = gtk_menu_item_new_with_label (_("File"));
  gtk_widget_show (file_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), file_menuitem);

  rack_menuitem = gtk_menu_item_new_with_label (_("Rack"));
  gtk_widget_show (rack_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), rack_menuitem);
  

#ifdef HAVE_GNOME  
  help_menuitem = gtk_menu_item_new_with_label (_("Help"));
  gtk_widget_show (help_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), help_menuitem);
#endif

  /* file menu */
  file_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menuitem), file_menu);
  
  ui->new_menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
  gtk_widget_show (ui->new_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), ui->new_menuitem);
  g_signal_connect (G_OBJECT (ui->new_menuitem), "activate",
                    G_CALLBACK (new_cb), ui);

#ifdef HAVE_XML
  ui->open_menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
  gtk_widget_show (ui->open_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), ui->open_menuitem);
  g_signal_connect (G_OBJECT (ui->open_menuitem), "activate",
                    G_CALLBACK (open_cb), ui);

  save = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, NULL);
  gtk_widget_show (save);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), save);
  g_signal_connect (G_OBJECT (save), "activate",
                    G_CALLBACK (save_cb), ui);

  save_as = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, NULL);
  gtk_widget_show (save_as);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), save_as);
  g_signal_connect (G_OBJECT (save_as), "activate",
                    G_CALLBACK (save_as_cb), ui);
#endif /* HAVE_XML */

#ifdef HAVE_LADCCA
  if (cca_enabled (global_cca_client))
    {
      separator = gtk_separator_menu_item_new ();
      gtk_widget_show (separator);
      gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), separator);

      ui->cca_save_menu_item = gtk_menu_item_new_with_label (_("Save project"));
      gtk_widget_show (ui->cca_save_menu_item);
      gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), ui->cca_save_menu_item);
      g_signal_connect (G_OBJECT (ui->cca_save_menu_item), "activate",
                        G_CALLBACK (cca_save_cb), ui);
    }
#endif /* HAVE_LADCCA */

  separator = gtk_separator_menu_item_new ();
  gtk_widget_show (separator);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), separator);
  
  quit = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  gtk_widget_show (quit);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit);
  g_signal_connect (G_OBJECT (quit), "activate",
                    G_CALLBACK (quit_cb), ui);


  /* rack menu */
  rack_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (rack_menuitem), rack_menu);
  
  ui->add_menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
  gtk_widget_show (ui->add_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (rack_menu), ui->add_menuitem);

  ui->add_menu = plugin_mgr_get_menu (ui->plugin_mgr, G_CALLBACK (add_cb), ui);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(ui->add_menuitem), ui->add_menu);

  ui->channels_menuitem = gtk_menu_item_new_with_label (_("Channels"));
  gtk_widget_show (ui->channels_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (rack_menu), ui->channels_menuitem);
  g_signal_connect (G_OBJECT (ui->channels_menuitem), "activate",
                    G_CALLBACK (channel_cb), ui);

#ifdef HAVE_ALSA  
  ui->midi_menuitem = gtk_menu_item_new_with_label (_("MIDI Controls"));  
  gtk_widget_show (ui->midi_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (rack_menu), ui->midi_menuitem);
  g_signal_connect (G_OBJECT (ui->midi_menuitem), "activate",
                    G_CALLBACK (midi_cb), ui);
#endif /* HAVE_ALSA */

#ifdef HAVE_GNOME
  /* help menu */
  help_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(help_menuitem), help_menu);
  
  about = gtk_menu_item_new_with_label (_("About"));
  gtk_widget_show (about);
  gtk_menu_shell_append (GTK_MENU_SHELL (help_menu), about);
  g_signal_connect (G_OBJECT (about), "activate",
                    G_CALLBACK (about_cb), ui);
#endif
}

static void
ui_set_default_window_icon ()
{
  gchar * icon_file;
  GError * icon_error = NULL;
  GList * icons = NULL;
  GdkPixbuf * icon;

  /* set the icons */
  icon_file = g_strdup_printf ("%s/pixmaps/%s", JR_DESKTOP_PREFIX, JACK_RACK_ICON_FILE);
  icon = gdk_pixbuf_new_from_file (icon_file, &icon_error);
  if (icon_error)
    {
      fprintf (stderr, "%s: error loading icon image from file '%s': '%s'\n",
               __FUNCTION__, icon_file, icon_error->message);
      g_error_free (icon_error);
    }
  g_free (icon_file);
  
  icons = g_list_append (icons, icon);

  gtk_window_set_default_icon_list (icons);
  
  g_list_free (icons);
  g_object_unref (icon);
}

static void
ui_init_gui (ui_t * ui, unsigned long channels)
{
  GtkWidget *main_box;
  GtkWidget *toolbar_handle;
  GtkWidget *toolbar;
  GtkWidget *plugin_scroll;
  GtkWidget *plugin_viewport;
  GtkWidget *channel_icon;
  gchar *channel_icon_file;


  /* main window */
  ui->main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (ui->main_window), client_name->str);
  gtk_window_set_default_size (GTK_WINDOW (ui->main_window), 620, 460);
  g_signal_connect (G_OBJECT (ui->main_window), "delete_event",
                        G_CALLBACK (window_destroy_cb), ui);  
                        
  

  /* button/viewport box */
  main_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (main_box);
  gtk_container_add (GTK_CONTAINER (ui->main_window), main_box);

  ui_init_gui_menu (ui, main_box);

  /* top button box and buttons */
  toolbar_handle = gtk_handle_box_new ();
  gtk_widget_show (toolbar_handle);
  gtk_box_pack_start (GTK_BOX (main_box), toolbar_handle, FALSE, FALSE, 0);

  toolbar = gtk_toolbar_new ();
  gtk_widget_show (toolbar);
  gtk_container_add (GTK_CONTAINER (toolbar_handle), toolbar);


  ui->add = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
				      GTK_STOCK_ADD,
				      _("Add a plugin"),
				      (const char *) NULL,
				      (GtkSignalFunc) NULL,
				      (gpointer)NULL,
				      (gint)-1);
  g_signal_connect_swapped (G_OBJECT (ui->add), "event",
                            G_CALLBACK (plugin_button_cb),
                            G_OBJECT (ui->add_menu));
                                                         

  /* channels */
  channel_icon_file = g_strdup_printf ("%s/%s", PKGDATADIR, JACK_RACK_CHANNELS_ICON_FILE);
  channel_icon = gtk_image_new_from_file (channel_icon_file);
  g_free (channel_icon_file);
  ui->channels = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Channels"),
                           _("Change the number of I/O channels the rack has"),
                           NULL,
                           channel_icon,
                           G_CALLBACK (channel_cb), ui);
                              
  
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  ui->new = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                            GTK_STOCK_NEW,
                            _("Clear the rack"),
                            NULL, G_CALLBACK (new_cb), ui, -1);

#ifdef HAVE_XML
  ui->open = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                            GTK_STOCK_OPEN,
                            _("Open a rack configuration file"),
                            NULL, G_CALLBACK (open_cb), ui, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                            GTK_STOCK_SAVE,
                            _("Save the rack configuration to the current file"),
                            NULL, G_CALLBACK (save_cb), ui, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                            GTK_STOCK_SAVE_AS,
                            _("Save the rack configuration to a file"),
                            NULL, G_CALLBACK (save_as_cb), ui, -1);
#endif /* HAVE_XML */

#ifdef HAVE_LADCCA
  if (cca_enabled (global_cca_client))
    {
      gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

      ui->cca_save = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                                           GTK_STOCK_SAVE,
                                           _("Save the LADCCA project"),
                                           NULL, G_CALLBACK (cca_save_cb), ui, -1);
    }

#endif /* HAVE_LADCCA */
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar),
                            GTK_STOCK_QUIT,
                            _("Quit JACK Rack"),
                            NULL, G_CALLBACK (quit_cb), ui, -1);

  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

                              

  /* viewport thingy */
  plugin_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (plugin_scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_box), plugin_scroll, TRUE, TRUE, 0);
  gtk_widget_show (plugin_scroll);

  plugin_viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (plugin_scroll), plugin_viewport);
  gtk_widget_show (plugin_viewport);


  /* the main vbox for the plugins */
  ui->plugin_box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (plugin_viewport), ui->plugin_box);
  gtk_widget_show (ui->plugin_box);

  gtk_widget_show (ui->main_window);
  
  /* the midi popup menu */
#ifdef HAVE_ALSA
  ui->midi_menu = gtk_menu_new ();
                       
  ui->midi_menu_item = gtk_menu_item_new_with_label (_("Add MIDI control"));
  g_signal_connect (G_OBJECT (ui->midi_menu_item), "activate",
                    G_CALLBACK (control_add_midi_cb), NULL);
  gtk_widget_show (ui->midi_menu_item);
                       
  gtk_menu_shell_append (GTK_MENU_SHELL (ui->midi_menu), ui->midi_menu_item);
#endif /* HAVE_ALSA */

#ifdef HAVE_XML
  /* open file from command line, if any */
  if ( initial_filename != NULL )
  {
    gchar* fn = g_string_free ( initial_filename, FALSE );
    int err = ui_open_file (ui, fn);
  
    if (!err)
      ui_set_filename (ui, fn);
    /* free? */
  }
#endif /* HAVE_XML */
}

void
ui_init_splash_screen (ui_t * ui)
{
  gchar * logo_file;
  GtkWidget * logo;
  GtkWidget * box;
  int i;
  gint h, w;
  
  ui_set_default_window_icon ();
  
  ui->splash_screen = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated (GTK_WINDOW (ui->splash_screen), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (ui->splash_screen), FALSE);
  gtk_window_set_title (GTK_WINDOW (ui->splash_screen), PACKAGE_NAME);
  gtk_container_set_resize_mode (GTK_CONTAINER (ui->splash_screen), GTK_RESIZE_IMMEDIATE);
  gtk_window_set_policy (GTK_WINDOW (ui->splash_screen), FALSE, TRUE, FALSE);
  
  box = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 3);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (ui->splash_screen), box);
  
  logo_file = g_strdup_printf ("%s/%s", PKGDATADIR, JACK_RACK_LOGO_FILE);
  logo = gtk_image_new_from_file (logo_file);
  gtk_widget_show (logo);
  gtk_box_pack_start (GTK_BOX (box), logo, FALSE, FALSE, 0);
  
  ui->splash_screen_text = gtk_label_new (NULL);
  gtk_widget_show (ui->splash_screen_text);
  gtk_box_pack_start (GTK_BOX (box), ui->splash_screen_text, TRUE, TRUE, 0);
  //  gtk_misc_set_alignment (GTK_MISC (ui->splash_screen_text), 0.0, 0.5);
  gtk_label_set_justify (GTK_LABEL (ui->splash_screen_text), GTK_JUSTIFY_CENTER);
  
  gtk_window_get_size (GTK_WINDOW (ui->splash_screen), &w, &h);
  gtk_window_move (GTK_WINDOW (ui->splash_screen),
                   (gdk_screen_width() / 2) - (w / 2),
                   (gdk_screen_height() / 2) - (h / 2));
  gtk_window_set_position (GTK_WINDOW (ui->splash_screen), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (ui->splash_screen);

  for (i = 0; i < 20; i++)
    gtk_main_iteration_do (FALSE);
}

ui_t *
ui_new (unsigned long channels)
{
  ui_t * ui;

  ui = g_malloc (sizeof (ui_t));
  ui->splash_screen = NULL;
  ui->splash_screen_text = NULL;
  ui->filename = NULL;
  ui->shutdown = FALSE;
  ui->state = STATE_NORMAL;
  ui->main_window = NULL;

#ifdef HAVE_LADCCA  
  ui->cca_save = NULL;
  ui->cca_save_menu_item = NULL;
#endif

  ui->ui_to_process = lff_new (PROCESS_FIFO_SIZE, sizeof (ctrlmsg_t));
  ui->process_to_ui = lff_new (PROCESS_FIFO_SIZE, sizeof (ctrlmsg_t));
#ifdef HAVE_ALSA
  ui->ui_to_midi    = lff_new (MIDI_FIFO_SIZE, sizeof (ctrlmsg_t));
  ui->midi_to_ui    = lff_new (MIDI_FIFO_SIZE, sizeof (ctrlmsg_t));
#endif

  ui_init_splash_screen (ui);
  
  ui->procinfo = process_info_new (ui, channels);
  if (!ui->procinfo)
    return NULL;

#ifdef HAVE_ALSA
  ui->midi_info   = midi_info_new (ui);
#endif
  ui->plugin_mgr = plugin_mgr_new (ui);
  plugin_mgr_set_plugins (ui->plugin_mgr, channels);
  ui->jack_rack = jack_rack_new (ui, channels);

  gtk_idle_add (idle_cb, ui);

  ui_init_gui (ui, channels);

#ifdef HAVE_ALSA  
  ui->midi_window = midi_window_new (ui);
#endif

  gtk_widget_destroy (ui->splash_screen);
  ui->splash_screen = NULL;
  ui->splash_screen_text = NULL;

  return ui;
}


void
ui_destroy (ui_t * ui)
{
  jack_rack_destroy (ui->jack_rack);  
#ifdef HAVE_ALSA
  midi_info_destroy (ui->midi_info);
#endif

  gtk_widget_destroy (ui->main_window);
  
  g_free (ui);
}

void
ui_display_error (ui_t * ui, const char * format, ...)
{
  va_list args;
  gchar * text;
  GtkWidget * dialog;
  
  va_start(args, format);
  
  text = g_strdup_vprintf (format, args);

  dialog = gtk_message_dialog_new (ui->main_window ? GTK_WINDOW (ui->main_window) : NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
                                   "%s", text);
  
  g_free (text);

  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);
  
  va_end(args);
}

void
ui_display_splash_text (ui_t * ui, const char * format, ...)
{
  va_list args;
  gchar * text;
  short j;
  
  if (!ui->splash_screen)
    return;
  
  va_start(args, format);
  
  text = g_strdup_vprintf (format, args);
  gtk_label_set_text (GTK_LABEL (ui->splash_screen_text), text);
  g_free (text);
  
  for (j = 0; j < 5; j++)
    gtk_main_iteration_do (FALSE);
  
  va_end(args);
}

gboolean
ui_get_ok (ui_t * ui, const char * format, ...)
{
  GtkWidget * dialog;
  gint response;
  va_list args;
  gchar * text;
  
  va_start(args, format);
  
  text = g_strdup_vprintf (format, args);
  va_end(args);
  
  dialog = gtk_message_dialog_new (ui->main_window ? GTK_WINDOW (ui->main_window) : NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_OK_CANCEL,
                                   "%s", text);
  g_free (text);

  response = gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);
  
  if (response == GTK_RESPONSE_OK)
    return TRUE;
  
  return FALSE;
}

void
ui_set_filename (ui_t * ui, const char * filename)
{
  if (ui->filename == filename)
    return;
  
  set_string_property (ui->filename, filename);

  if (filename)
    {
      const char * base_filename;
      char * window_title;
      
      base_filename = strrchr (filename, '/');
      if (base_filename)
        base_filename++;
      else
        base_filename = filename;
      
      window_title = g_strdup_printf ("%s - %s", client_name->str, base_filename);
      gtk_window_set_title (GTK_WINDOW (ui->main_window), window_title);
      g_free (window_title);
    }
  else
    gtk_window_set_title (GTK_WINDOW (ui->main_window), client_name->str);
}


void
ui_set_state    (ui_t * ui, ui_state_t state)
{
  ui->state = state;
}

ui_state_t
ui_get_state (ui_t * ui)
{
  return ui->state;
}

void
ui_set_channels (ui_t * ui, unsigned long channels)
{
  GList * slots, * list;
  plugin_slot_t * plugin_slot;
  plugin_t * plugin;

  jack_deactivate (ui->procinfo->jack_client);
  
  /* sort out the procinfo */
  ui->procinfo->chain = NULL;
  ui->procinfo->chain_end = NULL;
  
  /* remove all the slots */
  slots = g_list_copy (ui->jack_rack->slots);
  for (list = slots; list; list = g_list_next (list))
    {
      plugin_slot = (plugin_slot_t *) list->data;
      plugin = plugin_slot->plugin;

#ifdef HAVE_ALSA      
      plugin_slot_remove_midi_controls (plugin_slot);
#endif

      jack_rack_remove_plugin_slot (ui->jack_rack, plugin_slot);
      plugin_destroy (plugin, ui);
    }
  g_list_free (slots);
  
  jack_rack_destroy (ui->jack_rack);
  g_signal_handlers_disconnect_by_func (G_OBJECT (ui->add), G_CALLBACK (plugin_button_cb), G_OBJECT(ui->add_menu));
  gtk_widget_destroy (ui->add_menu);
  
  
  /* recreate the rack */
  process_info_set_channels (ui->procinfo, ui, channels);
  plugin_mgr_set_plugins (ui->plugin_mgr, channels);
  ui->jack_rack = jack_rack_new (ui, channels);

  ui->add_menu = plugin_mgr_get_menu (ui->plugin_mgr, G_CALLBACK (add_cb), ui);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(ui->add_menuitem), ui->add_menu);
  g_signal_connect_swapped (G_OBJECT (ui->add), "event",
                            G_CALLBACK (plugin_button_cb),
                            G_OBJECT (ui->add_menu));
  
  jack_activate (ui->procinfo->jack_client);
}

/* EOF */



