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
 
#define _GNU_SOURCE

#include "ac_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#ifdef HAVE_GNOME
#include <libgnomeui/libgnomeui.h>
#endif

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "callbacks.h"
#include "control_message.h"
#include "globals.h"
#include "file.h"
#include "ui.h"
#include "wet_dry_controls.h"

void
add_cb (GtkMenuItem * menuitem, gpointer user_data)
{
  ui_t * ui;
  plugin_desc_t * desc;
  
  ui = (ui_t *) user_data;
  
  /* get the selected plugin out of the gobject data thing */
  desc = g_object_get_data (G_OBJECT(menuitem), "jack-rack-plugin-desc");
  if (!desc)
    {
      fprintf (stderr, "%s: no plugin description!\n", __FUNCTION__);
      return;
    }
  
  jack_rack_add_plugin (ui->jack_rack, desc);
}

void
channel_cb (GtkWidget * widget, gpointer user_data)
{
  ui_t * ui;
  unsigned long channels;
  GtkWidget * dialog;
  GtkWidget * spin;
  GtkWidget * warning;
  gint response;
  
  ui = (ui_t *) user_data;
  
  dialog = gtk_dialog_new_with_buttons ("I/O Channels",
                                        GTK_WINDOW (ui->main_window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,     GTK_RESPONSE_ACCEPT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        NULL);
  
  spin = gtk_spin_button_new_with_range (1.0, 60, 1.0);
  gtk_widget_show (spin);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), ui->jack_rack->channels);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), spin);

  warning = gtk_label_new ("Warning: changing the number of\nchannels will clear the current rack");
  gtk_widget_show (warning);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), warning);
  
  
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  
  if (response != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }
  
  channels = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin));
  
  gtk_widget_destroy (dialog);
  
  if (channels == ui->jack_rack->channels)
    return;
  
  ui_set_channels (ui, channels);
}

void
new_cb (GtkWidget * widget, gpointer user_data)
{
  ui_t * ui;
  ctrlmsg_t ctrlmsg;
  
  ui = (ui_t *) user_data;
  
  ctrlmsg.type = CTRLMSG_CLEAR;
  
  lff_write (ui->ui_to_process, &ctrlmsg);
  
  ui_set_filename (ui, NULL);
}

static const char *
get_filename (jack_rack_t * jack_rack)
{
  static GtkWidget * dialog = NULL;
  static char * file = NULL;
  int response;
  
  if (!dialog)
    dialog = gtk_file_selection_new ("Select file");
  
  
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);
  
  if (response != GTK_RESPONSE_OK)
    return NULL;

  if (file)
    g_free (file);
  
  file = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog)));
  return file;
}


#ifdef HAVE_XML
void
open_cb (GtkButton * button, gpointer user_data)
{
  const char * filename;
  ui_t * ui;
  int err;
  
  ui = (ui_t *) user_data;
  
  filename = get_filename (ui->jack_rack);
  
  if (!filename)
    return;
  
  err = ui_open_file (ui, filename);
  
  if (!err)
    ui_set_filename (ui, filename);
}
 
void
save_cb (GtkButton * button, gpointer user_data)
{
  ui_t * ui;
  
  ui = (ui_t *) user_data;
  
  if (!ui->filename)
    {
      save_as_cb (button, user_data);
      return;
    }
  
  ui_save_file (ui, ui->filename);
}


 
void
save_as_cb (GtkButton * button, gpointer user_data)
{
  const char * filename;
  ui_t * ui;
  int err;
  
  ui = (ui_t *) user_data;
  
  filename = get_filename (ui->jack_rack);
  
  if (!filename)
    return;
  
  err = ui_save_file (ui, filename);
  
  if (!err)
    ui_set_filename (ui, filename);
}
#endif /* HAVE_XML */
 
#ifdef HAVE_LADCCA
void
cca_save_cb (GtkButton * button, gpointer user_data)
{
  cca_event_t * event;
  
  printf ("%s: sending event\n", __FUNCTION__);
  event = cca_event_new_with_type (CCA_Save);
  cca_send_event (global_cca_client, event);
  printf ("%s: event sent\n", __FUNCTION__);
}
#endif /* HAVE_LADCCA */


void
quit_cb (GtkButton * button, gpointer user_data)
{
  ui_t * ui;
  ctrlmsg_t ctrlmsg;
  
  ui = user_data;
  
  if (ui->shutdown)
    {
      ui_set_state (ui, STATE_QUITTING);
      gtk_main_quit ();
      return;
    }
  
  ctrlmsg.type = CTRLMSG_CLEAR;
  lff_write (ui->ui_to_process, &ctrlmsg);

  ctrlmsg.type = CTRLMSG_QUIT;
  lff_write (ui->ui_to_process, &ctrlmsg);
}

gboolean
window_destroy_cb (GtkWidget *widget,
                         GdkEvent *event,
                         gpointer user_data)
{
  quit_cb (NULL, user_data);
  return TRUE;
}

#ifdef HAVE_GNOME
void
about_cb (GtkWidget * widget, gpointer user_data)
{
  GtkWidget * dialog;
  const char * authors[] = { "Bob Ham <node@users.sourceforge.net>", NULL };
  const char * documenters[] = { NULL };
  GtkWidget * url;
  gchar * logo_file;
  GdkPixbuf * logo;
  GError * err = NULL;
  
  logo_file = g_strdup_printf ("%s/%s", PKGDATADIR, JACK_RACK_LOGO_FILE);
  logo = gdk_pixbuf_new_from_file (logo_file, &err);
  if (err)
    {
      fprintf (stderr, "%s: error loading logo image from file '%s': '%s'\n",
               __FUNCTION__, logo_file, err->message);
      g_error_free (err);
    }
  g_free (logo_file);
  
  dialog = gnome_about_new (PACKAGE_NAME,
                              PACKAGE_VERSION,
                              "Copyright (C) 2002, 2003 Robert Ham <node@users.sourceforge.net>",
                              "A stereo LADSPA effects rack for the JACK audio API",
                              authors,
                              documenters,
                              _(""),
                              logo);
  
  url = gnome_href_new (JACK_RACK_URL, JACK_RACK_URL);
  gtk_widget_show (url);
  GTK_WIDGET_UNSET_FLAGS (url, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), url, FALSE, FALSE, 0);
  
  gtk_dialog_run (GTK_DIALOG (dialog));
}
#endif /* HAVE_GNOME */

void
slot_change_cb (GtkMenuItem * menuitem, gpointer user_data)
{
  ctrlmsg_t ctrlmsg;
  plugin_slot_t * plugin_slot;
  plugin_t * plugin;
  plugin_desc_t * desc;
  
  plugin_slot = user_data;
  
  if (ui_get_state (global_ui) == STATE_RACK_CHANGE)
    return;
  
  desc = (plugin_desc_t *) g_object_get_data (G_OBJECT(menuitem), "jack-rack-plugin-desc");

  plugin = jack_rack_instantiate_plugin (global_ui->jack_rack, desc);
  
  if (!plugin)
    return;

  ctrlmsg.type = CTRLMSG_CHANGE;
  ctrlmsg.number = g_list_index (global_ui->jack_rack->slots, plugin_slot);
  ctrlmsg.pointer = plugin;
  ctrlmsg.second_pointer = plugin;
  
  lff_write (global_ui->ui_to_process, &ctrlmsg);
}

void
slot_move_cb (GtkButton * button, gpointer user_data)
{
  ctrlmsg_t ctrlmsg;
  plugin_slot_t * plugin_slot;
  gint up;
  GList * slot_list_data;

  plugin_slot = user_data;
  up = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(button), "jack-rack-up"));
  
  /* this is because we use indicies */
  if (ui_get_state (global_ui)  == STATE_RACK_CHANGE)
    return;
  
  
  slot_list_data = g_list_find (global_ui->jack_rack->slots, plugin_slot);
  /* check the logic of what we're doing */
  if ((up && !g_list_previous (slot_list_data)) ||
      (!up && !g_list_next (slot_list_data)))
    return;
  
  ui_set_state (global_ui, STATE_RACK_CHANGE);
  
  ctrlmsg.type = CTRLMSG_MOVE;
  ctrlmsg.number = g_list_index (global_ui->jack_rack->slots, plugin_slot);
  ctrlmsg.pointer = GINT_TO_POINTER (up);
  ctrlmsg.second_pointer = plugin_slot;
  
  lff_write (global_ui->ui_to_process, &ctrlmsg);
  
}

void
slot_remove_cb (GtkButton * button, gpointer user_data)
{
  ctrlmsg_t ctrlmsg;
  plugin_slot_t * plugin_slot;
  
  plugin_slot = (plugin_slot_t *) user_data;
  
  ctrlmsg.type = CTRLMSG_REMOVE;
  ctrlmsg.number = g_list_index (global_ui->jack_rack->slots, plugin_slot);
  ctrlmsg.second_pointer = plugin_slot;
  
  lff_write (global_ui->ui_to_process, &ctrlmsg);
}


gboolean
slot_ablise_cb (GtkWidget * button, GdkEventButton *event, gpointer user_data)
{
  if (event->type == GDK_BUTTON_PRESS) {
    plugin_slot_t * plugin_slot; 
    gboolean ablise;
  
    plugin_slot = (plugin_slot_t *) user_data;
    ablise = settings_get_enabled (plugin_slot->settings) ? FALSE : TRUE;
  
    plugin_slot_ablise (plugin_slot, ablise);
  
    return TRUE;
  }
  
  return FALSE;
}

gboolean
slot_wet_dry_cb (GtkWidget * button, GdkEventButton *event, gpointer user_data)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      plugin_slot_t * plugin_slot; 
      gboolean ablise;
    
      plugin_slot = (plugin_slot_t *) user_data;
      ablise = settings_get_wet_dry_enabled (plugin_slot->settings) ? FALSE : TRUE;
  
      plugin_slot_ablise_wet_dry (plugin_slot, ablise);
  
      return TRUE;
  }
  
  return FALSE;
}
    	                              

void
slot_wet_dry_control_cb (GtkRange * range, gpointer user_data)
{
  LADSPA_Data value;
  wet_dry_controls_t * wet_dry;
  unsigned long channel;
  
  wet_dry = (wet_dry_controls_t *) user_data;
  channel = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(range), "jack-rack-wet-dry-channel"));
  value = gtk_range_get_value (range);

  lff_write (wet_dry->fifos + channel, &value);
  
  /* set peers */
  if (settings_get_wet_dry_locked (wet_dry->plugin_slot->settings))
    {
      unsigned long c;
      for (c = 0; c < wet_dry->plugin_slot->jack_rack->channels ; c++)
        if (c != channel)
          gtk_range_set_value (GTK_RANGE(wet_dry->controls[c]),
                               gtk_range_get_value (range));
    }
}

void
slot_wet_dry_lock_cb    (GtkToggleButton * button, gpointer user_data)
{
  wet_dry_controls_t * wet_dry;
  
  wet_dry = (wet_dry_controls_t *) user_data;
  
  settings_set_wet_dry_locked (wet_dry->plugin_slot->settings,
                               gtk_toggle_button_get_active (button));
  
  plugin_slot_show_wet_dry_controls (wet_dry->plugin_slot);
}

void
slot_lock_all_cb (GtkToggleButton * button, gpointer user_data)
{
  gboolean on;
  plugin_slot_t * plugin_slot;
  unsigned long i;
  guint lock_copy;
  
  on = gtk_toggle_button_get_active (button);
  
  plugin_slot = (plugin_slot_t *) user_data;
  
  settings_set_lock_all (plugin_slot->settings, on);

  for (i = 0; i < plugin_slot->plugin->desc->control_port_count; i++)
    {
      if (on)
        {
          plugin_slot->port_controls[i].locked = TRUE;
        }
      else
        {
          plugin_slot->port_controls[i].locked =
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin_slot->port_controls[i].lock));
        }
    
    }
  
  lock_copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button), "jack-rack-lock-copy"));
  
  plugin_slot_show_controls (plugin_slot, lock_copy);
  
  g_object_set_data (G_OBJECT(button), "jack-rack-lock-copy", GINT_TO_POINTER (0));
}

void control_lock_cb (GtkToggleButton * button, gpointer user_data) {
  port_controls_t * port_controls;
  
  port_controls = (port_controls_t *) user_data;
  
  port_controls->locked = gtk_toggle_button_get_active (button);
  settings_set_lock (port_controls->plugin_slot->settings, port_controls->control_index, port_controls->locked);

  plugin_slot_show_controls (port_controls->plugin_slot, port_controls->lock_copy);
  
  port_controls->lock_copy = 0;
}

/* lock a specific control with a click+ctrl */
gboolean
control_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  port_controls_t * port_controls;

  port_controls = (port_controls_t *) user_data;
  
  if (port_controls->locked ||
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (port_controls->plugin_slot->lock_all)))
    return FALSE;
  
  if (event->state & GDK_CONTROL_MASK)
    {
      guint lock_copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "jack-rack-plugin-copy"));
      switch (event->button)
        {
        /* lock the control row */
        case 1:
          port_controls->lock_copy = lock_copy;
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->lock), TRUE);
          return TRUE;
          
        /* lock all */
        case 3:
          g_object_set_data (G_OBJECT(port_controls->plugin_slot->lock_all),
                             "jack-rack-lock-copy", GINT_TO_POINTER (lock_copy));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->plugin_slot->lock_all), TRUE);
          return TRUE;
        
        default:
          break;
        }
    }
  
  return FALSE;
}

static void
set_widget_text_colour (GtkWidget * widget, const char * colour_str)
{
  GdkColor colour;
  GdkColormap * colourmap;
  
  colourmap = gtk_widget_get_colormap (widget);
  
  gdk_color_parse (colour_str, &colour);
  gdk_colormap_alloc_color (colourmap, &colour, FALSE, TRUE);
  gtk_widget_modify_text (widget, GTK_STATE_NORMAL, &colour);
}

void control_float_cb (GtkRange * range, gpointer user_data) {
  GtkAdjustment * adjustment;
  port_controls_t * port_controls;
  gchar * str;
  LADSPA_Data value;
  guint copy;
  
  port_controls = (port_controls_t *) user_data;
  
  
  value = gtk_range_get_value (range);
  
  if (port_controls->logarithmic)
    value = exp (value);
  
  str = g_strdup_printf ("%f", value);
  
  /* get which copy we're using from the g object data stuff */
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(range), "jack-rack-plugin-copy"));
    
  /* write to the fifo */
  lff_write (port_controls->controls[copy].control_fifo, &value);
  
  adjustment = gtk_range_get_adjustment (GTK_RANGE (port_controls->controls[copy].control));
    
  /* print the value in the text box */
  gtk_entry_set_text (GTK_ENTRY(port_controls->controls[copy].text), str);
    
  /* store the value */
  settings_set_control_value (port_controls->plugin_slot->settings, copy, port_controls->control_index, value);
              
    
  /* possibly set our peers */
  if (port_controls->plugin_slot->plugin->copies > 1
      && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        {
          if (i != copy)
            gtk_range_set_value (GTK_RANGE(port_controls->controls[i].control),
                                 gtk_range_get_value (range));
        }
    }
    
    
  set_widget_text_colour (port_controls->controls[copy].text, "Black");
  
  g_free (str);
  
}

void control_float_text_cb (GtkEntry * entry, gpointer user_data) {
  port_controls_t * port_controls;
  LADSPA_Data value;
  const char * str;
  char * endptr;
  GtkAdjustment * adjustment;
  guint copy;

  printf ("%s: boo\n", __FUNCTION__);
  
  port_controls = (port_controls_t *) user_data;
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(entry), "jack-rack-plugin-copy"));

  str = gtk_entry_get_text (entry);
  value = strtof (str, &endptr);
  
  /* check for errors */
  adjustment = gtk_range_get_adjustment (GTK_RANGE (port_controls->controls[copy].control));

  if (value == HUGE_VALF ||
      value == -HUGE_VALF ||
      endptr == str ||
      value < adjustment->lower ||
      value > adjustment->upper)
    {
      /* set the entry's text colour */
      set_widget_text_colour (GTK_WIDGET(entry), "DarkRed");
      return;
    }
  
  
  /* set the value in range */
  gtk_range_set_value (GTK_RANGE(port_controls->controls[copy].control),
                       port_controls->logarithmic ? log (value) : value);
    
  /* possibly set our peers */
  /* we shouldn't need to do this as the set_value above will do it */
/*  if (port_controls->plugin_slot->plugin->copies > 1 && port_controls->locked)
    {
      guint i;
      for (i = 0; i < copy; i++)
        gtk_range_set_value (GTK_RANGE(port_controls->controls[i].control),
                             port_controls->logarithmic ? log (value) : value);

      for (i = copy + 1; i < port_controls->plugin_slot->plugin->copies; i++)
        gtk_range_set_value (GTK_RANGE(port_controls->controls[i].control),
                             port_controls->logarithmic ? log (value) : value);
    } */
  
  set_widget_text_colour (GTK_WIDGET(entry), "Black");
}


void control_bool_cb (GtkToggleButton * button, gpointer user_data) {
  port_controls_t * port_controls;
  gboolean on;
  LADSPA_Data data;
  int copy;
  
  port_controls = (port_controls_t *) user_data;
  
  on = gtk_toggle_button_get_active (button);
  data = on ? 1.0 : -1.0;

  /* get which channel we're using from the g object data stuff */
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button), "jack-rack-plugin-copy"));
  
  /* write to the fifo */
  lff_write (port_controls->controls[copy].control_fifo, &data);
  
  /* store value */
  settings_set_control_value (port_controls->plugin_slot->settings, copy, port_controls->control_index, data);
  
  /* possibly set other controls */
  if (port_controls->plugin_slot->plugin->copies > 1 && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        {
          if (i != copy)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(port_controls->controls[i].control), on);
        }
    }
}


void control_int_cb (GtkSpinButton * spinbutton, gpointer user_data) {
  port_controls_t * port_controls;
  int value;
  LADSPA_Data data;
  int copy;
  
  port_controls = (port_controls_t *) user_data;
  
  value = gtk_spin_button_get_value_as_int (spinbutton);
  data = value;
  
  /* get which channel we're using from the g object data stuff */
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(spinbutton), "jack-rack-plugin-copy"));
  
  /* write to the fifo */
  lff_write (port_controls->controls[copy].control_fifo, &data);
  
  /* store value */
  settings_set_control_value (port_controls->plugin_slot->settings, copy, port_controls->control_index, data);
      
  /* possibly set other controls */
  if (port_controls->plugin_slot->plugin->copies > 1 && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        {
          if (i != copy)
            {
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (port_controls->controls[copy].control), value);
            }
        }
    }
}

/** callback for plugin menu buttons (Add and the plugin slots' change buttons) */
gint plugin_button_cb (GtkWidget *widget, GdkEvent *event) {

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton * bevent = (GdkEventButton *) event;
      gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL,
                      bevent->button, bevent->time);
      /* Tell calling code that we have handled this event; the buck
       * stops here. */
      return TRUE;
    }
  
  /* Tell calling code that we have not handled this event; pass it on. */
  return FALSE;
}

#ifdef HAVE_LADCCA
void
deal_with_cca_event (ui_t * ui, cca_event_t * event)
{
  switch (cca_event_get_type (event))
    {
    case CCA_Save_File:
      ui_save_file (ui, cca_get_fqn (cca_event_get_string (event), "jack_rack.rack"));
      cca_send_event (global_cca_client, event);
      break;
    case CCA_Restore_File:
      ui_open_file (ui, cca_get_fqn (cca_event_get_string (event), "jack_rack.rack"));
      cca_send_event (global_cca_client, event);
      break;
    case CCA_Quit:
      quit_cb (NULL, ui);
      cca_event_destroy (event);
      break;
    default:
      fprintf (stderr, "Recieved LADCCA event of unknown type %d\n", cca_event_get_type (event));
      cca_event_destroy (event);
      break;
    }
}
#endif /* HAVE_LADCCA */

static void
ui_check_kicked (ui_t * ui)
{
  static gboolean shown_shutdown_msg = FALSE;
  
  if (ui->shutdown &&
      !shown_shutdown_msg &&
      ui_get_state (ui) != STATE_QUITTING)
    {
      ui_display_error (ui, "%s\n\n%s%s",
                        "JACK client thread shut down by server",
                        "JACK, the bastard, kicked us out.  ",
                        "You'll have to restart I'm afraid.  Sorry.");
    
      gtk_widget_set_sensitive (ui->plugin_box, FALSE);
      gtk_widget_set_sensitive (ui->add, FALSE);

      shown_shutdown_msg = TRUE;
    }
}

/* do the process->gui message processing */
gboolean idle_cb (gpointer data) {
  ctrlmsg_t ctrlmsg;
  ui_t * ui;
  jack_rack_t * jack_rack;
  plugin_t * plugin;
  plugin_slot_t * plugin_slot;
  gboolean enabled;

#ifdef HAVE_LADCCA
  static int ladcca_enabled_at_start = -10;
  
  if (ladcca_enabled_at_start == -10)
    ladcca_enabled_at_start = cca_enabled (global_cca_client);
#endif

  ui = (ui_t *) data;
  jack_rack = ui->jack_rack;
  
  ui_check_kicked (ui);
  
  while (lff_read (ui->process_to_ui, &ctrlmsg) == 0)
    {
    switch (ctrlmsg.type)
      {
      case CTRLMSG_ADD:
        plugin = ctrlmsg.pointer;
        jack_rack_add_plugin_slot (jack_rack, plugin);
        break;

      case CTRLMSG_REMOVE:
        plugin = ctrlmsg.pointer;
        plugin_slot = ctrlmsg.second_pointer;
        jack_rack_remove_plugin_slot (jack_rack, plugin_slot);
        plugin_destroy (plugin, ui->procinfo->jack_client);
        break;

      case CTRLMSG_MOVE:
        plugin_slot = ctrlmsg.second_pointer;
        jack_rack_move_plugin_slot (jack_rack, plugin_slot, GPOINTER_TO_INT (ctrlmsg.pointer));
        ui_set_state (ui, STATE_NORMAL);
        break;

      case CTRLMSG_CHANGE:
        plugin_slot = jack_rack_get_plugin_slot (jack_rack, ctrlmsg.number);
        plugin = ctrlmsg.pointer;
        jack_rack_change_plugin_slot (jack_rack, plugin_slot, ctrlmsg.second_pointer);
        plugin_destroy (plugin, ui->procinfo->jack_client);
        break;

      case CTRLMSG_ABLE:
        plugin_slot = ctrlmsg.second_pointer;
        enabled = GPOINTER_TO_INT(ctrlmsg.pointer);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin_slot->enable),
                                      enabled);
        settings_set_enabled (plugin_slot->settings, enabled);
        break;

      case CTRLMSG_ABLE_WET_DRY:
        plugin_slot = ctrlmsg.second_pointer;
        enabled = GPOINTER_TO_INT(ctrlmsg.pointer);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin_slot->wet_dry),
                                      enabled);
        settings_set_wet_dry_enabled (plugin_slot->settings, enabled);
        plugin_slot_show_wet_dry_controls (plugin_slot);
        break;
      
      case CTRLMSG_CLEAR:
        plugin = ctrlmsg.pointer;
        jack_rack_clear_plugins (jack_rack, plugin);
        break;

      case CTRLMSG_QUIT:
        gtk_main_quit ();
        break;
        
/*      case CTRLMSG_TIME:
        {
          char * time;

          plugin_slot = jack_rack_get_plugin_slot (jack_rack, GPOINTER_TO_UINT (ctrlmsg.pointer));

          time = g_strdup_printf ("Time: %lds:%ldu", ctrlmsg.number, ctrlmsg.second_number);

          gtk_label_set_text (GTK_LABEL (plugin_slot->time), time);

          g_free (time);
          break;
        } */
      }
    }

#ifdef HAVE_LADCCA
  if (ladcca_enabled_at_start && !cca_enabled (global_cca_client))
    {
      /* server disconnected */
      printf ("ladcca server disconnected\n");
      gtk_widget_set_sensitive (global_ui->cca_save, FALSE);
      gtk_widget_set_sensitive (global_ui->cca_save_menu_item, FALSE);
      ladcca_enabled_at_start = 0;
    }
  
  if (ladcca_enabled_at_start && cca_enabled (global_cca_client))
    {
      cca_event_t * event;
      
      while ( (event = cca_get_event (global_cca_client)) )
        {
          deal_with_cca_event (ui, event);
        }
    }
#endif
  
  usleep (1000);
  
  return TRUE;
}

void jack_shutdown_cb (void * data) {
  jack_rack_t * jack_rack;
  
  jack_rack = data;
  
  jack_rack->ui->shutdown = TRUE;
}

