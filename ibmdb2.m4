AC_DEFUN(WEST_WITH_IBM_DB2,
[
AC_MSG_CHECKING(if IBM DB2 installed in system)
AC_ARG_WITH(db2_prefix, 
[  --with-ibm-db2[=DIR]    Include IBM DB2 support, DIR is the IBM DB2 base
                          install directory, defaults to /home/db2inst1/sqllib])
if test -z $withval ; then
    ibm_db2_locations="/home/db2inst1/sqllib /usr/IBMdb2/V7.1 $HOME/sqllib"

    for f in $ibm_db2_locations ; do
	if test -r "$f/include/db2ApiDf.h" -o -r "$f/include/db2AuCfg.h" ; then
    	    DB2HOME="$f"
    	    break
	fi
    done

    if test -z "$DB2HOME" ; then
	AC_MSG_RESULT(IBM DB2 home directory not found in $ibm_db2_locations)
	AC_MSG_RESULT(You can adjust the IBM DB2 path to your directory with)
	AC_MSG_RESULT(your distribution --with-ibm-db2[=DIR])
	AC_ERROR(Could not find the IBM DB2 home header/library files.)
    fi
    
    CPPFLAGS="-I$DB2HOME/include $CPPFLAGS"
    LDFLAGS="-L$DB2HOME/lib $LIBFLAGS"
else
    CPPFLAGS="-I$withval/include $CPPFLAGS"
    LDFLAGS="-L$withval/lib $LIBFLAGS $LDFLAGS"
fi
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_IBMDB2, 1, [Define if UDB DB2 installed in system ])
])


