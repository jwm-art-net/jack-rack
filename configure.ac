AC_INIT([JACK Rack], 1.2.0)
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
PKG_CHECK_MODULES(JACK, jack, true, AC_MSG_ERROR([you need the jack audio api]))
JACK_VERSION=$( pkg-config --modversion jack )
AC_DEFINE_UNQUOTED(JACK_VERSION, "$JACK_VERSION", [The version of jack we're compiling against])
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)

### optional funcs
ORIG_LIBS="$LIBS"
LIBS="$LIBS $JACK_LIBS"
ORIG_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $JACK_CFLAGS"

AC_CHECK_FUNCS(jack_set_server_dir)

LIBS="$ORIG_LIBS"
CFLAGS="$ORIG_CFLAGS"


##############
### GTK+ 2 ###
##############
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0)
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
  AC_SUBST(GNOMEUI_CFLAGS)
  AC_SUBST(GNOMEUI_LIBS)

  if test "$GNOME_FOUND" = "yes"; then
    AC_DEFINE(HAVE_GNOME, 1, [whether or not we are supporting gnome])
    GNOME_VERSION=$( pkg-config --modversion libgnomeui-2.0 )
    AC_DEFINE_UNQUOTED(GNOME_VERSION, "$GNOME_VERSION", [The version of gnome we're compiling against])
  fi
else
  GNOME_FOUND="no"
fi
AM_CONDITIONAL(HAVE_GNOME, test "$GNOME_FOUND" = "yes")


##############
### LADCCA ###
##############
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
  AC_SUBST(LADCCA_CFLAGS)
  AC_SUBST(LADCCA_LIBS)

  if test "$LADCCA_FOUND" = "yes"; then
    AC_DEFINE(HAVE_LADCCA, 1, [whether or not we are supporting ladcca])
    LADCCA_VERSION=$( pkg-config --modversion ladcca-1.0 )
    AC_DEFINE_UNQUOTED(LADCCA_VERSION, "$LADCCA_VERSION", [The version of ladcca we're compiling against])
  fi
else
  AC_MSG_WARN([LADCCA support is disabled])
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

  LRDF_CFLAGS=""
  LRDF_LIBS="-llrdf"
else
  AC_MSG_WARN([lrdf support is disabled])
  LRDF_FOUND="no"
fi

if test x$LRDF_FOUND = xyes; then
  AC_DEFINE(HAVE_LRDF, 1, "whether or not lrdf support is enabled")
  AC_SUBST(LRDF_CFLAGS)
  AC_SUBST(LRDF_LIBS)
fi
AM_CONDITIONAL(HAVE_LRDF, test "$LRDF_FOUND" = "yes")

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
])

AC_MSG_RESULT([

  JACK Rack configured
  
  GNOME 2 support:      $GNOME_FOUND
  LADCCA support:       $LADCCA_FOUND
  LRDF support:         $LRDF_FOUND
  
  Installation prefix:  $prefix
  Desktop files prefix: $JR_DESKTOP_PREFIX

])
