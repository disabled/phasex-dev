dnl --------------------------------------------------
dnl configure.ac for the PHASEX
dnl --------------------------------------------------

AC_INIT(PHASEX, 0.12.0-pre1)
AM_INIT_AUTOMAKE(0.10.0)
AC_CONFIG_FILES([Makefile src/Makefile phasex.spec])
AM_CONFIG_HEADER(config.h)
###AM_MAINTAINER_MODE
###AM_ACLOCAL_INCLUDE(m4)


# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
ac_prefix=$prefix
if test "x$ac_prefix" = "xNONE" || test "x$ac_prefix" = "x"; then
    ac_prefix=$ac_default_prefix
fi
AC_SUBST(ac_prefix)

ac_datadir=$DATAROOTDIR
if test "x$ac_datadir" = "xNONE" || test "x$ac_datadir" = "x"; then
	ac_datadir="${ac_prefix}/share"
fi
AC_SUBST(ac_datadir)

ac_localedir=$LOCALEDIR
if test "x$ac_localedir" = "xNONE" || test "x$ac_localedir" = "x"; then
	ac_localedir="${ac_datadir}/locale"
fi
AC_SUBST(ac_localedir)


AC_DEFINE_UNQUOTED(CONFIG_PREFIX,        ["${ac_prefix}"],                                           [Default installation prefix])
AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,     ["${ac_datadir}"],                                          [Default installation data prefix])
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,   ["${ac_localedir}"],                                        [Default installation locale prefix])
AC_DEFINE_UNQUOTED(PHASEX_LICENSE_FILE,  ["${ac_datadir}/doc/phasex-${PACKAGE_VERSION}/LICENSE"],    [PHASEX License Text])
AC_DEFINE_UNQUOTED(GPLv2_LICENSE_FILE,   ["${ac_datadir}/doc/phasex-${PACKAGE_VERSION}/GPL-v2.txt"], [GPLv2 License Text])
AC_DEFINE_UNQUOTED(PHASEX_DIR,           ["${ac_datadir}/phasex"],                                   [PHASEX Data Directory])
AC_DEFINE_UNQUOTED(PIXMAP_DIR,           ["${ac_datadir}/phasex/pixmaps"],                           [Pixmap Directory])
AC_DEFINE_UNQUOTED(PATCH_DIR,            ["${ac_datadir}/phasex/sys-patches"],                       [System Patch Directory])
AC_DEFINE_UNQUOTED(MIDIMAP_DIR,          ["${ac_datadir}/phasex/sys-midimaps"],                      [System MidiMap Directory])
AC_DEFINE_UNQUOTED(SAMPLE_DIR,           ["${ac_datadir}/phasex/sys-samples"],                       [System Sample Directory])
AC_DEFINE_UNQUOTED(PARAM_HELPFILE,       ["${ac_datadir}/phasex/help/parameters.help"],              [Parameter Help File])
AC_DEFINE_UNQUOTED(PHASEX_GTK_ENGINE_RC, ["${ac_datadir}/phasex/gtkenginerc"],                       [GTKRC stub for theme engine])
AC_DEFINE_UNQUOTED(GTKRC_DARK,           ["${ac_datadir}/themes/Phasex-Dark/gtk-2.0/gtkrc"],         [PHASEX Dark GTK Theme])
AC_DEFINE_UNQUOTED(GTKRC_LIGHT,          ["${ac_datadir}/themes/Phasex-Light/gtk-2.0/gtkrc"],        [PHASEX Light GTK Theme])
AC_DEFINE_UNQUOTED(PACKAGE_BUGREPORT,    ["weston@sysex.net"],                                       [Address where bug reports should be sent])


# save original vars while autoconf looks for programs (and changes user vars...)
ORIG_CFLAGS="$CFLAGS"
ORIG_LIBS="$LIBS"


# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL


# Check for headerss
AC_HEADER_STDC


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST


# Check for standard library functions.
AC_FUNC_MALLOC

AC_CHECK_FUNCS([sleep usleep poll fopen fgets fread fclose fprintf snprintf \
		strdup strerror strerror_r strstr strtok_r strcmp strncmp \
		strcasecmp atoi atof isdigit isupper memset memcpy])

AC_CHECK_FUNC(atan2f,[], AC_CHECK_LIB(m, atan2f))
AC_CHECK_FUNC(cosf,  [], AC_CHECK_LIB(m, cosf))
AC_CHECK_FUNC(exp,   [], AC_CHECK_LIB(m, exp))
AC_CHECK_FUNC(expf,  [], AC_CHECK_LIB(m, expf))
AC_CHECK_FUNC(fabs,  [], AC_CHECK_LIB(m, fabs))
AC_CHECK_FUNC(floor, [], AC_CHECK_LIB(m, floor))
AC_CHECK_FUNC(log,   [], AC_CHECK_LIB(m, log))
AC_CHECK_FUNC(logf,  [], AC_CHECK_LIB(m, logf))
AC_CHECK_FUNC(pow,   [], AC_CHECK_LIB(m, pow))
AC_CHECK_FUNC(sin,   [], AC_CHECK_LIB(m, sin))
AC_CHECK_FUNC(sqrt,  [], AC_CHECK_LIB(m, sqrt))

AC_CHECK_LIB(pthread, pthread_mutexattr_setprotocol,
	[AC_DEFINE_UNQUOTED(HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL, [1],
	[libpthread has pthread_mutexattr_setprotocol])])


# Check for libraries

# X
if test "$X_DISPLAY_MISSING" != ""; then
  AC_MSG_ERROR([X required])
fi

# GTK
PKG_CHECK_MODULES(GTK,
  gtk+-2.0 >= 2.4,
  true,
  AC_MSG_ERROR([need gtk+ >= 2.4])
)
AC_SUBST(GTK_CFLAGS)

# ALSA
PKG_CHECK_MODULES(ALSA,
  alsa >= 0.9.0,
  true,
  AC_MSG_ERROR([need ALSA >= 0.9.x])
)
AC_SUBST(ALSA_CFLAGS)

# JACK
PKG_CHECK_MODULES(JACK,
  jack >= 0.99.0,
  true,
  AC_MSG_ERROR([need JACK >= 0.99.0])
)
AC_SUBST(JACK_CFLAGS)

# libsamplerate
PKG_CHECK_MODULES(SAMPLERATE,
  samplerate >= 0.1.2,
  true,
  AC_MSG_ERROR([need libsamplerate >= 0.1.2])
)
AC_SUBST(SAMPLERATE_CFLAGS)


# restore original vars
CFLAGS="$ORIG_CFLAGS"
CONF_LIBS="$LIBS"
LIBS="$ORIG_LIBS"


# piece together gtk engine directory
gtk_lib_dir=`$PKG_CONFIG --variable libdir gtk+-2.0`
gtk_binary_version=`$PKG_CONFIG --variable gtk_binary_version gtk+-2.0`
gtk_engine_dir="$gtk_lib_dir/gtk-2.0/$gtk_binary_version/engines"
AC_DEFINE_UNQUOTED(GTK_ENGINE_DIR, ["$gtk_engine_dir"], [GTK engine directory])


# --enable-debug=FOO option:  pass debug flags to compiler
DEBUG_CFLAGS="none"
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug=FLAGS], [passe FLAGS to compiler for debugging.]),
	[DEBUG_CFLAGS="$enableval"])

# --enable-arch=FOO option:  select arch specific compilation
OPT_ARCH="none"
AC_ARG_ENABLE(arch,
	AC_HELP_STRING([--enable-arch=ARCH], [set the -march=ARCH and -mtune=ARCH flags for gcc.
					      (see the gcc man page for supported cpu types.)]),
	[OPT_ARCH="$enableval"])


# determine gcc version
gccver=`$CC -v 2>&1 | grep '^gcc version' | cut -f 3 -d ' '`
gccmajor=`echo "$gccver" | cut -f 1 -d '.'`
gccminor=`echo "$gccver" | cut -f 2 -d '.'`


# compiler flags
CPU_POWER_LEVEL=2
ARCH_OPT_CFLAGS=""
ARCH_MATH_CFLAGS=""
#ARCH_BITS=`( ldd /bin/true | grep /lib64/ > /dev/null && echo 64 ) || echo 32`
ARCH_BITS=`( file /bin/true | grep 'ELF 64-bit' > /dev/null && echo 64 ) || echo 32`

WARNING=`echo "*"`
case "$OPT_ARCH" in
	"none"|"generic")
		CPU_POWER_LEVEL=2
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 2))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=generic"
			WARNING=`echo "*" && \
				 echo "* WARNING:  No arch specific optimization has been specified." && \
				 echo "*           Try --enable-arch=native !!!" && \
				 echo "*"`
		else
			ARCH_OPT_CFLAGS="-mtune=i686 -march=i386"
			WARNING=`echo "*" && \
				 echo "* WARNING:  No arch specific optimization has been specified." && \
				 echo "*           Defaulting to i686 tuning for i386 CPU !!!" && \
				 echo "*"`
		fi
		ARCH_MATH_CFLAGS=""
		;;
	"native")
		CPU_POWER_LEVEL=2
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 2))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=native -march=native"
		else
			ARCH_OPT_CFLAGS=""
			WARNING=`echo "*" && \
				 echo "* WARNING:  --enable-arch=native can only be used with gcc-4.2.x or higher!" && \
				 echo "*           No arch specific optimizations will be used!" && \
				 echo "*"`
			OPT_ARCH="none"
		fi
		ARCH_MATH_CFLAGS=""
		;;
	"i386"|"i486"|"i586"|"pentium")
		CPU_POWER_LEVEL=1
		ARCH_OPT_CFLAGS="-mtune=i586 -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=32
		;;
	"i686"|"pentiumpro")
		CPU_POWER_LEVEL=1
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=32
		;;
	"pentium-mmx"|"pentium2"|"k6"|"winchip-c6")
		CPU_POWER_LEVEL=1
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-mmmx"
		ARCH_BITS=32
		;;
	"pentium3"|"pentium3m"|"c3-2")
		CPU_POWER_LEVEL=1
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse -mfpmath=sse,387"
		ARCH_BITS=32
		;;
	"pentium-m"|"pentium4"|"pentium4m")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse2 -mfpmath=sse,387"
		ARCH_BITS=32
		;;
	"k6-2"|"k6-3"|"winchip2"|"c3"|"geode")
		CPU_POWER_LEVEL=1
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-mmmx -m3dnow"
		ARCH_BITS=32
		;;
	"athlon")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=athlon-xp -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-mmmx -m3dnow"
		ARCH_BITS=32
		;;
	"athlon-tbird")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-mmmx -m3dnow"
		ARCH_BITS=32
		;;
	"athlon-4"|"athlon-mp"|"athlon-xp")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse -mfpmath=sse,387"
		ARCH_BITS=32
		;;
	"k8"|"opteron"|"athlon64"|"athlon-fx")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse2 -mfpmath=sse"
		ARCH_BITS=64
		;;
	"k8-sse3"|"opteron-sse3"|"athlon64-sse3")
		CPU_POWER_LEVEL=2
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 4))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
			ARCH_MATH_CFLAGS="-msse3"
		else
			ARCH_OPT_CFLAGS="-mtune=athlon64 -march=athlon64"
			ARCH_MATH_CFLAGS="-msse2"
		fi
		ARCH_BITS=64
		;;
	"amdfam10")
		CPU_POWER_LEVEL=3
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse4a -mfpmath=sse"
		ARCH_BITS=64
		;;
	"barcelona")
		CPU_POWER_LEVEL=3
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 4))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=barcelona -march=barcelona"
		else
			ARCH_OPT_CFLAGS="-mtune=amdfam10 -march=amdfam10"
		fi
		ARCH_MATH_CFLAGS="-msse4a -mfpmath=sse"
		ARCH_BITS=64
		;;
	"x86_64")
		CPU_POWER_LEVEL=3
		ARCH_OPT_CFLAGS="-mtune=generic"
		ARCH_MATH_CFLAGS="-msse2 -mfpmath=sse"
		ARCH_BITS=64
		;;
	"prescott")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse3 -mfpmath=sse"
		ARCH_BITS=32
		;;
	"nocona")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS="-msse3 -mfpmath=sse"
		ARCH_BITS=32
		;;
	"core2")
		CPU_POWER_LEVEL=3
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 2))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=core2 -march=core2"
		else
			ARCH_OPT_CFLAGS="-mtune=nocona -march=nocona"
		fi
		ARCH_MATH_CFLAGS="-mssse3 -mfpmath=sse"
		ARCH_BITS=64
		;;
	"amd64")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=athlon64 -march=athlon64"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=64
		;;
	"itanium"|"itanium1"|"merced"|"itanium2"|"mckinley")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=64
		;;
	"ia64")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=itanium"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=64
		;;
	"ia32e")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=generic"
		ARCH_MATH_CFLAGS=""
		ARCH_BITS=32
		;;
	"atom"|"atom32")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=prescott"
		if test "$(($gccmajor >= 4))" = "1"; then
			if test "$(($gccminor >= 5))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=atom -march=atom"
			elif test "$(($gccminor >= 3))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=core2 -march=core2"
			elif test "$(($gccminor >= 2))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=generic -march=prescot"
			fi
		fi
		ARCH_MATH_CFLAGS="-msse3 -mfpmath=sse"
		ARCH_BITS=32
		;;
	"atom64")
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=prescott"
		if test "$(($gccmajor >= 4))" = "1"; then
			if test "$(($gccminor >= 5))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=atom -march=atom"
			elif test "$(($gccminor >= 3))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=core2 -march=core2"
			elif test "$(($gccminor >= 2))" = "1"; then
				ARCH_OPT_CFLAGS="-mtune=prescott -march=prescott"
			fi
		fi
		ARCH_MATH_CFLAGS="-msse3 -mfpmath=sse"
		ARCH_BITS=64
		;;
	"geode")
		CPU_POWER_LEVEL=2
		if test "$(($gccmajor >= 4))" = "1" && test "$(($gccminor >= 3))" = "1"; then
			ARCH_OPT_CFLAGS="-mtune=geode -march=geode"
		else
			ARCH_OPT_CFLAGS="-mtune=athlon -march=athlon"
		fi
		ARCH_MATH_CFLAGS="-msse3 -mfpmath=sse"
		ARCH_BITS=64
		;;

	*)
		CPU_POWER_LEVEL=2
		ARCH_OPT_CFLAGS="-mtune=$OPT_ARCH -march=$OPT_ARCH"
		ARCH_MATH_CFLAGS=""
		;;
esac


# --enable-32bit option:  Force 32 bit builds (mainly for cross compiling...)
FORCE_32_BIT=0
AC_ARG_ENABLE(32bit,
	AC_HELP_STRING([--enable-32bit], [force 32-bit build (-m32) in 64-bit environment.]),
  	[FORCE_32_BIT=32])

if test "x$FORCE_32_BIT" = "x32"; then
	ARCH_BITS=32;
fi


# deal with user-overridden PHASEX_CPU_POWER flag
FORCE_CPU_POWER=0
AC_ARG_ENABLE(cpu-power,
	AC_HELP_STRING([--enable-cpu-power], [optimize sound quality for (1) low-, (2) medium-, or (3) high- powered CPU.]),
	[FORCE_CPU_POWER="$enableval"])

if test "x$FORCE_CPU_POWER" = "x1" || test "x$FORCE_CPU_POWER" = "x2" || test "x$FORCE_CPU_POWER" = "x3"; then
	CPU_POWER_LEVEL="$FORCE_CPU_POWER"
fi


# set optimization flags, with fewer optimizations when --enable-debug is given
case "$DEBUG_CFLAGS" in
	"none")
		DEBUG_CFLAGS=""
		OPT_CFLAGS="-pipe -Wcast-align $ARCH_OPT_CFLAGS -m$ARCH_BITS $ARCH_MATH_CFLAGS -O3 -ffast-math -fomit-frame-pointer"
		;;
	"yes")
		DEBUG_CFLAGS=""
		OPT_CFLAGS="-g -pipe -Wall -Wcast-align $ARCH_OPT_CFLAGS -m$ARCH_BITS $ARCH_MATH_CFLAGS -O2 -ffast-math"
		;;
	*)
		OPT_CFLAGS="-g -pipe -Wall -Wcast-align $ARCH_OPT_CFLAGS -m$ARCH_BITS $ARCH_MATH_CFLAGS -O2 -ffast-math"
		;;
esac


# Put all compile flags together
PHASEX_CFLAGS="$OPT_CFLAGS $DEBUG_CFLAGS"
AC_SUBST(PHASEX_CFLAGS)

PHASEX_CPPFLAGS="$ALSA_CFLAGS $JACK_CFLAGS $GTK_CFLAGS $SAMPLERATE_CFLAGS -D_GNU_SOURCE -D_REENTRANT -DPHASEX_CPU_POWER=$CPU_POWER_LEVEL"
AC_SUBST(PHASEX_CPPFLAGS)

PHASEX_LIBS="$ALSA_LIBS $JACK_LIBS $GTK_LIBS $SAMPLERATE_LIBS $CONF_LIBS -lpthread"
AC_SUBST(PHASEX_LIBS)


# Output files
AC_OUTPUT

# Display build info
AC_MSG_NOTICE([
******************************************************************************
*
* Optimized for arch: ... $OPT_ARCH (${ARCH_BITS}-bit)
* CPU power level: ...... $CPU_POWER_LEVEL / 3
*
* CC: ................... $CC
* GCC version: .......... $gccver
*
* GTK binary version: ... $gtk_binary_version
* GTK engine dir: ....... $gtk_engine_dir
*
* CFLAGS: ............... '$CFLAGS'
* PHASEX_CFLAGS: ........ '$PHASEX_CFLAGS'
*
* CPPFLAGS: ............. '$CPPFLAGS'
* PHASEX_CPPFLAGS: ...... '$PHASEX_CPPFLAGS'
*
* LIBS: ................. '$LIBS'
* PHASEX_LIBS: .......... '$PHASEX_LIBS'
*
* PHASEX is now configured!  Now run:
*   make
*   make install
$WARNING
******************************************************************************])
