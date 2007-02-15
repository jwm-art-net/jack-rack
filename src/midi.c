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

#include "ac_config.h"

#ifdef HAVE_ALSA

#define _GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include "globals.h"
#include "midi.h"
#include "ui.h"
#include "control_message.h"

static void * midi_run (void * data);


static snd_seq_t *
midi_info_connect_alsa (ui_t * ui)
{
  snd_seq_t * seq;
  int err;
  
  ui_display_splash_text (ui, _("Connecting to ALSA sequencer"));
  
  /* connect to the sequencer */
  err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
  if (err)
      ui_display_error (ui, E_FATAL,
                            _("Could not open ALSA sequencer, aborting:\n\n%s\n\nPlease check that your ALSA configuration is correct."),
                            snd_strerror (err));
  
  ui_display_splash_text (ui, _("Connected to ALSA sequencer with id %d"), snd_seq_client_id (seq));


  
  
  /* tell ladcca our client id */
#ifdef HAVE_LADCCA
  cca_alsa_client_id (global_cca_client, snd_seq_client_id (seq));
#endif /* HAVE_LADCCA */

  /* set our client name */
  snd_seq_set_client_name (seq, client_name->str);
  
  
  
  
  ui_display_splash_text (ui, _("Creating ALSA sequencer port"));
  
  /* create a port */
  err = snd_seq_create_simple_port (seq, "Control",
                                    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_DUPLEX|
                                    SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                    SND_SEQ_PORT_TYPE_APPLICATION|SND_SEQ_PORT_TYPE_SPECIFIC);
  if (err)
      ui_display_error (ui, E_FATAL, _("Could not create ALSA port, aborting:\n\n%s"), snd_strerror (err));

  ui_display_splash_text (ui, _("Created ALSA sequencer port"));
  
  return seq;
}


midi_info_t *
midi_info_new (struct _ui * ui)
{
  midi_info_t * minfo;
  
  minfo = g_malloc (sizeof (midi_info_t));
  
  minfo->ui            = ui;
  minfo->seq           = midi_info_connect_alsa (ui);
  minfo->midi_to_ui    = ui->midi_to_ui;
  minfo->ui_to_midi    = ui->ui_to_midi;
  minfo->midi_controls = NULL;
  minfo->quit          = FALSE;
  
  pthread_create (&minfo->thread, NULL, midi_run, minfo);
  
  return minfo;  
}


void
midi_info_destroy (midi_info_t * minfo)
{
  minfo->quit = TRUE;
  
  pthread_join (minfo->thread, NULL);
  
  g_free (minfo);
}


static void
midi_send_control (midi_info_t *minfo, midi_control_t *midi_ctrl, LADSPA_Data value)
{
  snd_seq_event_t event;
  signed int midi_value;
  int err;
  
  midi_value = 128 / ((midi_ctrl->max - midi_ctrl->min) / (value - midi_ctrl->min));
    
  snd_seq_ev_clear      (&event);
  snd_seq_ev_set_direct (&event);
  snd_seq_ev_set_subs   (&event);
  snd_seq_ev_set_source (&event, 0);
  snd_seq_ev_set_controller (&event,
                             midi_control_get_midi_channel (midi_ctrl),
                             midi_control_get_midi_param (midi_ctrl),
                             midi_value);

  err = snd_seq_event_output (minfo->seq, &event);
  if (err < 0)
    {
      fprintf (stderr, _("%s: error sending MIDI event: %s\n"),
               __FUNCTION__, snd_strerror (err));
    }
  
  snd_seq_drain_output (minfo->seq);
}


static void
midi_process_ctrlmsgs (midi_info_t *minfo)
{
  ctrlmsg_t ctrlmsg;
  
  while (lff_read (minfo->ui_to_midi, &ctrlmsg) == 0)
    {
      switch (ctrlmsg.type)
        {
        case CTRLMSG_MIDI_ADD:
          minfo->midi_controls = g_slist_append (minfo->midi_controls,
                                                 ctrlmsg.data.midi.midi_control);
          lff_write (minfo->midi_to_ui, &ctrlmsg);
          break;
        case CTRLMSG_MIDI_REMOVE:
          minfo->midi_controls = g_slist_remove (minfo->midi_controls,
                                                 ctrlmsg.data.midi.midi_control);
          lff_write (minfo->midi_to_ui, &ctrlmsg);
          break;
        case CTRLMSG_MIDI_CTRL:
	    case CTRLMSG_MIDI_ABLE:
          midi_send_control (minfo, ctrlmsg.data.midi.midi_control, ctrlmsg.data.midi.value);
          break;
        case CTRLMSG_QUIT:
          minfo->quit = TRUE;
          break;
        default:
          break;
        }
    }
}

static void
midi_control (midi_info_t *minfo, snd_seq_ev_ctrl_t *event, 
	      snd_seq_event_type_t type)
{
  GSList *list;
  midi_control_t *midi_ctrl;
  LADSPA_Data value;
  ctrlmsg_t ctrlmsg;

  for (list = minfo->midi_controls; list; list = g_slist_next (list))
    {
      midi_ctrl = (midi_control_t *) list->data;
      
      int param = midi_control_get_midi_param (midi_ctrl);
      int channel = midi_control_get_midi_channel (midi_ctrl);
      
      /* Note: this might seem a bit swilly, but it was useful for me to 
         be able to use program change events to toggle the enabled state.
         (my midi controller only had 12 controller change triggers, but 120
         program change triggers...)  Might be useful for others too....
         Hopefully, not too confusing though. */
      if ( ( type == SND_SEQ_EVENT_CONTROLLER &&
	     event->param == param &&
	     event->channel+1 == channel )  /* control change */
	   ||
	     ( type == SND_SEQ_EVENT_PGMCHANGE &&
	     event->value == param ) )  /* prog change stores our 'param' in 
	                                   'value'. ignore channel. */
        {
          value = midi_ctrl->min + ((midi_ctrl->max - midi_ctrl->min) / 127.0) * event->value;
	            
          /* send the value to the process callback */
	  if(midi_ctrl->ctrl_type != PLUGIN_ENABLE_CONTROL)
	    {
	      if (!midi_control_get_locked (midi_ctrl))
		    lff_write (midi_ctrl->fifo, &value);
	      else
		    {
		      unsigned long i;
		      unsigned long limit = midi_ctrl->ctrl_type == LADSPA_CONTROL
		           ? midi_ctrl->plugin_slot->plugin->desc->control_port_count
		           : midi_ctrl->plugin_slot->jack_rack->channels;
		  
		      for (i = 0; i < limit; i++)
		        lff_write (midi_ctrl->fifos + i, &value);
		    }
	    
	      /* FIXME: need to send to all copies
	       * number of channels is in minfo->ui->jack_rack->channels
	       * (midi_control.midi_channel)
	       */

          /* send the value to the ui */
	      ctrlmsg.type = CTRLMSG_MIDI_CTRL;
	      ctrlmsg.data.midi.midi_control = midi_ctrl;
	      ctrlmsg.data.midi.value = value;
	      lff_write (minfo->midi_to_ui, &ctrlmsg);
	    }
	  else 
	    {
	      /* This is an 'Enable' midi message */
	      plugin_t * plugin;
	      
	      /* TODO: add fifo */
	      plugin = midi_ctrl->plugin_slot->plugin;
	      
	      /* For control change, off = 0, on = 127.
	         For prog change, just flip the state */
	      if(type == SND_SEQ_EVENT_CONTROLLER)
		    {
		      if(event->value == 0)
		        plugin->enabled = FALSE;
		      else if(event->value == 127)
		        plugin->enabled = TRUE;
		      else
		        return;
		    }
	      else if(type == SND_SEQ_EVENT_PGMCHANGE)
		    {
		      gboolean wasEnabled = plugin->enabled;	      
		      plugin->enabled = !wasEnabled;
		    }

	      ctrlmsg.type =  CTRLMSG_MIDI_ABLE;
	      ctrlmsg.data.ablise.plugin = plugin;
	      ctrlmsg.data.ablise.enable = plugin->enabled;
	      ctrlmsg.data.ablise.plugin_slot = midi_ctrl->plugin_slot;
	      lff_write (minfo->midi_to_ui, &ctrlmsg);	      
	    }
    }
    }
}


/**
 * Check for new events in the sequencer's event queue
 */
static void
midi_get_events (midi_info_t *minfo)
{
  snd_seq_event_t *event;
  
  do
    { 
      snd_seq_event_input (minfo->seq, &event);
      switch (event->type)
        {
        case SND_SEQ_EVENT_CONTROLLER:
	    case SND_SEQ_EVENT_PGMCHANGE: 
          midi_control (minfo, &event->data.control, event->type);
          break;
        default:
          break;
        }
    }
  while (snd_seq_event_input_pending (minfo->seq, 0) > 0);
}


/**
 * Poll the sequencer's file descriptors
 */
static void
midi_process (midi_info_t *minfo)
{
  int err;
  int i;
  
  err = poll (minfo->pfds, minfo->seq_nfds, 300);
  if (err > 0)
    for (i = 0; i < minfo->seq_nfds; i++)
      if (minfo->pfds[i].revents > 0)
        midi_get_events (minfo);
}


static void
midi_realise_time (midi_info_t *minfo)
{
  pthread_t jack_thread;
  struct sched_param rt_param;
  gboolean set_priority = FALSE;
  int policy;
  int err;
   
  memset(&rt_param, 0, sizeof(rt_param));
  
  jack_thread = jack_client_thread_id (minfo->ui->procinfo->jack_client);
  err = pthread_getschedparam (jack_thread, &policy, &rt_param);
  if (err)
    fprintf (stderr, _("%s: could not get scheduling parameters of JACK thread: %s\n"),
             __FUNCTION__, strerror (err));
  else
    {
      rt_param.sched_priority += 1;
      set_priority = TRUE;
    }
                                                                                                               
  /* make a guess; jackd runs at priority 10 by default I think */
  if (!set_priority)
    {
      memset(&rt_param, 0, sizeof(rt_param));
      rt_param.sched_priority = 15;
    }
                                                                                                               
  policy = SCHED_FIFO;
                                                                                                               
  err = pthread_setschedparam (pthread_self (), policy, &rt_param);
  if (err)
    fprintf (stderr, _("%s: could not set SCHED_FIFO for MIDI thread: %s\n"),
             __FUNCTION__, strerror (errno));
}


static void *
midi_run (void * data)
{
  midi_info_t * minfo = (midi_info_t *) data;
  
  /* this is dealt with by the JACK thread */
  signal (SIGHUP, SIG_IGN);
  
  midi_realise_time (minfo);
  
  minfo->seq_nfds = snd_seq_poll_descriptors_count(minfo->seq, POLLIN);
  minfo->pfds     = g_malloc (sizeof(struct pollfd) * minfo->seq_nfds);
  snd_seq_poll_descriptors(minfo->seq, minfo->pfds, minfo->seq_nfds, POLLIN);
  
  while (!minfo->quit)
    {
      midi_process (minfo);
      midi_process_ctrlmsgs (minfo);
    }
  
  g_free (minfo->pfds);
  
  return NULL;
}

#endif /* HAVE_ALSA */


