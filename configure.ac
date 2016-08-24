# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

# this project
AC_INIT([tinyssh-convert], [0.2], [@ansemjo])
AC_CONFIG_SRCDIR([tinyssh-convert.c])
AC_CONFIG_FILES([Makefile])

# configuration results
#AC_CONFIG_HEADERS([config.h])

# put autotools auxfiles in subdirectories to reduce clutter
#AC_CONFIG_AUX_DIR([buildtools])
AC_CONFIG_MACRO_DIR([m4])

# Check for C compiler.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([ctype.h errno.h fcntl.h poll.h stdio.h stdlib.h string.h strings.h sys/stat.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memchr strcasecmp strchr strcspn])

AC_OUTPUT
