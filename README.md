AffinityThreadPool
==================

This is a header-only, standard-only threadpool package which supports affinity.

A motivation is to fix core-affinity to avoid performance penalty. There are some cases when core-affinity is critical to performance, but external helper threads would swipe around, leading to thread migration.

While non-standard packages such as OpenMP 4.0 does support affinity, such setting is done with environmental variable setting. The consequence is the lost of control over taking advantage of hyperthreading/thread-migration in one part of the code while taking advantage of thread-pinning in another.
