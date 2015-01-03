AffinityThreadPool
==================

This is a header-only, standard-only threadpool package which supports affinity. The development is incremental, right now, it works with Linux pthread systems with Intel processors with 2xAPIC support. Everything is quick-n-dirty and will be flushed out when I have time.

The standard-only aspect is motivated by "effective" protability. From my past experience, open-source package written for physics and biology are deployed by organizations without DevOps, and adding requirements such as Boost/Cilk/TBB have in the past cause tremendous maintainance headaches.

The affinity aspect is motivated by performance issues I've seen in the past. There are some cases when core-affinity is critical to performance, but external helper threads would swipe around, leading to thread migration.  While OpenMP 4.0 does support affinity, such setting is done with environmental variable setting. In certain realistic scenarios, a executable might benefit from pinned threads in a section, but benefit from hyperthreading/thread-migration in another.

Of course, the main motivation is that this is something I've been wanting to write for the sake of it for a long time.

EXAMPLE:
$g++ -std=c++11 -lpthread example.cc -o example

$./example

thread pool has 2 threads

core idenfication 2xAPIC: logical_id core_id package_id ...

worker's core toplogy 0: 0 0 0

worker's core toplogy 2: 0 1 0
