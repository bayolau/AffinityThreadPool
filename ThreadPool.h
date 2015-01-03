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
#include "util.h"

namespace bayolau {
namespace affinity {
/**
  * A quick-and-dirty thread pool implementation.
  * A singleton is intialized with one thread pinned to one physical core.
  * The scheduler takes a std::function<void(void)> callable as a work unit
  *
  * TODO
  * advanced scheduling will be added
  * exception safty will be flushed out after scheduling changes
  */

class ThreadPool{
  /**
    * a wrapper, if Functor. We use a non-callable functor as termination signal
    */
  typedef typename std::function<void(void)> WorkPackage;
  static bool Terminate(const WorkPackage& wp){ return !static_cast<bool>(wp); }
public:
  typedef WorkPackage Functor;
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
    * register units of work to be run
    */
  template<class Iterator>
  void Schedule(Iterator begin, Iterator end) {
    static_assert( std::is_same<typename Iterator::value_type,Functor>::value,
                   "value type must be of Functor type");
    auto begin_end = util::FilteredIterators(begin,end,
                         [](const typename Iterator::value_type&val)
                           {return static_cast<bool>(val);});
    work_queue_.push(begin_end.first,begin_end.second);
  }

  /**
    * register a unit of work to be run
    */
  void Schedule(Functor work){
    work_queue_.push(WorkPackage(work));
  }

  /**
    * register a unit of work to be run
    */
  unsigned num_threads() const noexcept {
    return threads_.size();
  }

  ~ThreadPool() {
    std::vector<WorkPackage> kills(threads_.size());
    work_queue_.push(kills.begin(),kills.end());
    for(auto&entry : threads_){
      entry.join();
    }
  }
private:
  threadsafe::Queue< WorkPackage > work_queue_;
  std::vector<std::thread> threads_;

  ThreadPool(): work_queue_(), threads_()
  {
    const unsigned num_threads = CpuTopology::Instance().num_cores();
    threads_.reserve(num_threads);
    std::promise<void> start_flag;
    std::shared_future<void> sf = start_flag.get_future();
    for(unsigned tt = 0 ; tt < num_threads ; ++tt){
      threads_.emplace_back(&ThreadPool::Worker,this,sf);
    }
    CpuTopology::Instance().SetAffinity(threads_);
    start_flag.set_value();
  }

  void Worker(std::shared_future<void> start) {
    start.wait();
    for(bool work = true ; work ; ){
      auto work_ptr = work_queue_.wait_and_pop();
      work = !Terminate(*work_ptr);
      if(work){
        (*work_ptr)();
      }
    } 
  }
};

}
}

#endif
