#include "sys.h"
#include "microbench/microbench.h"
#include "threadsafe/AIReadWriteSpinLock.h"
#include "debug.h"

#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

#define DIRECT 1

int const number_of_threads = std::thread::hardware_concurrency();
#if DIRECT
int constexpr n0 = 200000;
#else
int constexpr n0 = 150000000;
#endif
int constexpr n1 = 10000;
int constexpr sn = 20;
int constexpr cache_linesize = 64;
int constexpr max_align = alignof(std::max_align_t);
int constexpr cache_line_dist = cache_linesize / max_align;

std::atomic<int> thr_count;
std::atomic<int> wait_in{0};
std::atomic<int> wait_out{0};
std::atomic<int> notify_one_calls{0};

alignas(std::max_align_t) std::atomic_int volatile x{0};        // max_align is only 16 bytes.
std::max_align_t seperation1[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::atomic_int volatile y{0};
std::max_align_t seperation2[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::mutex m;
std::max_align_t seperation3[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::condition_variable cv;
std::max_align_t seperation4[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::atomic_int s_idle;

void bench_mark0();
void bench_mark1();

// Benchmark bench_mark0 while all other threads call bench_mark1.
void bench_run()
{
  Debug(NAMESPACE_DEBUG::init_thread());
  int thr = ++thr_count;

  if (thr > 4)
  {
    for (int j = 0; j < sn; ++j)
      for (int i = 0; i < n1; ++i)
        bench_mark1();
  }
  else
  {
    // Bench mark.
    moodycamel::stats_t stats = moodycamel::microbench_stats(&bench_mark0, n0, sn);

    printf("Thread %d statistics: avg: %.2fns, min: %.2fns, max: %.2fns, stddev: %.2fns, Q1: %.2fns, median: %.2fns, Q3: %.2fns\n",
      thr,
      stats.avg() * 1000000, // From ms to ns.
      stats.min() * 1000000,
      stats.max() * 1000000,
      stats.stddev() * 1000000,
      stats.q1() * 1000000,
      stats.median() * 1000000,
      stats.q3() * 1000000);

    Dout(dc::notice, "wait_in = " << wait_in << "; wait_out = " << wait_out << "; notify_one_calls = " << notify_one_calls);
    Dout(dc::notice, "The average time spend on calling notify_one() was: " << (stats.avg() * 1000000 - 2.1) * 4 * n0 * sn / notify_one_calls << " ns.");
  }
  std::cout << "Thread " << thr << " finished.\n";
}

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "cache_linesize = " << cache_linesize);
  Dout(dc::notice, "max_align = " << max_align);
  Dout(dc::notice, "cache_line_dist = " << cache_line_dist);
  Dout(dc::notice, "&x = " << (void*)&x);
  Dout(dc::notice, "&y = " << (void*)&y);

  assert(alignof(x) >= alignof(std::max_align_t));
  assert(alignof(y) >= alignof(std::max_align_t));
  ssize_t d = (ssize_t)&y - (ssize_t)&x;
  if (d < 0)
    d = -d;
  Dout(dc::notice, "d = " << d);
  assert(d >= cache_linesize);

  std::vector<std::thread> thread_pool;
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

void bench_mark0()                                                      // This function is called 4 * sn * n0 times.
{
#if DIRECT
  notify_one_calls.fetch_add(1, std::memory_order_relaxed);
  cv.notify_one();
#else
  int waiting;
  while ((waiting = s_idle.load(std::memory_order_relaxed)) > 0)        // This line takes 2.1 ns (including function call).
  {
    if (!s_idle.compare_exchange_weak(waiting, waiting - 1, std::memory_order_relaxed, std::memory_order_relaxed))
      continue;
    std::unique_lock<std::mutex> lk(m);
    notify_one_calls.fetch_add(1, std::memory_order_relaxed);           // Count the number of times we get here.
    cv.notify_one();
    break;
  }
#endif
}

void bench_mark1()
{
  std::unique_lock<std::mutex> lk(m);
  s_idle.fetch_add(1, std::memory_order_relaxed);
  wait_in.fetch_add(1, std::memory_order_relaxed);
  cv.wait(lk);
  wait_out.fetch_add(1, std::memory_order_relaxed);
}
