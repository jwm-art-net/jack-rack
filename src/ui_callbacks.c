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
#include "ui_callbacks.h"
#include "control_message.h"
#include "globals.h"
#include "file.h"
#include "ui.h"
#include "process.h"
#include "wet_dry_controls.h"


void
add_cb (GtkMenuItem * menuitem, gpointer user_data)
{
  ui_t * ui;
  plugin_desc_t * desc;
  
  ui = (ui_t *) user_data;
  
  desc = g_object_get_data (G_OBJECT(menuitem), "jack-rack-plugin-desc");
  if (!desc)
    {
      fprintf (stderr, _("%s: no plugin description!\n"), __FUNCTION__);
      return;
    }
  
  jack_rack_send_add_plugin (ui->jack_rack, desc);
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
  
  dialog = gtk_dialog_new_with_buttons (_("I/O Channels"),
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

  warning = gtk_label_new (_("Warning: changing the number of\nchannels will clear the current rack"));
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
  
  ui = (ui_t *) user_data;
  
  jack_rack_send_clear_plugins (ui->jack_rack);
  
  ui_set_filename (ui, NULL);
}

#ifdef HAVE_XML
void
open_cb (GtkButton * button, gpointer user_data)
{
  ui_t * ui = (ui_t *) user_data;
  GtkWidget * dialog;

  dialog = gtk_file_chooser_dialog_new (_("Open File"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char * filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (!ui_open_file (ui, filename))
        ui_set_filename (ui, filename);

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}


void
save_cb (GtkButton * button, gpointer user_data)
{
  ui_t * ui = (ui_t *) user_data;
  
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
  ui_t * ui = (ui_t *) user_data;
  GtkWidget * dialog;

  dialog = gtk_file_chooser_dialog_new (_("Save File"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char * filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (!ui_save_file (ui, filename))
        ui_set_filename (ui, filename);

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}
#endif /* HAVE_XML */
 
#ifdef HAVE_LASH
void
lash_save_cb (GtkButton * button, gpointer user_data)
{
  lash_event_t * event;
  
  event = lash_event_new_with_type (LASH_Save);
  lash_send_event (global_lash_client, event);
}
#endif /* HAVE_LASH */


void
quit_cb (GtkButton * button, gpointer user_data)
{
  ui_t * ui;
  ctrlmsg_t ctrlmsg;
  
  ui = user_data;

  ui_set_state (ui, STATE_QUITTING);
  
  if (ui->shutdown)
    {
      gtk_main_quit ();
      process_info_destroy (ui->procinfo);
      return;
    }
  
  jack_rack_send_clear_plugins (ui->jack_rack);

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
  const char * authors[] = { "Bob Ham <node@users.sourceforge.net>", "Leslie P. Polzer <leslie.polzer@gmx.net>", "Adam Sampson <ats@offog.org>", NULL };
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

  gtk_show_about_dialog (NULL,
                         "authors", authors,
                         "comments", _("A LADSPA effects rack for the JACK audio API"),
                         "copyright", COPYRIGHT_MESSAGE,
                         "logo", logo,
                         "name", PACKAGE_NAME,
                         "translator-credits", _("__TRANSLATOR_CREDITS"),
                         "version", PACKAGE_VERSION,
                         "website", JACK_RACK_URL,
                         NULL);

}
#endif /* HAVE_GNOME */


#ifdef HAVE_ALSA
void
midi_cb (GtkWidget * button, gpointer user_data)
{
  ui_t * ui = user_data;
  gtk_widget_show (ui->midi_window->window);
}
#endif /* HAVE_ALSA */


/** callback for plugin menu buttons ("Add" and the plugin slots' change buttons) */
gint
plugin_button_cb (GtkWidget *widget, GdkEvent *event)
{
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


void
plugin_add_button_cb (GtkWidget *widget, gpointer user_data)
{
  gtk_menu_popup (GTK_MENU (user_data), NULL, NULL, NULL, NULL,
                  0, gtk_get_current_event_time ());
}


#ifdef HAVE_LASH
static int
lash_idle (ui_t * ui, lash_client_t * client)
{
  lash_event_t * event;

  while ( (event = lash_get_event (client)) )
    {
      switch (lash_event_get_type (event))
        {
        case LASH_Save_File:
          ui_save_file (ui, lash_get_fqn (lash_event_get_string (event), "jack_rack.rack"));
          lash_send_event (global_lash_client, event);
          break;
        case LASH_Restore_File:
          ui_open_file (ui, lash_get_fqn (lash_event_get_string (event), "jack_rack.rack"));
          lash_send_event (global_lash_client, event);
          break;
        case LASH_Quit:
          quit_cb (NULL, ui);
          lash_event_destroy (event);
          break;
        case LASH_Server_Lost:
          printf ("server lost\n");
          printf (_("LASH server disconnected\n"));
          gtk_widget_set_sensitive (GTK_WIDGET (ui->lash_save), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET (ui->lash_save_menu_item), FALSE);
          lash_event_destroy (event);
          return 0;
          break;
        default:
          fprintf (stderr, _("Received LASH event of unknown type %d\n"), lash_event_get_type (event));
          lash_event_destroy (event);
          break;
        }
    }

  if (!lash_enabled (client))
    return 0;


  return 1;
}
#endif /* HAVE_LASH */


#ifdef HAVE_ALSA
static void
ui_set_port_value (ui_t *ui, midi_control_t *midi_control, LADSPA_Data value)
{
  if (!midi_control_get_locked (midi_control))
    settings_set_control_value (midi_control->plugin_slot->settings,
                                midi_control->control.ladspa.copy,
                                midi_control->control.ladspa.control,
                                value);
  else
    {
      guint copy;
      for (copy = 0; copy < midi_control->plugin_slot->plugin->copies; copy++)
        settings_set_control_value (midi_control->plugin_slot->settings,
                                    copy,
                                    midi_control->control.ladspa.control,
                                    value);
    }
  
  plugin_slot_set_port_controls (midi_control->plugin_slot,
                                 midi_control->plugin_slot->port_controls +
                                   midi_control->control.ladspa.control,
                                 TRUE);
}

static void
ui_set_wet_dry_value (ui_t *ui, midi_control_t *midi_control, LADSPA_Data value)
{
  if (!midi_control_get_locked (midi_control))
    settings_set_wet_dry_value (midi_control->plugin_slot->settings,
                                midi_control->control.wet_dry.channel,
                                value);
  else
    {
      unsigned long channel;
      for (channel = 0; channel < ui->jack_rack->channels; channel++)
        settings_set_wet_dry_value (midi_control->plugin_slot->settings,
                                    channel,
                                    value);
    }
  
  plugin_slot_set_wet_dry_controls (midi_control->plugin_slot, TRUE);
}

static void
midi_idle (ui_t * ui)
{
  ctrlmsg_t ctrlmsg;
  plugin_slot_t * plugin_slot;

  while (lff_read (ui->midi_to_ui, &ctrlmsg) == 0)
    {
      switch (ctrlmsg.type)
        {
        case CTRLMSG_MIDI_ADD:
          plugin_slot = ctrlmsg.data.midi.midi_control->plugin_slot;
          plugin_slot->midi_controls = g_slist_append (plugin_slot->midi_controls,
                                                       ctrlmsg.data.midi.midi_control);
          midi_window_add_control (ui->midi_window, ctrlmsg.data.midi.midi_control);
          break;
        case CTRLMSG_MIDI_REMOVE:
          midi_window_remove_control (ui->midi_window, ctrlmsg.data.midi.midi_control);
          midi_control_destroy (ctrlmsg.data.midi.midi_control);
          break;
        case CTRLMSG_MIDI_CTRL:
          if (ctrlmsg.data.midi.midi_control->ctrl_type == LADSPA_CONTROL)
            ui_set_port_value (ui, ctrlmsg.data.midi.midi_control, 
			       ctrlmsg.data.midi.value);
          else if(ctrlmsg.data.midi.midi_control->ctrl_type == WET_DRY_CONTROL)
            ui_set_wet_dry_value (ui, ctrlmsg.data.midi.midi_control, 
				  ctrlmsg.data.midi.value);
          break;
	    case CTRLMSG_MIDI_ABLE:
	      gtk_toggle_button_set_active (
		          GTK_TOGGLE_BUTTON(ctrlmsg.data.ablise.plugin_slot->enable),
		          ctrlmsg.data.ablise.enable);
	      break; 
        default:
          break;
        }
    }
}  
#endif /* HAVE_ALSA */


struct reconnect_data
{
        gboolean active;
        GtkWidget* dialog;
        
        ui_t* ui;
};


/**
 * Attempt to reconnect to JACK.
 *
 * @param data Argument of type 'struct reconnect_data'
 * 
 * @return FALSE on success (this tells Glib to delete the timeout)
 */
static gboolean
reconnect_cb ( gpointer data )
{
        struct reconnect_data*  rcdata = (struct reconnect_data*)data;

        process_info_t*         procinfo = rcdata->ui->procinfo;
        
        if ( rcdata->active == FALSE )
                return FALSE;

        /* reconnect */
        procinfo = process_info_new (rcdata->ui, 2);
        
        if (!procinfo)
                /* attempt not successful, maintain callback */
                return TRUE;
        
        /* attempt successful */
        
        gtk_widget_hide (rcdata->dialog);
        rcdata->ui->shutdown = FALSE;

        return FALSE;
}


void
setup_reconnect ( gpointer data )
{
        static GtkWidget* dialog = NULL;
        static int active = TRUE;
        static struct reconnect_data rcdata;
	guint tid;
	gint answer;
	GtkWidget *rcnotice;

        ui_t* ui = (ui_t*)data;
        
        rcdata.active = TRUE;
        rcdata.ui = ui;
        
        if ( dialog == NULL )
              dialog = gtk_dialog_new_with_buttons (
                                _("Connecting to JACK..."),
                                NULL, GTK_DIALOG_MODAL,
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                NULL );
        rcnotice = gtk_label_new (_("Connecting to JACK server..."));
        gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), rcnotice);
        gtk_container_set_border_width (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 50);
        gtk_widget_set_size_request (GTK_DIALOG(dialog)->vbox, 250, 100); 
        gtk_widget_show_all (GTK_DIALOG(dialog)->vbox);
        rcdata.dialog = dialog;
        
        tid = g_timeout_add (1000, &reconnect_cb, (gpointer)&rcdata);
        
        answer = gtk_dialog_run (GTK_DIALOG(dialog));
        
        if (answer == GTK_RESPONSE_CANCEL)
        {
                /* FIXME show notice */
                rcdata.active = FALSE;

                if (ui->main_window == NULL)
                {
                        ui_set_state (ui, STATE_QUITTING);
                        return;
                }
                
                gtk_widget_set_sensitive (ui->plugin_box, FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (ui->add), FALSE);
                gtk_widget_set_sensitive (ui->add_menuitem, FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (ui->channels), FALSE);
                gtk_widget_set_sensitive (ui->channels_menuitem, FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (ui->new), FALSE);
                gtk_widget_set_sensitive (ui->new_menuitem, FALSE);
#ifdef HAVE_ALSA
                gtk_widget_set_sensitive (ui->midi_menuitem, FALSE);
                gtk_widget_set_sensitive (ui->midi_window->main_box, FALSE);
#endif
#ifdef HAVE_XML
                gtk_widget_set_sensitive (GTK_WIDGET (ui->open), FALSE);
                gtk_widget_set_sensitive (ui->open_menuitem, FALSE);
#endif
        }
        
        active = FALSE;
        gtk_widget_hide (dialog);
}


static void
ui_check_kicked (ui_t * ui)
{
        static gboolean shown_shutdown_msg = FALSE;
  
        if (ui->shutdown && !shown_shutdown_msg
            && ui_get_state (ui) != STATE_QUITTING)
        {
                setup_reconnect (ui);
                shown_shutdown_msg = TRUE;
        }

        return;
}


/* do the process->gui message processing */
gboolean
idle_cb (gpointer data)
{
  ctrlmsg_t ctrlmsg;
  ui_t * ui;
  jack_rack_t * jack_rack;
#ifdef HAVE_LASH
  static int call_lash_idle = 1;
#endif /* HAVE_LASH */

  ui = (ui_t *) data;
  jack_rack = ui->jack_rack;
  
  ui_check_kicked (ui);

#ifdef HAVE_ALSA
  if (!ui->shutdown)
    midi_idle (ui);
#endif
  
  while (lff_read (ui->process_to_ui, &ctrlmsg) == 0)
    {
    switch (ctrlmsg.type)
      {
      case CTRLMSG_ADD:
        jack_rack_add_plugin (jack_rack, ctrlmsg.data.add.plugin);
        break;

      case CTRLMSG_REMOVE:
        jack_rack_remove_plugin_slot (jack_rack, ctrlmsg.data.remove.plugin_slot);
        plugin_destroy (ctrlmsg.data.remove.plugin, ui);
        break;

      case CTRLMSG_MOVE:
        jack_rack_move_plugin_slot (jack_rack,
                                    ctrlmsg.data.move.plugin_slot,
                                    ctrlmsg.data.move.up);
        ui_set_state (ui, STATE_NORMAL);
        break;

      case CTRLMSG_CHANGE:
        jack_rack_change_plugin_slot (jack_rack,
                                      ctrlmsg.data.change.plugin_slot,
                                      ctrlmsg.data.change.new_plugin);
        plugin_destroy (ctrlmsg.data.change.old_plugin, ui);
        break;

      case CTRLMSG_ABLE:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ctrlmsg.data.ablise.plugin_slot->enable),
                                      ctrlmsg.data.ablise.enable);
        settings_set_enabled (ctrlmsg.data.ablise.plugin_slot->settings,
                              ctrlmsg.data.ablise.enable);
        break;

      case CTRLMSG_ABLE_WET_DRY:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ctrlmsg.data.ablise.plugin_slot->wet_dry),
                                      ctrlmsg.data.ablise.enable);
        settings_set_wet_dry_enabled (ctrlmsg.data.ablise.plugin_slot->settings,
                                      ctrlmsg.data.ablise.enable);
        plugin_slot_show_wet_dry_controls (ctrlmsg.data.ablise.plugin_slot);
        break;
      
/*      case CTRLMSG_CLEAR:
        jack_rack_clear_plugins (jack_rack, ctrlmsg.data.clear.chain);
        break; */

      case CTRLMSG_QUIT:
        gtk_main_quit ();
        process_info_destroy (ui->procinfo);
        break;
        
      default:
        break;
      }
    }

#ifdef HAVE_LASH
  if (call_lash_idle)
    call_lash_idle = lash_idle (ui, global_lash_client);
#endif
  
  usleep (1000);
  
  return TRUE;
}

void
jack_shutdown_cb (void * data)
{
  ui_t * ui = (ui_t*)data;
  
  ui->shutdown = TRUE;
}

#if HAVE_JACK_SESSION 

static gboolean
jack_session_cb (gpointer data)
{
  ui_t * ui = (ui_t*)data;
  char cmd_buf[256];
  char fname_buf[256];
  jack_session_event_type_t ev_type = ui->js_event->type;

  snprintf( fname_buf, sizeof(fname_buf), "%srack.xml", ui->js_event->session_dir);
  snprintf( cmd_buf, sizeof(cmd_buf), "jack-rack --jack-session-uuid %s \"%s\"", ui->js_event->client_uuid, fname_buf );

  ui->js_event->command_line = strdup( cmd_buf );

  if (!ui_save_file (ui, fname_buf))
    ui->js_event->flags = JackSessionSaveError;

  jack_session_reply( ui->procinfo->jack_client, ui->js_event );

  if (ev_type == JackSessionSaveAndQuit)
    ui->shutdown = TRUE;

  return FALSE;
}

void 
jack_session_cb_aux (jack_session_event_t *ev, void *data)
{
  ui_t * ui = (ui_t*)data;
  ui->js_event = ev;
  g_idle_add (jack_session_cb, data);
}

#endif

