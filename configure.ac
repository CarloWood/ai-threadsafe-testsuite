dnl Define package version here.
define(CW_VERSION_MAJOR, 1)
define(CW_VERSION_MINOR, 0)
define(CW_VERSION_REVISION, 0)
define(CW_PACKAGE_NAME, ai-threadsafe)
define(CW_BUGREPORT, carlo@alinoe.com)

dnl Because we use cwm4.
include([cwm4/configure_ac_top.m4])

dnl We use doxygen for documentation.
CW_DOXYGEN

dnl The submodules that we're using.
CW_SUBMODULE([cwd])
CW_SUBMODULE([utils])
CW_SUBMODULE([threadsafe])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
