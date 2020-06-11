#include "sys.h"
#include "microbench/microbench.h"
#include "threadsafe/AIReadWriteSpinLock.h"
#include "debug.h"

#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <algorithm>

constexpr unsigned int size_of_count = 33;
long volatile count[size_of_count];
int const number_of_threads = std::min(std::thread::hardware_concurrency(), size_of_count - 1);
int const n = 10000000;

std::atomic<int> write_access;
std::atomic<int> read_access;
std::atomic<int> thr_count;
std::atomic<int> max_readers;

inline void add(long d, int i = 0)
{
  ++write_access;
  assert(write_access == 1 && read_access == 0);
  count[0] = count[i] + d;
  --write_access;
}

inline void read(int i)
{
  ++read_access;
  assert(write_access == 0);
  count[i] = count[0];
  int v = read_access;
  if (v > max_readers)
    max_readers = v;
  --read_access;
}

AIReadWriteSpinLock m;
AIReadWriteSpinLock a[9];

void run()
{
  Debug(NAMESPACE_DEBUG::init_thread());

  int thr = ++thr_count;
  for (int i = 0; i < n; ++i)
  {
    std::this_thread::yield();
    if (i % 100 == 0)
    {
      m.wrlock();
      add(1);
      m.wrunlock();
      m.rdlock();
      read(thr);
      m.rdunlock();
    }
  }
  std::cout << "Thread " << thr << " finished.\n";
}

int volatile x = 0;

void bench_mark0()
{
  a[0].rdlock();
  a[0].rdunlock();
}

void bench_mark1()
{
  a[1].rdlock();
  a[1].rdunlock();
}

void bench_run()
{
  Debug(NAMESPACE_DEBUG::init_thread());
  int thr = ++thr_count;

  if (thr == number_of_threads)
  {
    // Bench mark.
    moodycamel::stats_t stats = moodycamel::microbench_stats(&bench_mark0, 1000000, 20);

    printf("Thread %d statistics: avg: %.2fns, min: %.2fns, max: %.2fns, stddev: %.2fns, Q1: %.2fns, median: %.2fns, Q3: %.2fns\n",
      thr,
      stats.avg() * 1000000,
      stats.min() * 1000000,
      stats.max() * 1000000,
      stats.stddev() * 1000000,
      stats.q1() * 1000000,
      stats.median() * 1000000,
      stats.q3() * 1000000);
  }
  else
  {
    for (int j = 0; j < 20; ++j)
      for (int i = 0; i < 1000000; ++i)
        bench_mark1();
  }
  std::cout << "Thread " << thr << " finished.\n";
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  std::vector<std::thread> thread_pool;
  for (int i = 0; i < number_of_threads; ++i)
  {
    thread_pool.emplace(thread_pool.end(), run);
  }
  std::cout << "All started!" << std::endl;

  for (int i = 0; i < number_of_threads; ++i)
  {
    thread_pool[i].join();
  }
  std::cout << "All finished!" << std::endl;

  std::cout << max_readers << " simultaneous readers!" << std::endl;
  std::cout << "count = " << count[0] << std::endl;

  thread_pool.clear();
  thr_count = 0;
  for (int i = 0; i < number_of_threads; ++i)
  {
    thread_pool.emplace(thread_pool.end(), bench_run);
  }
  std::cout << "All started!" << std::endl;

  for (int i = 0; i < number_of_threads; ++i)
  {
    thread_pool[i].join();
  }
  std::cout << "All finished!" << std::endl;
}
