#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#

AC_DEFUN([XFT], [

if test "${TEA_WINDOWINGSYSTEM}" = "x11"; then
    AC_MSG_CHECKING([whether to use xft])
    AC_ARG_ENABLE(xft,
	AC_HELP_STRING([--enable-xft],
	    [use freetype/fontconfig/xft (default: on)]),
	[enable_xft=$enableval], [enable_xft="default"])
    XFT_CFLAGS=""
    XFT_LIBS=""
    if test "$enable_xft" = "no" ; then
	AC_MSG_RESULT([$enable_xft])
    else
	found_xft="yes"
	dnl make sure package configurator (xft-config or pkg-config
	dnl says that xft is present.
	XFT_CFLAGS=`xft-config --cflags 2>/dev/null` || found_xft="no"
	XFT_LIBS=`xft-config --libs 2>/dev/null` || found_xft="no"
	if test "$found_xft" = "no" ; then
	    found_xft=yes
	    XFT_CFLAGS=`pkg-config --cflags xft 2>/dev/null` || found_xft="no"
	    XFT_LIBS=`pkg-config --libs xft 2>/dev/null` || found_xft="no"
	fi
	AC_MSG_RESULT([$found_xft])
	dnl make sure that compiling against Xft header file doesn't bomb
	if test "$found_xft" = "yes" ; then
	    tk_oldCFlags=$CFLAGS
	    CFLAGS="$XINCLUDES $XFT_CFLAGS"
	    tk_oldLibs=$LIBS
	    LIBS="$tk_oldLIBS $XFT_LIBS $XLIBSW"
	    AC_CHECK_HEADER(X11/Xft/Xft.h, [], [
		found_xft=no
	    ],[#include <X11/Xlib.h>])
	    CFLAGS=$tk_oldCFlags
	    LIBS=$tk_oldLibs
	fi
	dnl make sure that linking against Xft libraries finds freetype
	if test "$found_xft" = "yes" ; then
	    tk_oldCFlags=$CFLAGS
	    CFLAGS="$XINCLUDES $XFT_CFLAGS"
	    tk_oldLibs=$LIBS
	    LIBS="$tk_oldLIBS $XFT_LIBS $XLIBSW"
	    AC_CHECK_LIB(Xft, XftFontOpen, [], [
		found_xft=no
	    ])
	    CFLAGS=$tk_oldCFlags
	    LIBS=$tk_oldLibs
	fi
	dnl print a warning if xft is unusable and was specifically requested
	if test "$found_xft" = "no" ; then
	    if test "$enable_xft" = "yes" ; then
		AC_MSG_WARN([Can't find xft configuration, or xft is unusable])
	    fi
	    enable_xft=no
	    XFT_CFLAGS=""
	    XFT_LIBS=""
	else
            enable_xft=yes
	fi
    fi
    if test $enable_xft = "yes" ; then
	AC_DEFINE(HAVE_XFT, 1, [Have we turned on XFT (antialiased fonts)?])
    fi
    AC_SUBST(XFT_CFLAGS)
    AC_SUBST(XFT_LIBS)
fi

])

