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
  * A simple RAII of futures, which wait for the future to be ready upon destruction
  * can be more generic with type erasure
  */
struct Futures {
  typedef std::future<void> Future;

  /**
    * destroying Futures will BLOCK until all futures are ready
    */
  ~Futures() { wait(); }

  /**
    * log future
    */
  void log(Future&& f) { futures_.push_back(std::forward<Future>(f)); }

  /**
    * log futures
    */
  void log(Futures&& fs){
    futures_.insert(futures_.end(),
                    std::make_move_iterator(fs.futures_.begin()),
                    std::make_move_iterator(fs.futures_.end()));
  }

  /**
    * wait for all futures to finish
    */
  void wait() {
    for( auto& entry : futures_ ){
      if(entry.valid()) entry.wait();
    }
  }

  /**
    * preallocated for at least n total elements
    */
  void reserve(size_t n) { futures_.reserve(n); }

  Futures& operator+=(Future&& f) { log(std::forward<Future>(f)); }

  Futures& operator+=(Futures&& fs) { log(std::forward<Futures>(fs)); }

  Futures(): futures_() { }

  Futures(const Future&fs) = delete;

  Future& operator=(const Future&fs) = delete;

  Futures(Futures&&fs) { futures_.swap(fs.futures_); }

  Futures& operator=(Futures&&fs) { futures_.swap(fs.futures_); }

private:
  std::vector<Future> futures_;
};


/**
  * A simple thread pool implementation.
  * An instance can has either 1) one thread pinned to one physical core, or 2) number
  * of threads equal to the number of logical cores (with hyperthreading)
  * The scheduler takes a std::function<void(void)> callable as a work unit
  *
  * TODO
  * advanced scheduling will be added
  * exception safty will be flushed out after scheduling changes
  */

class ThreadPool{
  /**
    * a wrapper of functor. We use a non-callable functor as termination signal
    */
  typedef std::packaged_task<void(void)> WorkPackage;
  static bool Terminate(const WorkPackage& wp){ return !wp.valid(); }
public:
  typedef std::function<void(void)> Functor;
  typedef typename Futures::Future Future;

  /**
    * Construct a threadpool
    * @param pin_threads true to pin 1 thread to each physical core, false to create 1 thread per logical core
    */
  ThreadPool(bool pin_threads = true): work_queue_(), threads_(), pinned_(false)
  {
    const unsigned num_threads
        = pin_threads ? CpuTopology::Instance().num_cores() : std::thread::hardware_concurrency();
    threads_.reserve(num_threads);
    std::promise<void> start_flag;
    std::shared_future<void> sf = start_flag.get_future();
    for(unsigned tt = 0 ; tt < num_threads ; ++tt){
      threads_.emplace_back(&ThreadPool::Worker,this,sf);
    }
    if(pin_threads){
      CpuTopology::Instance().SetAffinity(threads_);
      pinned_ = true;
    }
    start_flag.set_value();
    if( ++num_instances() > 1){
      std::cerr << "WARNING: more than one ThreadPool has been instantiated" << std::endl;
    }
  }

  /**
    * register units of work to be run
    */
  template<class Iterator>
  Futures Schedule(Iterator begin, Iterator end) {
    static_assert( std::is_same<typename Iterator::value_type,Functor>::value,
                   "value type must be of Functor type");
    const auto num_elements = std::distance(begin,end);
    if(num_elements < 1) return Futures();

    Futures out; out.reserve(num_elements);
    std::vector<WorkPackage> wps; wps.reserve(num_elements);
    for(auto itr = begin ; itr != end ; ++itr){
      if( static_cast<bool>(*itr) ){
        wps.emplace_back( std::move(*itr) );
        out.log(wps.back().get_future());
      }
    }
    work_queue_.push(std::make_move_iterator(wps.begin()),
                     std::make_move_iterator(wps.end()));
    return out;
  }

  /**
    * register a unit of work to be run
    */
  Future Schedule(Functor work){
    if( !work ) return std::future<void>();
    WorkPackage wp(std::move(work));
    Future out(wp.get_future());
    work_queue_.push(std::move(wp));
    return out;
  }

  /**
    * Try to pop from work queue and work.
    * @return true if a termination signal is not detected
    */
  bool TryWork() {
    auto work_ptr = work_queue_.pop();
    if( not work_ptr )
      return true;
    if( Terminate(*work_ptr) ) { // restore termination signal
      work_queue_.push(std::move(work_ptr));
      return false;
    }
    (*work_ptr)();
    return true;
  }

  /**
    * wait til all tasks are done. will be replaced by futures
    */
  void Wait()const{
    while( !work_queue_.empty() ) {
      std::this_thread::yield();
    }
  }

  /**
    * @return the number of threads
    */
  unsigned num_threads() const noexcept {
    return threads_.size();
  }

  /**
    * @return if the underlying threads are pinned
    */
  bool pinned() const noexcept {
    return pinned_;
  }

  ~ThreadPool() {
    std::vector<WorkPackage> kills(threads_.size());
    work_queue_.push(std::make_move_iterator(kills.begin()),
                     std::make_move_iterator(kills.end()));
    for(auto&entry : threads_){
      entry.join();
    }
    --num_instances();
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator= (const ThreadPool&) = delete;
  //move will require more safety code, keep things simple for now
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator= (ThreadPool&&) = delete;


private:
  threadsafe::Queue< WorkPackage > work_queue_;
  std::vector<std::thread> threads_;
  bool pinned_;

  static std::atomic<unsigned>& num_instances(){
    static std::atomic<unsigned> counter;
    return counter;
  };

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
