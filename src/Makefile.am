AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = threadsafe_test readwritemutex_test readwritespinlock_test condition_variable_test

threadsafe_test_SOURCES = \
	threadsafe_test.cxx \
	threadsafe.h

readwritemutex_test_SOURCES = \
	AIReadWriteMutex_test.cxx \
	AIReadWriteMutex.h

readwritespinlock_test_SOURCES = \
	AIReadWriteSpinLock_test.cxx \
	AIReadWriteSpinLock.h

condition_variable_test_SOURCES = \
	condition_variable_test.cxx

threadsafe_test_CXXFLAGS = -std=c++11 @LIBCWD_R_FLAGS@
threadsafe_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

readwritemutex_test_CXXFLAGS = -std=c++11 @LIBCWD_R_FLAGS@
readwritemutex_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

readwritespinlock_test_CXXFLAGS = -O3 -std=c++11 @LIBCWD_R_FLAGS@
readwritespinlock_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la ../microbench/libmicrobench.la $(top_builddir)/cwds/libcwds_r.la

condition_variable_test_CXXFLAGS = -O3 -std=c++11
condition_variable_test_LDFLAGS = -pthread

# --------------- Maintainer's Section

if MAINTAINER_MODE
distclean-local:
	rm -f *.s *.ii

clean-local:
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
