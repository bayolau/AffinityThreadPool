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

#ifndef THREAD_TOPOLOGY_H
#define THREAD_TOPOLOGY_H

#include <stdexcept>
#include <vector>
#include <cstring>

namespace bayolau {
namespace affinity {

struct ThreadTopology{
  /**
      @return a list of id of the hardware thread. The 1st/2nd/3rd entry is typically the HT/core/package ID.
   */
  const std::vector<unsigned>& level_ids() const noexcept { return level_ids_; }
  /**
      @return the 2xAPIC ID of the hardware thread
   */
  unsigned u2xapic() const noexcept { return u2xapic_; }
  /**
      @return true if the instance contains a successfully snap shot of a hardware thread
   */
  bool valid() const noexcept { return valid_; }

  /**
      Acquires 2xAPIC id and derive HT/core/package ID of the hardware thread carrying the execution.
      The function supports only Intel processor with 2xAPIC at this time, and will throw otherwise.
      This method has strong exception guarentee.
   */
  void Acquire() {
    unsigned eax,ebx,ecx,edx;

    RunCpuid(0,0,eax,ebx,ecx,edx);
    if( eax < 11 ) throw std::runtime_error("CPUID does not support 2xAPIC");
    if( strncmp(reinterpret_cast<char*>(&ebx),"Genu",4)) throw std::runtime_error("non-Intel cpu not supported");
    if( strncmp(reinterpret_cast<char*>(&ecx),"ntel",4)) throw std::runtime_error("non-Intel cpu not supported");
    if( strncmp(reinterpret_cast<char*>(&edx),"ineI",4)) throw std::runtime_error("non-Intel cpu not supported");

    // done this way to be more future proof
    std::vector<unsigned> shift_to_next_level(1,0);
    for(unsigned level = 0 ; ; ++level){
      RunCpuid(11,level,eax,ebx,ecx,edx);

      if(level != (ecx&0xFF)) throw std::logic_error("EDX inconsistent with Intel's description");
      const unsigned level_type = ecx>>8 & 0xF;
      if(level_type==0) break;
      shift_to_next_level.push_back(eax&0x1F);
    }
    std::vector<unsigned> loc_level_ids; loc_level_ids.reserve(3);
    for(size_t ii = 0 ; ii + 1< shift_to_next_level.size() ; ++ii) {
      loc_level_ids.push_back(
          (edx & ~(0xFFFFFFFF<<shift_to_next_level[ii+1])) >> shift_to_next_level[ii] );
    }
    if(shift_to_next_level.size()>0){
      loc_level_ids.push_back( edx >> shift_to_next_level.back() );
    }

    level_ids_.swap(loc_level_ids);
    u2xapic_ = edx;
    valid_ = true;
  }

  ThreadTopology():
    level_ids_(),
    u2xapic_(0xFFFFFFFF),
    valid_(false) { }

private:
  std::vector<unsigned> level_ids_;
  unsigned u2xapic_;
  bool valid_;

  void RunCpuid(const unsigned a_in, const unsigned c_in, unsigned& a, unsigned& b, unsigned& c, unsigned& d){
    asm("cpuid" : "=a"(a), "=b"(b), "=c" (c), "=d"(d) : "a"(a_in), "c"(c_in));
  }
};

}
}

#endif
