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

#ifndef CPU_TOPOLOGY_H
#define CPU_TOPOLOGY_H

#include "ThreadTopology.h"

namespace bayolau {
namespace affinity {

struct CpuTopology{
  /**
    * only need to do this once, hardware is not going to change
    */
  static const CpuTopology& Instance(){
    static CpuTopology instance; // thread safe in c++11
    return instance;
  }
  /**
    * @return number of cores, discarding hyperthreaded cores
    */
  size_t num_cores() const noexcept { return core_masks_.size(); }
  /**
    * iterate through the provided thread list and set affinity in a round-robin fashion
    */
  void SetAffinity(std::vector<std::thread>& threads) const {
    if( num_cores() == 0) return;
    cpu_set_t cpu_set;
    for(size_t tt = 0 ; tt < threads.size() ; ++tt){
      CPU_ZERO(&cpu_set);
      CPU_SET(core_masks_[tt%core_masks_.size()],&cpu_set);
      pthread_setaffinity_np(threads[tt].native_handle(),sizeof(cpu_set_t),&cpu_set);
    }
  }
  
private:
  std::vector<affinity::ThreadTopology> mask_topology_;
  std::vector<unsigned> core_masks_;

  CpuTopology():mask_topology_(std::thread::hardware_concurrency()){
    const unsigned num_threads = mask_topology_.size();
    std::promise<void> start_signal;
    std::shared_future<void> sf(start_signal.get_future());
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for(unsigned tt = 0 ; tt < num_threads; ++tt){
      threads.emplace_back(LogTopology,sf,std::ref(mask_topology_[tt]));
      cpu_set_t cpu_set;
      CPU_ZERO(&cpu_set);
      CPU_SET(tt,&cpu_set);
      pthread_setaffinity_np(threads[tt].native_handle(),sizeof(cpu_set_t),&cpu_set);
    }
    start_signal.set_value();
    for(auto& entry: threads){
      entry.join();
    }
    core_masks_.clear();
    for(unsigned tt = 0 ; tt < num_threads; ++tt){
      const auto& tp = mask_topology_[tt];
      if( tp.valid() and tp.level_ids().size() > 0 and tp.level_ids().front() == 0 ){
        core_masks_.push_back(tt);
      }
    }
  };

  static void LogTopology(std::shared_future<void> start_signal,bayolau::affinity::ThreadTopology& tp) {
    start_signal.wait();
    try{
      tp.Acquire();
    }
    catch(std::exception& e){
      std::cerr << e.what() << std::endl;
    }
  }

};

}
}

#endif
