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

#include <sstream>
#include <iostream>
#include "ThreadPool.h"

// compile with g++ -std=c++11 -lpthread example.cc -o example

void PrintTopology(std::ostream&os){
  static std::mutex lk;
  bayolau::affinity::ThreadTopology tp;
  try{
    tp.Acquire();
    if(tp.valid()){
      std::stringstream ss;
      ss << "worker's core toplogy " << tp << std::endl;
      std::lock_guard<std::mutex> lg(lk);
      os << ss.str();
    }
  }
  catch(std::exception& e){
    std::cout << e.what() << std::endl;
  }
}

int main (int argc, const char* argv[]){
  bayolau::affinity::ThreadPool::Instance();
  std::cout << "thread pool has "
            <<  bayolau::affinity::ThreadPool::Instance().num_threads()
            << " threads" << std::endl;;
  std::cout << "core idenfication " << bayolau::affinity::ThreadTopology::description() << std::endl;

  for(size_t ii = 0 ; ii < 10 ; ++ii){
    bayolau::affinity::ThreadPool::Instance().Schedule(std::bind(PrintTopology,std::ref(std::cout)));
    std::this_thread::yield();
  }

}
