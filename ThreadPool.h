/*
The MIT License (MIT)

Copyright (c) 2015 Bayo Lau bayo.lau@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef AFFINITY_THREAD_POOL_H
#define AFFINITY_THREAD_POOL_H

#include <thread>
#include <vector>
#include <future>
#include <functional>
#include <atomic>
#include <utility>
#include "Queue.h"
#include "CpuTopology.h"

namespace bayolau {
namespace affinity {

/**
  * A TEMPORARY, quick-and-dirty thread pool
  * affinity will be added
  * exception safty will be enforced
  * advanced scheduling will be added
  */

struct ThreadPool{

/**
  * return the singleton instance, in my experience non-singleton would at
  * some point lead to accidental nesting, leading to subtle performance
  * decay, or even worst violating the number of threads set by ulimit
  */
  static ThreadPool& Instance() {
    static ThreadPool instance; // this allocation is threadsafe for post-C++11 compiler
    return instance;
  }

/**
  * register a unit of work to be run
  */
  void Schedule(std::function<void(void)> work){
    work_queue_.push(work);
  }

/**
  * register a unit of work to be run
  */
  unsigned num_threads() const noexcept {
    return threads_.size();
  }

  ~ThreadPool() {
    finished_=true;
    for(auto&entry : threads_){
      entry.join();
    }
  }
private:
  std::atomic<bool> finished_;
  bayolau::threadsafe::Queue< std::function<void(void)> > work_queue_;
  std::vector<std::thread> threads_;
  std::promise<void> start_flag_;

  ThreadPool()
    : finished_(false)
    , work_queue_()
    , threads_()
    , start_flag_()
  {
    const unsigned num_threads = bayolau::affinity::CpuTopology::Instance().num_cores();
    threads_.reserve(num_threads);
    std::shared_future<void> sf = start_flag_.get_future();
    for(unsigned tt = 0 ; tt < num_threads ; ++tt){
      threads_.emplace_back(&ThreadPool::Worker,this,sf);
    }
    bayolau::affinity::CpuTopology::Instance().SetAffinity(threads_);
    start_flag_.set_value();
  }

  void Worker(std::shared_future<void> start) {
    start.wait();
    while(!finished_ or !work_queue_.empty()) {
      auto work_ptr = work_queue_.pop();
      if(work_ptr) {
        (*work_ptr)();
      }
      else {
        // there can be other threads, eg helpers, running
        std::this_thread::yield();
      }
    }
  }
};

}
}

#endif
