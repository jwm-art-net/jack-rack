pixmapdir = $(datadir)/pixmaps

bin_PROGRAMS = jack-rack

jack_rack_SOURCES = \
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
	callbacks.c callbacks.h \
	file.c file.h \
	control_message.h \
	main.c \
	globals.h

jack_rack_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GNOMEUI_CFLAGS) \
	$(JACK_CFLAGS) \
	$(LADCCA_CFLAGS) \
	$(LRDF_CFLAGS) \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPIXMAPDIR=\"$(pixmapdir)\" \
	-I$(top_srcdir)/range-widget

#	-DG_DISABLE_DEPRECATED=1 \
#	-DGTK_DISABLE_DEPRECATED=1 \
#	-DGDK_DISABLE_DEPRECATED=1 \
#	-DGDK_PIXBUF_DISABLE_DEPRECATED=1 \
#	-DGNOME_DISABLE_DEPRECATED=1 \
#

jack_rack_LDFLAGS = \
	$(JACK_LIBS) \
	$(GTK_LIBS) \
	$(GNOMEUI_LIBS) \
	$(LADCCA_LIBS) \
	$(LRDF_LIBS)

splint-check:
	splint  +systemdirs /usr/include:/usr/local/lib \
		-sys-dir-errors \
		-systemdirerrors \
		+posixlib \
		$(jack_rack_CFLAGS) \
		*.c
