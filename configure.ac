AC_INIT([JACK Rack], 1.4.8rc1)
AC_CONFIG_SRCDIR(src/lock_free_fifo.h)
AM_INIT_AUTOMAKE(1.6)
AM_CONFIG_HEADER(src/ac_config.h)

AC_PROG_CC
AM_PROG_CC_C_O

#AC_PROG_LIBTOOL


############
### JACK ###
############
PKG_CHECK_MODULES(JACK, jack >= 0.50.0, true,
                  AC_MSG_ERROR([you need jack >= 0.50.0  see http://sf.net/projects/jackit]))
JACK_VERSION=$( pkg-config --modversion jack )
AC_DEFINE_UNQUOTED(JACK_VERSION, "$JACK_VERSION", [The version of jack we're compiling against])
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)

AC_CHECK_HEADER(jack/session.h, JACK_SESSION_FOUND="yes", JACK_SESSION_FOUND="no")
if test "$JACK_SESSION_FOUND" = "yes"; then
	AC_DEFINE(HAVE_JACK_SESSION, 1, [whether jack/session.h is available])
fi


##############
### LADSPA ###
##############

AC_CHECK_HEADER(ladspa.h, true, AC_MSG_ERROR([LADSPA is required  see http://www.ladspa.org/]))


##############
### GTK+ 2 ###
##############
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12.0, true,
                  AC_MSG_ERROR([you need gtk+ >= 2.12.0  see http://www.gtk.org/]))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)


###############
### libxml2 ###
###############
PKG_CHECK_MODULES(XML, libxml-2.0, true, AC_MSG_ERROR([libxml2 is required see http://www.xmlsoft.org/]))

XML_VERSION=$( pkg-config --modversion libxml-2.0 )
AC_DEFINE_UNQUOTED(XML_VERSION, "$XML_VERSION", [The version of libxml2 we're compiling against])
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)


############
### ALSA ###
############
jr_enable_aseq="yes"
AC_ARG_ENABLE(aseq,
[  --disable-aseq          disable ALSA sequencer support],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      jr_enable_aseq="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-aseq(=yes/no) or --disable-aseq])
      ;;
  esac
])

if test "$jr_enable_aseq" = "yes"; then
  PKG_CHECK_MODULES(ALSA, alsa >= 0.9, ALSA_FOUND="yes", ALSA_FOUND="no")
  AC_SUBST(ALSA_CFLAGS)
  AC_SUBST(ALSA_LIBS)

  if test "$ALSA_FOUND" = "yes"; then
    AC_DEFINE(HAVE_ALSA, 1, [whether or not we are supporting the alsa sequencer])
    ALSA_VERSION=$( pkg-config --modversion alsa )
    AC_DEFINE_UNQUOTED(ALSA_VERSION, "$ALSA_VERSION", [The version of alsa we're compiling against])
  fi
else
  ALSA_FOUND="no"
fi
AM_CONDITIONAL(HAVE_ALSA, test "$ALSA_FOUND" = "yes")

###############
### GNOME 2 ###
###############
jr_enable_gnome="no" # does not build
AC_ARG_ENABLE(gnome,
[  --disable-gnome         disable GNOME 2 support (default: disabled)],[
  case "$enableval" in
    "yes")
      jr_enable_gnome="yes"
      ;;
    "no")
      jr_enable_gnome="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-gnome(=yes/no) or --disable-gnome])
      ;;
  esac
])

if test "$jr_enable_gnome" = "yes"; then
  PKG_CHECK_MODULES(GNOMEUI, libgnomeui-2.0, GNOME_FOUND="yes", GNOME_FOUND="no")

  if test "$GNOME_FOUND" = "yes"; then
    AC_DEFINE(HAVE_GNOME, 1, [whether or not we are supporting gnome])
    GNOME_VERSION=$( pkg-config --modversion libgnomeui-2.0 )
    AC_DEFINE_UNQUOTED(GNOME_VERSION, "$GNOME_VERSION", [The version of gnome we're compiling against])
    AC_SUBST(GNOMEUI_CFLAGS)
    AC_SUBST(GNOMEUI_LIBS)
  fi
else
  GNOME_FOUND="no"
fi
AM_CONDITIONAL(HAVE_GNOME, test "$GNOME_FOUND" = "yes")


##############
### LASH ###
##############
if test x$XML_FOUND = xyes; then
  jr_enable_lash="no"
  AC_ARG_ENABLE(lash,
  [  --disable-lash          disable LASH support (default: disabled)],[
    case "$enableval" in
      "yes")
        jr_enable_lash="yes"
        ;;
      "no")
        jr_enable_lash="no"
        ;;
      *)
        AC_MSG_ERROR([must use --enable-lash(=yes/no) or --disable-lash])
        ;;
    esac
  ])

  if test "$jr_enable_lash" = "yes"; then
    PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.0, LASH_FOUND="yes", LASH_FOUND="no")
  
    if test "$LASH_FOUND" = "yes"; then
      AC_DEFINE(HAVE_LASH, 1, [whether or not we are supporting lash])
      LASH_VERSION=$( pkg-config --modversion lash-1.0 )
      AC_DEFINE_UNQUOTED(LASH_VERSION, "$LASH_VERSION", [The version of lash we're compiling against])
      AC_SUBST(LASH_CFLAGS)
      AC_SUBST(LASH_LIBS)
    fi
  else
    AC_MSG_WARN([LASH support is disabled])
    LASH_FOUND="no"
  fi
else
  LASH_FOUND="no"
fi

AM_CONDITIONAL(HAVE_LASH, test "$LASH_FOUND" = "yes")

############
### lrdf ###
############
jr_enable_lrdf="yes"
AC_ARG_ENABLE(lrdf,
[  --disable-lrdf          disable lrdf (XML LADSPA description) support],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      jr_enable_lrdf="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-lrdf(=yes/no) or --disable-lrdf])
      ;;
  esac
])

if test x"$jr_enable_lrdf" = x"yes"; then
  AC_CHECK_HEADER(lrdf.h,
    AC_CHECK_LIB(lrdf, lrdf_init, LRDF_FOUND="yes", LRDF_FOUND="no"),
    LRDF_FOUND="no")
else
  AC_MSG_WARN([lrdf support is disabled])
  LRDF_FOUND="no"
fi

if test x$LRDF_FOUND = xyes; then
  AC_DEFINE(HAVE_LRDF, 1, "whether or not lrdf support is enabled")
  LRDF_CFLAGS=""
  LRDF_LIBS="-llrdf"
  AC_SUBST(LRDF_CFLAGS)
  AC_SUBST(LRDF_LIBS)
fi
AM_CONDITIONAL(HAVE_LRDF, test "$LRDF_FOUND" = "yes")


###############
### gettext ###
###############
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.14.5])


AC_OUTPUT([ po/Makefile.in
Makefile
src/Makefile
pixmaps/Makefile
])

AC_MSG_RESULT([

  JACK Rack configured
  
  ALSA support:            $ALSA_FOUND
  LRDF support:            $LRDF_FOUND
  LASH support:            $LASH_FOUND
  GNOME 2 support:         $GNOME_FOUND
  JACK Session support:    $JACK_SESSION_FOUND
  
  Installation prefix:     $prefix

])
