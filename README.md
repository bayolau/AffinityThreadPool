AffinityThreadPool
==================



Description:

This is a header-only, standard-only core-affined threadpool package.

The development is incremental. Right now, it works with only Linux pthread systems with Intel processors with 2xAPIC support. Everything is quick-n-dirty and will be flushed out as time goes on.

The affinity aspect is motivated by performance issues I've seen in the past. There are some cases when core-affinity is critical to performance, but external system/helper threads would swipe around, leading to undesirable thread migration.  While OpenMP 4.0 does support affinity, such setting is done with environmental variable setting. In certain realistic scenarios, an executable might benefit from pinned threads in a section, but benefit from hyperthreading/thread-migration in another.

The standard-only aspect is motivated by "effective" protability. From my past experience, open-source package written for physics and biology are deployed by organizations without DevOps, and adding requirements such as Boost/Cilk/TBB have in the past cause tremendous maintainance headaches.

Usage: see example.cc
```c++
#include "ThreadPool.h"
std::vector<std::function<void(void)> > work;
.
.
bayolau::affinity::ThreadPool threadpool;
auto futures = threadpool.Schedule(work.begin(),work.end());
futures += threadpool.Schedule([]{});
futures.wait();
```

Example output on AWS c3.8xlarge instance:

```
$g++ -std=c++11 -lpthread example.cc -o example
$./example

thread pool has 16 core-affined threads out of 32 hardware threads 

core idenfication 2xAPIC: logical_id core_id package_id ...

worker's core toplogy 4: 0 2 0
worker's core toplogy 10: 0 5 0
worker's core toplogy 4: 0 2 0
worker's core toplogy 12: 0 6 0
worker's core toplogy 10: 0 5 0
worker's core toplogy 14: 0 7 0
worker's core toplogy 8: 0 4 0
worker's core toplogy 0: 0 0 0
worker's core toplogy 6: 0 3 0
worker's core toplogy 2: 0 1 0

distributed sum of 0..9 = 45
```
