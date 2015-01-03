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

#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <iterator>

namespace bayolau {
namespace util {

template<class Iterator> struct FilteredIterator;

/**
  * @return a pair of iterators over a [begin,end) range, skipping elements evaluated to false for the predicate
  */
template<class Iterator, class Predicate>
std::pair<FilteredIterator<Iterator>,FilteredIterator<Iterator> >
    FilteredIterators(Iterator begin, Iterator end, Predicate pred);

/**
  * A converted iterator over [begin,end), skipping elements evaluted to false
  * according to a predicate
  */
template<class Iterator>
struct FilteredIterator{
  typedef Iterator iterator_type;
  typedef typename std::forward_iterator_tag iterator_category;
  typedef typename std::iterator_traits<Iterator>::value_type value_type;
  typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;

  typedef std::function<bool(const value_type&)> Predicate;

  FilteredIterator(Iterator curr, Iterator end, Predicate p)
    : curr_(curr), end_(end), pred_(p) {}

  reference operator*() { return *curr_; }

  FilteredIterator& operator++() {
    for(++curr_ ; curr_!=end_ and not pred_(*curr_) ; ++curr_) { }
    return *this;
  }

  bool operator!=(const FilteredIterator& other){
    return curr_ != other.curr_;
  }
private:
  Iterator curr_, end_;
  Predicate pred_;
};

/**
  * return a pair of iterators over a [begin,end) range, skipping elements evaluated to
  * false for the predicate
  */
template<class Iterator, class Predicate>
std::pair<FilteredIterator<Iterator>,FilteredIterator<Iterator> >
    FilteredIterators(Iterator begin, Iterator end, Predicate pred){
  for( ; begin!=end and not pred(*begin) ; ++begin) { }
  for( ; begin!=end and begin + 1 != end and not pred(*(end - 1)) ; --end) { }
  return std::make_pair( FilteredIterator<Iterator>(begin,end,pred),
                         FilteredIterator<Iterator>(end,end,pred));
}

}
}

#endif
