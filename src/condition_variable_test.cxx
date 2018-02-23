#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <cstddef>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cmath>
#include <iomanip>

#ifndef DIRECT
#define DIRECT 0
#endif

int const number_of_threads = std::thread::hardware_concurrency();
#if DIRECT
int constexpr n0 = 200000;
#else
int constexpr n0 = 150000000;
#endif
int constexpr sn = 25;
int constexpr cache_linesize = 64;
int constexpr max_align = alignof(std::max_align_t);
int constexpr cache_line_dist = cache_linesize / max_align;

// This doesn't seem to matter that much, but for the sake of precision,
// put all atomic variables in different cache lines.
alignas(std::max_align_t) std::atomic<int> thr_count;
std::max_align_t seperation3[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::mutex m;
std::max_align_t seperation4[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::condition_variable cv;
std::max_align_t seperation5[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::atomic_int s_idle;
std::max_align_t seperation6[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::atomic<int> notify_one_calls{0};
std::max_align_t seperation7[cache_line_dist - 1];              // Put 48 bytes in between.
alignas(std::max_align_t) std::atomic_bool finished{false};
std::mutex cout_mutex;

// Benchmark bench_mark0 when called by four thread, while four other threads call bench_mark1.
void bench_run()
{
  int thr = ++thr_count;

  // Spin until all threads have started.
  while (thr_count.load() != number_of_threads)
    ;

  // The first four threads continuously wait for cv.
  if (thr > number_of_threads / 2)
  {
    while (!finished.load(std::memory_order_relaxed))
    {
      //======================================================================
      // CONSUMER THREADS.
      std::unique_lock<std::mutex> lk(m);                       // Atomically increment s_idle and go into the wait state.
      s_idle.fetch_add(1, std::memory_order_relaxed);           // Requirement: threads seeing this increment also must see the mutex being locked.
      cv.wait(lk);
      //======================================================================

      // Note that the relaxed increment is enough. It is impossible
      // for another thread to see the increment and still be able
      // to obtain the lock (while this thread is still holding the lock;
      // or while this thread didn't take the lock yet).
      //
      // This can be checked by running the following code snippet:
      //
      // int main()
      // {
      //   atomic_int s_idle = 0;
      //   mutex m;
      //   int x = 0;
      //
      //   {{{
      //     {
      //       x = 0;
      //       m.lock();
      //       s_idle.store(1, mo_relaxed);
      //     }
      //   |||
      //     {
      //       s_idle.load(mo_relaxed).readsvalue(1);
      //       {
      //         m.lock();
      //         x = 1;
      //       }
      //     }
      //   }}}
      // }
      //
      // on http://svr-pes20-cppmem.cl.cam.ac.uk/cppmem/
      // showing that if the second thread sees the value 1
      // stored by the first thread, then it will never be
      // able to obtain the lock on m.
    }
  }
  else
  {
    double measurements[sn];
    for (int s = 0; s < sn; ++s)
    {
      auto start = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < n0; ++i)
      {
        //==========================================================================
        // PRODUCER THREADS.
#if DIRECT
        cv.notify_one();
#else
        int waiting;
        while ((waiting = s_idle.load(std::memory_order_relaxed)) > 0)        // This line takes 0.9...0.97 ns.
        {
          if (!s_idle.compare_exchange_weak(waiting, waiting - 1, std::memory_order_relaxed, std::memory_order_relaxed))
            continue;
          notify_one_calls.fetch_add(1, std::memory_order_relaxed);           // Count the number of times we get here.
          // Taking this lock is only necessary when waiting == 1,
          // but adding a test for that makes things only 1% slower
          // due to branch misprediction.
          std::unique_lock<std::mutex> lk(m);
          cv.notify_one();                                                    // This lines turns out to take 19.5 microseconds!
          break;
        }
        //==========================================================================
#endif
      }
      auto end = std::chrono::high_resolution_clock::now();
      measurements[s] = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    double sum_us = 0;
    double min_us = 1e30;
    double max_us = 0.0;
    for (int s = 0; s < sn; ++s)
    {
      sum_us += measurements[s];
      if (measurements[s] > max_us)
        max_us = measurements[s];
      if (measurements[s] < min_us)
        min_us = measurements[s];
    }
    double avg_us = sum_us / ((uint64_t)sn * n0);

    double sum_of_squares_us = 0;
    for (int s = 0; s < sn; ++s)
      sum_of_squares_us += (measurements[s] / n0 - avg_us) * (measurements[s] / n0 - avg_us);

    double avg_ns = 1000 * avg_us;
    double min_ns = min_us / n0 * 1000;
    double max_ns = max_us / n0 * 1000;
    double stddev_ns = std::sqrt(sum_of_squares_us / (sn - 1)) * 1000;

    std::unique_lock<std::mutex> lk(cout_mutex);
    std::streamsize old_precision = std::cout.precision(2);
    std::cout << "Thread " << thr << " statistics: avg: " << std::fixed << avg_ns << "ns, min: " << min_ns << "ns, max: " << max_ns << "ns, stddev: " << stddev_ns << "ns\n";
    std::cout.precision(old_precision);

    std::cout << "The average time spent on calling notify_one() (";
#if DIRECT
    std::cout << ((uint64_t)sn * n0 * number_of_threads / 2);
#else
    std::cout << notify_one_calls;
#endif
    std::cout << " calls) was: ";
#if DIRECT
    std::cout << avg_ns;
#else
    std::cout << (avg_ns - stddev_ns - 0.97) * 4 * n0 * sn / notify_one_calls << " - " << (avg_ns + stddev_ns - 0.90) * 4 * n0 * sn / notify_one_calls;
#endif
    std::cout << " ns.\n";

    finished = true;
  }
  std::cout << "Thread " << thr << " finished." << std::endl;
}

int main()
{
  std::vector<std::thread> thread_pool;
  for (int i = 0; i < number_of_threads; ++i)
    thread_pool.emplace(thread_pool.end(), bench_run);
  std::cout << "All started!" << std::endl;

  for (int i = 0; i < number_of_threads; ++i)
    thread_pool[i].join();
  std::cout << "All finished!" << std::endl;
}
