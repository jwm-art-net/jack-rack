AC_INIT([JACK Rack], 1.4.2)
AC_CONFIG_SRCDIR(src/lock_free_fifo.h)
AM_INIT_AUTOMAKE(1.6)
AM_CONFIG_HEADER(src/ac_config.h)
AM_MAINTAINER_MODE

dnl ***** Check for programs *****
AC_PROG_CC
AC_PROG_LIBTOOL

##############
### LADSPA ###
##############
AC_CHECK_HEADER(ladspa.h, true, AC_MSG_ERROR([ladspa is required]))

############
### JACK ###
############
PKG_CHECK_MODULES(JACK, jack >= 0.50.0, true,
                  AC_MSG_ERROR([you need jack >= 0.50.0  see http://sf.net/projects/jackit]))
JACK_VERSION=$( pkg-config --modversion jack )
AC_DEFINE_UNQUOTED(JACK_VERSION, "$JACK_VERSION", [The version of jack we're compiling against])
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)

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



##############
### GTK+ 2 ###
##############
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0, true,
                  AC_MSG_ERROR([you need gtk+ >= 2.0.0  see http://www.gtk.org/]))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

###############
### GNOME 2 ###
###############
jr_enable_gnome="yes"
AC_ARG_ENABLE(gnome,
[  --disable-gnome         disable GNOME 2 support],[
  case "$enableval" in
    "yes")
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

###############
### libxml2 ###
###############
jr_enable_xml="yes"
AC_ARG_ENABLE(xml,
[  --disable-xml           disable libxml2 support (this is required for saving/loading files)],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      jr_enable_xml="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-xml(=yes/no) or --disable-xml])
      ;;
  esac
])

if test "$jr_enable_xml" = "yes"; then
  PKG_CHECK_MODULES(XML, libxml-2.0, XML_FOUND="yes", XML_FOUND="no")

  if test "$XML_FOUND" = "yes"; then
    AC_DEFINE(HAVE_XML, 1, [whether or not we are have xml support])
    XML_VERSION=$( pkg-config --modversion libxml-2.0 )
    AC_DEFINE_UNQUOTED(XML_VERSION, "$XML_VERSION", [The version of libxml2 we're compiling against])
    AC_SUBST(XML_CFLAGS)
    AC_SUBST(XML_LIBS)
  fi
else
  XML_FOUND="no"
fi
AM_CONDITIONAL(HAVE_XML, test "$XML_FOUND" = "yes")

##############
### LADCCA ###
##############
if test x$XML_FOUND = xyes; then
  jr_enable_ladcca="yes"
  AC_ARG_ENABLE(ladcca,
  [  --disable-ladcca        disable LADCCA support],[
    case "$enableval" in
      "yes")
        ;;
      "no")
        jr_enable_ladcca="no"
        ;;
      *)
        AC_MSG_ERROR([must use --enable-ladcca(=yes/no) or --disable-ladcca])
        ;;
    esac
  ])

  if test "$jr_enable_ladcca" = "yes"; then
    PKG_CHECK_MODULES(LADCCA, ladcca-1.0, LADCCA_FOUND="yes", LADCCA_FOUND="no")
  
    if test "$LADCCA_FOUND" = "yes"; then
      AC_DEFINE(HAVE_LADCCA, 1, [whether or not we are supporting ladcca])
      LADCCA_VERSION=$( pkg-config --modversion ladcca-1.0 )
      AC_DEFINE_UNQUOTED(LADCCA_VERSION, "$LADCCA_VERSION", [The version of ladcca we're compiling against])
      AC_SUBST(LADCCA_CFLAGS)
      AC_SUBST(LADCCA_LIBS)
    fi
  else
    AC_MSG_WARN([LADCCA support is disabled])
    LADCCA_FOUND="no"
  fi
else
  LADCCA_FOUND="no"
fi
AM_CONDITIONAL(HAVE_LADCCA, test "$LADCCA_FOUND" = "yes")


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
INTLIBS=""
AC_CHECK_FUNC(gettext, [HAVEGETTEXT="yes"],
	AC_CHECK_LIB(intl, gettext, [INTLIBS="-lintl" HAVEGETTEXT="yes"],
		INTLIBS=""))

AC_CHECK_PROG(XGETTEXT, xgettext, xgettext)
AC_CHECK_PROG(MSGFMT, msgfmt, msgfmt)

if test "x$XGETTEXT" != "x"; then
	if $XGETTEXT --help 2>&1 | grep illegal &> /dev/null; then
		echo "xgettext isn't GNU version"
		XGETTEXT=""
	fi
fi

ENABLE_NLS=no
if test "x$MSGFMT" != "x" -a "x$HAVEGETTEXT" != "x"; then
	AC_DEFINE(ENABLE_NLS, 1, [Define if you want internationalization support])
	ENABLE_NLS=yes
fi
AC_SUBST(ENABLE_NLS)

AC_ARG_WITH(nlsdir,
	AC_HELP_STRING([--with-nlsdir=PATH],[specify where locale stuff should go]))

if test "x$NLSDIR" = "x"; then
	if test "x$with_nlsdir" != "x"; then
		NLSDIR=$with_nlsdir
	else
		NLSDIR=$prefix/lib/locale
	fi
fi

AC_DEFINE_UNQUOTED(NLSDIR, "$NLSDIR", [where to look for locale files])
AC_SUBST(NLSDIR)
AC_SUBST(XGETTEXT)
AC_SUBST(MSGFMT)

#######################
### desktop install ###
#######################
jr_desktop_install="/usr/share"
AC_ARG_ENABLE(desktop-inst,
[  --disable-desktop-inst  don't install pixmap and .desktop to /usr/share],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      jr_desktop_install="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-desktop-inst(=yes/no) or --disable-desktop-inst])
      ;;
  esac
])

if test "x$jr_desktop_install" = xno; then
  if test "x$prefix" != xNONE; then
    JR_DESKTOP_PREFIX="$( eval echo $datadir )"
  else
    JR_DESKTOP_PREFIX="/usr/local/share"
  fi
else
  JR_DESKTOP_PREFIX="$jr_desktop_install"
fi
AC_DEFINE_UNQUOTED(JR_DESKTOP_PREFIX, "$JR_DESKTOP_PREFIX", [Where files for desktop integration will be installed])
AC_SUBST(JR_DESKTOP_PREFIX)


AC_OUTPUT([
Makefile
src/Makefile
pixmaps/Makefile
po/Makefile
])

AC_MSG_RESULT([

  JACK Rack configured
  
  ALSA support:            $ALSA_FOUND
  XML support:             $XML_FOUND
  LRDF support:            $LRDF_FOUND
  LADCCA support:          $LADCCA_FOUND
  GNOME 2 support:         $GNOME_FOUND
  
  Installation prefix:     $prefix
  Desktop files prefix:    $JR_DESKTOP_PREFIX

])
