#include "sys.h"
#include "threadsafe/threadsafe.h"
#include "threadsafe/AIReadWriteMutex.h"
#include "utils/AIRefCount.h"

#include <iostream>
#include <cassert>
#include <concepts>

using namespace threadsafe;

class B : public AIRefCount {
 public:
  virtual ~B() {}
  virtual void modify() = 0;
  virtual void print() const = 0;
};

class A : public B {
  int m_;

 public:
  A(int m) : m_(m)
  {
    DoutEntering(dc::notice, "A::A(" << m << ") [" << this << "]");
  }

  ~A()
  {
    DoutEntering(dc::notice, "A::~A() [" << this << "]");
  }

  void modify() override
  {
    ++m_;
  }

  void print() const override
  {
    std::cout << "m_ = " << m_ << '\n';
  }
};

//using UnlockedA = Unlocked<A, policy::Primitive<std::mutex>>;
using UnlockedA = Unlocked<A, policy::ReadWrite<AIReadWriteMutex>>;
//using UnlockedA = Unlocked<A, policy::OneThread>;
using UnlockedB = UnlockedBase<B, UnlockedA::policy_type>;

void f(UnlockedB& b)
{
  {
    UnlockedB::wat b_w(b);    // Get write-access.
    b_w->modify();
  }
  {
    UnlockedB::crat b_r(b);    // Get read-access.
    b_r->print();
  }
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  boost::intrusive_ptr<UnlockedA> a = new UnlockedA(42);
  {
    UnlockedB b = *a;
    f(b);
    Dout(dc::notice, "Leaving scope");
  }
  Dout(dc::notice, "Leaving main()");
}
