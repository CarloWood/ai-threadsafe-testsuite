# Define package version and name.
define(CW_VERSION_MAJOR, 1)
define(CW_VERSION_MINOR, 0)
define(CW_VERSION_REVISION, 0)
define(CW_PACKAGE_NAME, ai-threadsafe)

# Other configuration.
define(CW_BUGREPORT, carlo@alinoe.com)
define(CW_COMPILER_WARNINGS, [-W -Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Winline])

# Because we use cwm4.
m4_include([cwm4/configure_ac_top.m4])

# We use doxygen for documentation.
CW_DOXYGEN

# The submodules that we're using.
CW_SUBMODULE([cwd])
CW_SUBMODULE([utils])
CW_SUBMODULE([threadsafe])

# Because we use cwm4.
m4_include([cwm4/configure_ac_bottom.m4])
