pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = jack_rack_1.0.dtd jack_rack_1.1.dtd

dtddir = $(datadir)/dtds
dtd_DATA = jack_rack_1.0.dtd jack_rack_1.1.dtd 

bin_PROGRAMS = jack-rack

jack_rack_SOURCES = \
	midi_window.c midi_window.h \
	midi_control.c midi_control.h \
	wet_dry_controls.c wet_dry_controls.h \
	file.c file.h \
	midi.c midi.h \
	plugin_settings.c plugin_settings.h \
	process.c process.h \
	plugin_desc.c plugin_desc.h \
	plugin_mgr.c plugin_mgr.h \
	ui.c ui.h \
	plugin.c plugin.h \
	port_controls.c port_controls.h \
	plugin_slot.c plugin_slot.h \
	jack_rack.h jack_rack.c \
	lock_free_fifo.c lock_free_fifo.h \
	ui_callbacks.c ui_callbacks.h \
	plugin_slot_callbacks.c plugin_slot_callbacks.h \
	control_callbacks.c control_callbacks.h \
	control_message.h \
	main.c \
	globals.h

jack_rack_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GNOMEUI_CFLAGS) \
	$(JACK_CFLAGS) \
	$(LADCCA_CFLAGS) \
	$(LRDF_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(XML_CFLAGS) \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPIXMAPDIR=\"$(pixmapdir)\" \
	-DDTDDIR=\"$(dtddir)\" \
	-DG_DISABLE_DEPRECATED=1 \
	-DGTK_DISABLE_DEPRECATED=1 \
	-DGDK_DISABLE_DEPRECATED=1 \
	-DGDK_PIXBUF_DISABLE_DEPRECATED=1 \
	-DGNOME_DISABLE_DEPRECATED=1


jack_rack_LDFLAGS = \
	$(JACK_LIBS) \
	$(GTK_LIBS) \
	$(GNOMEUI_LIBS) \
	$(LADCCA_LIBS) \
	$(LRDF_LIBS) \
	$(ALSA_LIBS) \
	$(XML_LIBS)

splint-check:
	splint  +systemdirs /usr/include:/usr/local/lib \
		-sys-dir-errors \
		-systemdirerrors \
		+posixlib \
		$(jack_rack_CFLAGS) \
		*.c
