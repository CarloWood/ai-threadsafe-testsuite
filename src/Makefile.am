AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = threadsafe_test readwritemutex_test readwritespinlock_test

threadsafe_test_SOURCES = \
	aithreadsafe_test.cxx \
	aithreadsafe.h

readwritemutex_test_SOURCES = \
	AIReadWriteMutex_test.cxx \
	AIReadWriteMutex.h

readwritespinlock_test_SOURCES = \
	AIReadWriteSpinLock_test.cxx \
	AIReadWriteSpinLock.h

threadsafe_test_CXXFLAGS = -std=c++11 @LIBCWD_R_FLAGS@
threadsafe_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

readwritemutex_test_CXXFLAGS = -std=c++11 @LIBCWD_R_FLAGS@
readwritemutex_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la $(top_builddir)/cwds/libcwds_r.la

readwritespinlock_test_CXXFLAGS = -O3 -std=c++11 @LIBCWD_R_FLAGS@
readwritespinlock_test_LDADD = ../threadsafe/libthreadsafe.la ../utils/libutils_r.la ../microbench/libmicrobench.la $(top_builddir)/cwds/libcwds_r.la

# --------------- Maintainer's Section

if MAINTAINER_MODE
distclean-local:
	rm -f *.s *.ii

clean-local:
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
