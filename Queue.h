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

#ifndef QUEUE_H
#define QUEUE_H

#include <deque>
#include <mutex>
#include <memory>
#include <utility>
#include <iterator>
#include <algorithm>

namespace bayolau {
namespace threadsafe {
/**
  * A quick-and-dirty implementation of thread-safe queue.
  *
  * Will be REPLACED by a lock-free version when infrasturcture is done
  */
template<typename T>
struct Queue{
  using DataPtr = std::unique_ptr<T>;

  /**
    * push values to the back of the queue
    * @return true if error occurs
    */
  template<class Iterator>
  bool push(Iterator begin, Iterator end){
    const int64_t num_elements = std::distance(begin,end);
    if(num_elements < 1) return true;
    std::vector<DataPtr> ptrs(num_elements);
    std::transform(begin, end,
                   ptrs.begin(),
                   [](T&&val){
                     return DataPtr(new T(std::forward<T>(val)));
                   });
    std::lock_guard<std::mutex> lg(lk_);
    impl_.insert(impl_.end(),
                 std::make_move_iterator(ptrs.begin()),
                 std::make_move_iterator(ptrs.end()));
    if(num_elements == 1) {
      cv_.notify_one();
    }
    else {
      cv_.notify_all();
    }
    return false;
  }

  /**
    * push value to the back of the queue
    * @return true if error occurs
    */
  bool push(T&& val) {
    DataPtr ptr( new T(std::forward<T>(val)) );
    return push(std::move(ptr));
  }
  bool push(DataPtr&& val) {
    std::lock_guard<std::mutex> lg(lk_);
    impl_.push_back(std::forward<DataPtr>(val));
    cv_.notify_one();
    return false;
  }

  /**
    * pop the front of the queue
    * @return a smart pointer to the popped element if queue is nonempty, NULL otherwise
    */
  DataPtr pop() {
    std::unique_lock<std::mutex> lg(lk_);
    if( impl_.empty()) return DataPtr();
    DataPtr out(std::move(impl_.front()));
    impl_.pop_front();
    return out;
  }

  /**
    * wait until the queue is not empty, then pop
    * @return a smart pointer to the popped element
    */
  DataPtr wait_and_pop() {
    for( std::unique_lock<std::mutex> lg(lk_)
       ; /*loops until return*/
       ; cv_.wait(lg,[this]{return !impl_.empty();})
       ){
      if( !impl_.empty() ){
        DataPtr out(std::move(impl_.front()));
        impl_.pop_front();
        return out;
      }
    }
  }

  /**
    * @true if queue is empty
    */
  bool empty() const {
    std::unique_lock<std::mutex> lg(lk_);
    return impl_.empty();
  }

private:
  std::deque<DataPtr> impl_;
  mutable std::mutex lk_;
  std::condition_variable cv_;
};

}
}

#endif
