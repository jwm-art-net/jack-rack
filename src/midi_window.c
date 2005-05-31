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

#ifdef HAVE_ALSA

#include "globals.h"
#include "midi_window.h"
#include "ui.h"
#include "control_message.h"


static void ok_cb      (GtkButton *button, gpointer user_data);
static void remove_cb  (GtkButton *button, gpointer user_data);
static void channel_cb (GtkCellRendererText *channel_renderer, gchar *arg1, gchar *arg2, gpointer user_data);
static void param_cb   (GtkCellRendererText *param_renderer, gchar *arg1, gchar *arg2, gpointer user_data);


static void
midi_window_create_control_view (midi_window_t *mwin)
{
  GtkWidget *scroll;
  GtkCellRenderer *renderer;
  GtkCellRenderer *channel_renderer;
  GtkCellRenderer *param_renderer;
  GtkTreeViewColumn *column;

  /* scroll window for the list */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_box_pack_start (GTK_BOX (mwin->main_box), scroll, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
                                                                                                               
  /* the store */
  mwin->controls = gtk_list_store_new (N_COLUMNS,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_INT,
                                       G_TYPE_INT,
                                       G_TYPE_INT,
                                       G_TYPE_POINTER);
  /* the view */
  mwin->controls_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mwin->controls));
  g_object_unref (G_OBJECT (mwin->controls));
  gtk_widget_show (mwin->controls_view);
  gtk_container_add (GTK_CONTAINER(scroll), mwin->controls_view);
  
  /* plugin column */
  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes (
               _("Plugin"), renderer, "text", PLUGIN_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mwin->controls_view), column);

  /* control column */
  column   = gtk_tree_view_column_new_with_attributes (
               _("Control"), renderer, "text", CONTROL_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mwin->controls_view), column);

  /* index column */
  column   = gtk_tree_view_column_new_with_attributes (
               _("Index"), renderer, "text", INDEX_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mwin->controls_view), column);


  /* channel column */
  channel_renderer = gtk_cell_renderer_text_new ();
  g_object_set (channel_renderer, "editable", TRUE, NULL);
  g_signal_connect (G_OBJECT (channel_renderer), "edited",
                    G_CALLBACK (channel_cb), mwin);
  
  column   = gtk_tree_view_column_new_with_attributes (
               _("MIDI Channel"), channel_renderer, "text", CHANNEL_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mwin->controls_view), column);

  /* param column */
  param_renderer = gtk_cell_renderer_text_new ();
  g_object_set (param_renderer, "editable", TRUE, NULL);
  g_signal_connect (G_OBJECT (param_renderer), "edited",
                    G_CALLBACK (param_cb), mwin);
  
  column = gtk_tree_view_column_new_with_attributes (
             _("MIDI Controller"), param_renderer, "text", PARAM_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mwin->controls_view), column);

}


midi_window_t *
midi_window_new     (ui_t *ui)
{
  midi_window_t *mwin;
  GtkWidget *button_box;
  GtkWidget *ok;
  GtkWidget *remove;
  
  mwin = g_malloc0 (sizeof (midi_window_t));
  
  mwin->ui   = ui;
  
  /* main window */
  mwin->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size        (GTK_WINDOW (mwin->window), 400, 200);
  gtk_window_set_title               (GTK_WINDOW (mwin->window), _("JACK Rack MIDI Controls"));
  gtk_window_set_policy              (GTK_WINDOW (mwin->window), FALSE, TRUE, FALSE);
  gtk_window_set_transient_for       (GTK_WINDOW (mwin->window), GTK_WINDOW (ui->main_window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (mwin->window), TRUE);
  g_signal_connect (G_OBJECT (mwin->window), "delete_event",
                    G_CALLBACK (gtk_widget_hide_on_delete), mwin->window);
  
  /* main box */
  mwin->main_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (mwin->main_box);
  gtk_container_add (GTK_CONTAINER (mwin->window), mwin->main_box);
  
  
  
  /*
   * buttons
   */
  button_box = gtk_hbutton_box_new ();
  gtk_widget_show (button_box);
  gtk_box_pack_end (GTK_BOX (mwin->main_box), button_box, FALSE, TRUE, 0);
  
  /* remove_button */
  remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
  gtk_widget_show (remove);
  g_signal_connect (G_OBJECT (remove), "clicked",
                    G_CALLBACK (remove_cb), mwin);
  gtk_box_pack_start (GTK_BOX (button_box), remove, FALSE, TRUE, 0);
  
  /* ok button */
  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_show (ok);
  g_signal_connect (G_OBJECT (ok), "clicked",
                    G_CALLBACK (ok_cb), mwin);
  gtk_box_pack_start (GTK_BOX (button_box), ok, FALSE, TRUE, 0);


  
  /*
   * midi controls
   */
  midi_window_create_control_view (mwin);
  
  return mwin;
}

void
midi_window_destroy (midi_window_t * mwin)
{
  g_free (mwin);
}

void
midi_window_remove_control (midi_window_t *mwin, midi_control_t *midi_ctrl)
{
  GtkTreeIter iter;
  gboolean succ;
  midi_control_t *list_mctrl;
  
  succ = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (mwin->controls), &iter);
  if (!succ)
    return;
  
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (mwin->controls), &iter,
                          MIDI_CONTROL_POINTER, &list_mctrl,
                          -1);
      
      if (list_mctrl == midi_ctrl)
        {
          gtk_list_store_remove (mwin->controls, &iter);
          return;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (mwin->controls), &iter));
}


void
midi_window_add_control (midi_window_t *mwin, midi_control_t *midi_ctrl)
{
  GtkTreeIter iter;
  
  gtk_list_store_append (mwin->controls, &iter);
  
  gtk_list_store_set (
    mwin->controls, &iter,
    PLUGIN_COLUMN, midi_ctrl->plugin_slot->plugin->desc->name,
    CONTROL_COLUMN, midi_control_get_control_name (midi_ctrl),
    INDEX_COLUMN, (midi_ctrl->ladspa_control
                    ? midi_ctrl->control.ladspa.copy
                    : midi_ctrl->control.wet_dry.channel) + 1,
    CHANNEL_COLUMN, midi_control_get_midi_channel (midi_ctrl),
    PARAM_COLUMN, midi_control_get_midi_param (midi_ctrl),
    MIDI_CONTROL_POINTER, midi_ctrl,
    -1);
  
  gtk_widget_show (mwin->window);
}


static void
ok_cb (GtkButton *button, gpointer user_data)
{
  midi_window_t *mwin = user_data;
  
  gtk_widget_hide (mwin->window);
}

static gboolean
get_selected_iter (midi_window_t *mwin, GtkTreeIter * iter)
{
  GtkTreeSelection * selection;
  GtkTreeModel * model;
 
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mwin->controls_view));
   
  return gtk_tree_selection_get_selected (selection, &model, iter);
}


static void
remove_cb (GtkButton *button, gpointer user_data)
{
  midi_window_t *mwin = user_data;
  midi_control_t *midi_ctrl;
  ctrlmsg_t ctrlmsg;
  
  GtkTreeIter iter;

  if (!get_selected_iter (mwin, &iter))
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (mwin->controls), &iter,
                      MIDI_CONTROL_POINTER, &midi_ctrl,
                      -1);
  
  ctrlmsg.type = CTRLMSG_MIDI_REMOVE;
  ctrlmsg.data.midi.midi_control = midi_ctrl;
  lff_write (midi_ctrl->plugin_slot->jack_rack->ui->ui_to_midi, &ctrlmsg);
  
  midi_ctrl->plugin_slot->midi_controls = 
    g_slist_remove (midi_ctrl->plugin_slot->midi_controls, midi_ctrl);
}

static void
channel_cb (GtkCellRendererText *cell,
            gchar *path_string,
            gchar *new_text,
            gpointer user_data)
{
  midi_window_t *mwin = user_data;
  midi_control_t *midi_ctrl;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  unsigned char channel;
  
  gtk_tree_model_get_iter (GTK_TREE_MODEL (mwin->controls), &iter, path);
  
  channel = atof (new_text);
  if (channel < 1 || channel > 16)
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (mwin->controls), &iter,
                      MIDI_CONTROL_POINTER, &midi_ctrl,
                      -1);
  midi_control_set_midi_channel (midi_ctrl, channel);
  
  gtk_list_store_set (mwin->controls, &iter,
                      CHANNEL_COLUMN, (int) channel,
                      -1);
}

static void
param_cb (GtkCellRendererText *cell,
            gchar *path_string,
            gchar *new_text,
            gpointer user_data)
{
  midi_window_t *mwin = user_data;
  midi_control_t *midi_ctrl;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  unsigned int param;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (mwin->controls), &iter, path);
  
  param = atof (new_text);
  if (param < 1 || param > 128)
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (mwin->controls), &iter,
                      MIDI_CONTROL_POINTER, &midi_ctrl,
                      -1);
  midi_control_set_midi_param (midi_ctrl, param);
  
  gtk_list_store_set (mwin->controls, &iter,
                      PARAM_COLUMN, (int) param,
                      -1);
}



#endif /* HAVE_ALSA */
