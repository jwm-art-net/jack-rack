nlsdir	= @NLSDIR@

EXTRA_DIST = ru.po
PO_FILES = ru.po
MO_FILES = ru.mo

POT_FILES = \
	$(top_builddir)/src/control_callbacks.c \
	$(top_builddir)/src/file.c \
	$(top_builddir)/src/jack_rack.c \
	$(top_builddir)/src/lock_free_fifo.c \
	$(top_builddir)/src/main.c \
	$(top_builddir)/src/midi.c \
	$(top_builddir)/src/midi_control.c \
	$(top_builddir)/src/midi_window.c \
	$(top_builddir)/src/plugin.c \
	$(top_builddir)/src/plugin_desc.c \
	$(top_builddir)/src/plugin_mgr.c \
	$(top_builddir)/src/plugin_settings.c \
	$(top_builddir)/src/plugin_slot.c \
	$(top_builddir)/src/plugin_slot_callbacks.c \
	$(top_builddir)/src/port_controls.c \
	$(top_builddir)/src/process.c \
	$(top_builddir)/src/ui.c \
	$(top_builddir)/src/ui_callbacks.c \
	$(top_builddir)/src/wet_dry_controls.c


SUFFIXES = .po .mo

.po.mo: 
	msgfmt -o $@ $<

all-local: $(MO_FILES)

jack-rack.pot: $(POTFILES)
	xgettext --default-domain jack-rack \
		--add-comments --keyword=_ \
		--keyword=N_ $(POT_FILES) \
		--output=$@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(nlsdir)
	chmod 755 $(DESTDIR)$(nlsdir)
	for n in $(MO_FILES) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			chmod 755 $(DESTDIR)$(nlsdir)/$$l; \
			chmod 755 $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/jack-rack.mo; \
		fi; \
	done
