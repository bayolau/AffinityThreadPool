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

#include <atomic>
#include <thread>
#include <mutex>
#include <set>
#include <map>
#include <iostream>
#include "ThreadTopology.h"

/*
  This file is SCRAP work, quick and dirty stuff to test code.
*/

template<typename T>
std::string BitPattern(T in){
  static_assert(std::is_integral<T>::value
               ,"works on integer for now to prevent reinterpret cast");
  std::string out(8*sizeof(T),'0');
  for( T ii = 0, mask = 1 ; ii < 8*sizeof(T) ; ++ii, mask<<=1 ){
    if( mask & in ){ out[out.size()-1-ii]='1'; }
  }
  return out;
}

std::mutex lk;
std::set<int> collection;
std::atomic<unsigned> n_logged;
std::vector<bayolau::affinity::ThreadTopology> topologies;

void LogTopology() {
  bayolau::affinity::ThreadTopology tp;
  while(n_logged.load() != std::thread::hardware_concurrency()){
    try{
      tp.Acquire();
      if(tp.valid()){
        std::lock_guard<std::mutex> lg(lk);
        size_t old_size = collection.size();
        collection.insert(tp.u2xapic());
        if( collection.size() != old_size) {
          std::cout<<tp.u2xapic()<<":";
          for(auto entry: tp.level_ids()){
            std::cout << entry << " ";
          }
          std::cout<<"\n";
          topologies.push_back(tp);
          ++n_logged;
        }
      }
    }
    catch(...){
    }
  }
}

int main (int argc, const char* argv[]){
  std::vector<std::thread> threads;
  threads.reserve(std::thread::hardware_concurrency());
  for(unsigned tt = 0 ; tt < std::thread::hardware_concurrency() ; ++tt){
    threads.emplace_back(LogTopology);
  }

  for(auto& entry: threads){
    entry.join();
  }

  std::map<std::tuple<unsigned,unsigned,unsigned>,std::vector<unsigned> > log;
  for(const auto& entry: topologies){
    log[ std::make_tuple(entry.level_ids()[0],entry.level_ids()[1],entry.level_ids()[2]) ]
        .push_back( entry.u2xapic());
  }

  for(const auto& entry: log){
    std::cout << std::get<0>(entry.first) << " "
              << std::get<1>(entry.first) << " "
              << std::get<2>(entry.first) << " (" << entry.second.size() << ")";
    for(const auto& apic: entry.second){ std::cout << " " << apic; }
    std::cout << std::endl;
  }
}
