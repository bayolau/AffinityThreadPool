AffinityThreadPool
==================

Usage: needs to include only ThreadPool.h, see example.cc

$g++ -std=c++11 -lpthread example.cc -o example

Description:

This is a header-only, standard-only core-affined threadpool package.

The development is incremental. Right now, it works with only Linux pthread systems with Intel processors with 2xAPIC support. Everything is quick-n-dirty and will be flushed out as time goes on.

The affinity aspect is motivated by performance issues I've seen in the past. There are some cases when core-affinity is critical to performance, but external system/helper threads would swipe around, leading to undesirable thread migration.  While OpenMP 4.0 does support affinity, such setting is done with environmental variable setting. In certain realistic scenarios, an executable might benefit from pinned threads in a section, but benefit from hyperthreading/thread-migration in another.

The standard-only aspect is motivated by "effective" protability. From my past experience, open-source package written for physics and biology are deployed by organizations without DevOps, and adding requirements such as Boost/Cilk/TBB have in the past cause tremendous maintainance headaches.

Of course, the main motivation is that this is something I've been wanting to write for the sake of it for a long time.

