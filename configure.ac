AC_INIT([OpenABMCovid19],[0.3])

AC_PATH_PROG([GSL_CONFIG], [gsl-config])
if test "${GSL_CONFIG}" != ""; then
    GSL_CFLAGS=`${GSL_CONFIG} --cflags`
    GSL_LIBS=`${GSL_CONFIG} --libs`
    AC_SUBST(GSL_CFLAGS)
    AC_SUBST(GSL_LIBS)
    AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT
else
    AC_MSG_ERROR(['gsl-config not found, GNU GSL required'])
fi

