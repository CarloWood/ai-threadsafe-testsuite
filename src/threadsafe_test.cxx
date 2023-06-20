#include "sys.h"
#include "threadsafe/threadsafe.h"
#include "threadsafe/AIReadWriteMutex.h"

#include <iostream>
#include <cassert>

using namespace threadsafe;

template<int size, typename U>
void do_asserts()
{
#if THREADSAFE_DEBUG
  static_assert(std::max(alignof(typename U::data_type), alignof(policy::OneThread)) == alignof(U), "alignof(Unlocked<T, OneThread>) != alignof(T)!");
#else
  static_assert(alignof(typename U::data_type) == alignof(U), "alignof(Unlocked<T, OneThread>) != alignof(T)!");
#endif
  static_assert(alignof(Unlocked<typename U::data_type, policy::Primitive<std::mutex>>) % alignof(typename U::data_type) == 0, "alignof(Unlocked<T, Primitive<std::mutex>>) is not a multiple of alignof(T)!");
}

template<int size>
void do_size_test()
{
  struct T0 { char a[size]; };
  struct T1 { char x; char a[size]; };
  struct T2 { short x; char a[size]; };
  struct T4 { int32_t x; char a[size]; };
  struct T8 { int64_t x; char a[size]; };

  do_asserts<size, Unlocked<T1, policy::OneThread>>();
  do_asserts<size, Unlocked<T2, policy::OneThread>>();
  do_asserts<size, Unlocked<T4, policy::OneThread>>();
  do_asserts<size, Unlocked<T8, policy::OneThread>>();
}

enum state_type { unlocked, readlocked, writelocked };

class TestRWMutex
{
  private:
    state_type m_state;

  public:
    TestRWMutex() : m_state(unlocked) { }

    void rdlock() { assert(m_state == unlocked); m_state = readlocked; }
    void rdunlock() { assert(m_state == readlocked); m_state = unlocked; }
    void wrlock() { assert(m_state == unlocked); m_state = writelocked; }
    void wrunlock() { assert(m_state == writelocked); m_state = unlocked; }
    void rd2wrlock() { assert(m_state == readlocked); m_state = writelocked; }
    void wr2rdlock() { assert(m_state == writelocked); m_state = readlocked; }
    void rd2wryield() { }

  public:
    bool is_unlocked() const { return m_state == unlocked; }
    bool is_readlocked() const { return m_state == readlocked; }
    bool is_writelocked() const { return m_state == writelocked; }
};

class TestMutex
{
  private:
    state_type m_state;

  public:
    TestMutex() : m_state(unlocked) { }

    void lock() { assert(m_state == unlocked); m_state = writelocked; }
    void unlock() { assert(m_state == writelocked); m_state = unlocked; }

  public:
    bool is_unlocked() const { return m_state == unlocked; }
    bool is_locked() const { return m_state == writelocked; }
};

struct Foo {
  int x;
};

#define TEST_READWRITE 1

#if TEST_READWRITE
using foo_t = Unlocked<Foo, policy::ReadWrite<TestRWMutex>>;
#else
using foo_t = Unlocked<Foo, policy::Primitive<TestMutex>>;
#endif

// Hack access to TestRWMutex.
class LockAccess : public foo_t
{
  public:
#if TEST_READWRITE
    bool is_unlocked() const { return this->m_read_write_mutex.is_unlocked(); }
    bool is_readlocked() const { return this->m_read_write_mutex.is_readlocked(); }
    bool is_writelocked() const { return this->m_read_write_mutex.is_writelocked(); }
#else
    bool is_unlocked() const { return this->m_primitive_mutex.is_unlocked(); }
    bool is_readlocked() const { return this->m_primitive_mutex.is_locked(); }
    bool is_writelocked() const { return this->m_primitive_mutex.is_locked(); }
#endif
};

bool is_unlocked(foo_t const& wrapper)
{
  return static_cast<LockAccess const&>(wrapper).is_unlocked();
}

bool is_readlocked(foo_t const& wrapper)
{
  return static_cast<LockAccess const&>(wrapper).is_readlocked();
}

bool is_writelocked(foo_t const& wrapper)
{
  return static_cast<LockAccess const&>(wrapper).is_writelocked();
}

// Hack access to m_unlocked.
class AccessUnlocked : public foo_t::crat
{
  public:
    bool is_unlocked() const { return ::is_unlocked(*this->m_unlocked); }
    bool is_readlocked() const { return ::is_readlocked(*this->m_unlocked); }
    bool is_writelocked() const { return ::is_writelocked(*this->m_unlocked); }
};

bool is_unlocked(foo_t::crat const& access)
{
  AccessUnlocked const& a = static_cast<AccessUnlocked const&>(access);
  return a.is_unlocked();
}

bool is_readlocked(foo_t::crat const& access)
{
  AccessUnlocked const& a = static_cast<AccessUnlocked const&>(access);
  return a.is_readlocked();
}

bool is_writelocked(foo_t::crat const& access)
{
  AccessUnlocked const& a = static_cast<AccessUnlocked const&>(access);
  return a.is_writelocked();
}

void func_read_const(foo_t::crat const& access)
{
  std::cout << access->x << std::endl;
  assert(is_readlocked(access) || is_writelocked(access));
}

void func_read_and_then_write(foo_t::rat& access)
{
  std::cout << access->x << std::endl;
  assert(is_readlocked(access) || is_writelocked(access));
#if TEST_READWRITE
  foo_t::wat write_access(access);				// This might throw if is_readlocked(access).
#else
  foo_t::wat const& write_access = wat_cast(access);
#endif
  write_access->x = 6;
  assert(is_writelocked(access));
}

void func_write(foo_t::wat const& access)
{
  access->x = 5;
  assert(is_writelocked(access));
}

template<typename onethread_t, typename primitive_t, typename readwrite_t>
void do_const_access_test(onethread_t const& onethread, primitive_t const& primitive, readwrite_t const& readwrite)
{
  // Reading
  {
    typename onethread_t::crat onethread_r(onethread);
    assert(onethread_r->x == 111);
  }
  {
    typename primitive_t::crat primitive_r(primitive);
    assert(primitive_r->x == 222);
  }
  {
    typename readwrite_t::crat readwrite_r(readwrite);
    assert(readwrite_r->x == 333);
  }
}

template<typename onethread_t, typename primitive_t, typename readwrite_t>
void do_access_test(onethread_t& onethread, primitive_t& primitive, readwrite_t& readwrite)
{
  // Writing
  {
    typename onethread_t::wat onethread_w(onethread);
    onethread_w->x = 111;
  }
  {
    typename primitive_t::wat primitive_w(primitive);
    primitive_w->x = 222;
  }
  {
    typename readwrite_t::wat readwrite_w(readwrite);
    readwrite_w->x = 333;
  }

  // Reading
  {
    typename onethread_t::rat onethread_r(onethread);
    assert(onethread_r->x == 111);
  }
  {
    typename primitive_t::rat primitive_r(primitive);
    assert(primitive_r->x == 222);
  }
  {
    typename readwrite_t::rat readwrite_r(readwrite);
    assert(readwrite_r->x == 333);
  }

  do_const_access_test(onethread, primitive, readwrite);

  // Conversions, write to read access.
  {
    typename onethread_t::wat onethread_w(onethread);
    onethread_w->x = 444;
    typename onethread_t::rat const& onethread_r(onethread_w);
    assert(onethread_r->x == 444);
  }
  {
    typename primitive_t::wat primitive_w(primitive);
    primitive_w->x = 555;
    typename primitive_t::rat const& primitive_r(primitive_w);
    assert(primitive_r->x == 555);
  }
  {
    typename readwrite_t::wat readwrite_w(readwrite);
    readwrite_w->x = 666;
    typename readwrite_t::rat const& readwrite_r(readwrite_w);
    assert(readwrite_r->x == 666);
  }

  // Conversions, read to write access.
  {
    typename onethread_t::rat onethread_r(onethread);
    typename onethread_t::wat const& onethread_w = wat_cast(onethread_r);
    onethread_w->x = 777;
    assert(onethread_r->x == 777);
  }
  {
    typename primitive_t::rat primitive_r(primitive);
    typename primitive_t::wat const& primitive_w = wat_cast(primitive_r);
    primitive_w->x = 888;
    assert(primitive_r->x == 888);
  }
  {
    typename readwrite_t::rat readwrite_r(readwrite);
    typename readwrite_t::wat readwrite_w(readwrite_r);
    readwrite_w->x = 999;
    assert(readwrite_r->x == 999);
  }
}

int main()
{
  std::cout << "Testing size and alignment... " << std::flush;
  do_size_test<1>();
  do_size_test<2>();
  do_size_test<3>();
  do_size_test<4>();
  do_size_test<5>();
  do_size_test<6>();
  do_size_test<7>();
  do_size_test<8>();
  do_size_test<9>();
  do_size_test<10>();
  do_size_test<11>();
  do_size_test<12>();
  do_size_test<13>();
  do_size_test<14>();
  do_size_test<15>();
  do_size_test<16>();
  do_size_test<17>();
  do_size_test<18>();
  do_size_test<19>();
  do_size_test<20>();
  do_size_test<21>();
  do_size_test<22>();
  do_size_test<23>();
  do_size_test<24>();
  do_size_test<25>();
  std::cout << "OK" << std::endl;

  // ThreadSafe compile tests.
  struct A { int x; };
  using onethread_t = Unlocked<A, policy::OneThread>;
  using primitive_t = Unlocked<A, policy::Primitive<std::mutex>>;
  using readwrite_t = Unlocked<A, policy::ReadWrite<AIReadWriteMutex>>;

  onethread_t onethread;
  primitive_t primitive;
  readwrite_t readwrite;

  do_access_test(onethread, primitive, readwrite);

  foo_t wrapper;
  foo_t const& const_wrapper(wrapper);

  // Things that should compile.
  {
    // Getting write access to non-const wrapper.
    foo_t::wat write_access(wrapper);
    write_access->x = 3;
    assert(is_writelocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Getting read only access to const wrapper.
    foo_t::crat read_access(const_wrapper);
    std::cout << read_access->x << std::endl;
    assert(is_readlocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Creating a crat from a non-const wrapper.
    foo_t::crat read_access(wrapper);
    std::cout << read_access->x << std::endl;
    assert(is_readlocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Getting first read access to non-const wrapper, and then write access.
#if TEST_READWRITE
    for(;;)
    {
      try
      {
#endif
	foo_t::rat read_access(wrapper);
	std::cout << read_access->x << std::endl;
	assert(is_readlocked(wrapper));
#if TEST_READWRITE
	foo_t::wat write_access(read_access);		// This might throw.
#else
	foo_t::wat const& write_access = wat_cast(read_access);
#endif
	write_access->x = 4;
	assert(is_writelocked(wrapper));
#if TEST_READWRITE
      }
      catch (std::exception const&)
      {
	wrapper.rd2wryield();				// Block until the other thread that tries to convert a read to write lock succeeded.
	// Try again.
	continue;
      }
      break;
    }
#endif
  }
  assert(is_unlocked(wrapper));
  {
    // Passing a crat to func_read_const
    foo_t::crat read_access_const(const_wrapper);	// OK
    func_read_const(read_access_const);
    assert(is_readlocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Passing a rat to func_read_const
    foo_t::rat read_access(wrapper);			// OK
    func_read_const(read_access);
    assert(is_readlocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Passing a wat to func_read_const
    foo_t::wat write_access(wrapper);			// OK
    func_read_const(write_access);
    assert(is_writelocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
#if TEST_READWRITE
    for(;;)
    {
      try
      {
#endif
	// Passing a rat to func_read
	foo_t::rat read_access(wrapper);		// OK
	func_read_and_then_write(read_access);		// This might throw.
	assert(is_readlocked(wrapper));
#if TEST_READWRITE
      }
      catch(std::exception const&)
      {
	wrapper.rd2wryield();
	continue;
      }
      break;
    }
#endif
  }
#if TEST_READWRITE
  assert(is_unlocked(wrapper));
  {
    foo_t::w2rCarry carry(wrapper);
    assert(is_unlocked(wrapper));
    func_write(foo_t::wat(carry));
    assert(is_readlocked(wrapper));
    {
      foo_t::rat read_access(carry);
      func_read_const(read_access);
      assert(is_readlocked(wrapper));
    }
    assert(is_readlocked(wrapper));
  }
#endif
  assert(is_unlocked(wrapper));
  {
    // Passing a wat to func_read
    foo_t::wat write_access(wrapper);			// OK
    func_read_and_then_write(write_access);
    assert(is_writelocked(wrapper));
  }
  assert(is_unlocked(wrapper));
  {
    // Passing a wat to func_write
    foo_t::wat write_access(wrapper);			// OK
    func_write(write_access);
    assert(is_writelocked(wrapper));
  }
  assert(is_unlocked(wrapper));

  std::cout << "Success!" << std::endl;

  // Things that should not compile:
#ifdef TEST1
  {
    // Getting write access to a const wrapper.
    foo_t::wat fail(const_wrapper);			// TEST1 FAIL (error: no matching constructor for initialization of 'foo_t::wat' (aka 'WriteAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST2
  {
    // Creating a rat from a const wrapper.
    foo_t::rat fail(const_wrapper);			// TEST2 FAIL (error: no matching constructor for initialization of 'foo_t::rat' (aka 'ReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST3
  {
    // Getting write access from wat.
    foo_t::wat write_access(wrapper);			// OK
    foo_t::wat fail(write_access);			// TEST3 FAIL (error: call to implicitly-deleted copy constructor of 'foo_t::wat' (aka 'WriteAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST4
  {
    // Getting write access from crat.
    foo_t::crat read_access_const(const_wrapper);	// OK
    foo_t::wat fail(read_access_const);			// TEST4 FAIL (error: no matching constructor for initialization of 'foo_t::wat' (aka 'WriteAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST5
  {
    // Write to something that you only have read access too.
    foo_t::crat read_access_const(const_wrapper);	// OK
    read_access_const->x = -1;				// TEST5 FAIL (error: cannot assign to return value because function 'operator->' returns a const value).
  }
#endif
#ifdef TEST6
  {
    // Write to something that you only have read access too.
    foo_t::rat read_access(wrapper);			// OK
    read_access->x = -1;				// TEST6 FAIL (error: cannot assign to return value because function 'operator->' returns a const value).
  }
#endif
#ifdef TEST7
  {
    // Create crat from crat.
    foo_t::crat read_access_const(const_wrapper);	// OK
    foo_t::crat fail(read_access_const);		// TEST7 FAIL (error: call to deleted constructor of 'foo_t::crat' (aka 'ConstReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST8
  {
    // Create crat from rat.
    foo_t::rat read_access(wrapper);			// OK
    foo_t::crat fail(read_access);			// TEST8 FAIL (error: call to deleted constructor of 'foo_t::crat' (aka 'ConstReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST9
  {
    // Create crat from wat.
    foo_t::wat write_access(wrapper);			// OK
    foo_t::crat fail(write_access);			// TEST9 FAIL (error: call to deleted constructor of 'foo_t::crat' (aka 'ConstReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST10
  {
    // Create rat from crat.
    foo_t::crat read_access_const(const_wrapper);	// OK
    foo_t::rat fail(read_access_const);			// TEST10 FAIL (error: no matching constructor for initialization of 'foo_t::rat' (aka 'ReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST11
  {
    // Create rat from rat.
    foo_t::rat read_access(wrapper);			// OK
    foo_t::rat fail(read_access);			// TEST11 FAIL (error: call to implicitly-deleted copy constructor of 'foo_t::rat' (aka 'ReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST12
  {
    // Create rat from wat.
    foo_t::wat write_access(wrapper);			// OK
    foo_t::rat fail(write_access);			// TEST12 FAIL (error: call to implicitly-deleted copy constructor of 'foo_t::rat' (aka 'ReadAccess<threadsafe::Unlocked<Foo, threadsafe::policy::ReadWrite<TestRWMutex>>>')).
  }
#endif
#ifdef TEST13
  {
    // Passing a crat to func_read.
    foo_t::crat read_access_const(const_wrapper);	// OK
    func_read_and_then_write(read_access_const);	// TEST13 FAIL (error: no matching function for call to 'func_read_and_then_write').
  }
#endif
#ifdef TEST14
  {
    // Passing a crat to func_write.
    foo_t::crat read_access_const(const_wrapper);	// OK
    func_write(read_access_const);			// TEST14 FAIL (error: no matching function for call to 'func_write').
  }
#endif
#ifdef TEST15
  {
    // Passing a rat to func_write.
    foo_t::rat read_access(wrapper);			// OK
    func_write(read_access);				// TEST15 FAIL (error: no matching function for call to 'func_write').
  }
#endif
#ifdef TEST16
#error That was the last test
#endif
}
