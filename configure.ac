AC_PREREQ(2.53)
AC_INIT(aclocal.m4)
AC_CANONICAL_SYSTEM

AC_ARG_ENABLE(debug,
              [  --enable-debug          Enable debugging information],
              USE_DEBUG="$enableval", USE_DEBUG="no")

if test $USE_DEBUG = yes ; then 
  DEBUG=1
  dnl Bakefile doesn't touch {C,CPP,CXX,LD}FLAGS in autoconf format, we
  dnl have to do it ourselves. This will work with many compilers
  dnl (but not all, proper configure script would check if the compiler
  dnl supports it):
  CFLAGS="$CFLAGS -g"
  CXXFLAGS="$CXXFLAGS -g"
else
  DEBUG=0
fi

AC_ARG_ENABLE(gprof,
              [  --enable-gprof          Enable gprof information],
              USE_GPROF="$enableval", USE_GPROF="no")

if test $USE_GPROF = yes ; then 
  GPROF=1
  dnl Bakefile doesn't touch {C,CPP,CXX,LD}FLAGS in autoconf format, we
  dnl have to do it ourselves. This will work with many compilers
  dnl (but not all, proper configure script would check if the compiler
  dnl supports it):
  CFLAGS="$CFLAGS -pg"
  CXXFLAGS="$CXXFLAGS -pg"
  LIBS="$LIBS -pg"
else
  GPROF=0
fi


dnl
dnl BOOST
dnl
AC_ARG_WITH(
	boost,
	AS_HELP_STRING([--with-boost=DIR],[use Boost directory DIR, e.g. /usr if header files are in /usr/include/boost]),
	[BOOST_PREFIX=$with_boost]
)
AC_SUBST(BOOST_PREFIX)

function get_library_name {	 
  cd $1; ls lib${2}*.so | head -n1 | sed 's/^lib//' | sed 's/\.so//'
}

AS_IF(
	[for ac_boost_path_tmp in ${BOOST_PREFIX} /usr /usr/local /opt /sw; do
				echo $ac_boost_path_tmp
                                if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
                                        LIBS="$LIBS -L$ac_boost_path_tmp/lib"
                                        CXXFLAGS="$CXXFLAGS -I$ac_boost_path_tmp/include"
					HAVE_BOOST=1
					BOOST_PROGRAM_OPTIONS_LIB=$(get_library_name $ac_boost_path_tmp/lib boost_program_options)
					test -r $BOOST_PROGRAM_OPTIONS_LIB
					BOOST_UNIT_TEST_FRAMEWORK_LIB=$(get_library_name $ac_boost_path_tmp/lib boost_unit_test_framework)
					test -r $BOOST_UNIT_TEST_FRAMEWORK_LIB
					BOOST_THREAD_LIB=$(get_library_name $ac_boost_path_tmp/lib boost_thread)
					test -r $BOOST_THREAD_LIB
					BOOST_FILESYSTEM_LIB=$(get_library_name $ac_boost_path_tmp/lib boost_filesystem)
					test -r $BOOST_FILESYSTEM_LIB
                                        break;
                                fi
                        done;
			test "$HAVE_BOOST"],
			[],
			AC_MSG_FAILURE([could not find Boost]))

AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB)
AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_LIB)
AC_SUBST(BOOST_THREAD_LIB)
AC_SUBST(BOOST_FILESYSTEM_LIB)

dnl
dnl OPENFST
dnl
AC_ARG_WITH(
	openfst,
	AS_HELP_STRING([--with-openfst=DIR],[use Openfst include directory DIR]),
	[OPENFST_PREFIX=$with_openfst]
)
AC_SUBST(OPENFST_PREFIX)

AS_IF(
	[for ac_openfst_path_tmp in ${OPENFST_PREFIX} ${HOME}/openfst /usr/openfst /usr/local/openfst /opt/openfst /sw/openfst /opt/local/openfst /Applications/openfst; do
                                if test -d "$ac_openfst_path_tmp/src/lib/.libs" && test -r "$ac_openfst_path_tmp/src/lib/.libs"; then
                                        LIBS="$LIBS -L$ac_openfst_path_tmp/src/lib/.libs"
                                        CXXFLAGS="$CXXFLAGS -I$ac_openfst_path_tmp/src/include -I$ac_openfst_path_tmp/src/bin -I$ac_openfst_path_tmp"
					HAVE_OPENFST=1
                                        break;
                                fi
                        done;
			test "$HAVE_OPENFST"],
			[],
			AC_MSG_FAILURE([could not find Openfst]))


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_C_INLINE

AC_BAKEFILE([m4_include(autoconf_inc.m4)])
AC_OUTPUT([Makefile])
