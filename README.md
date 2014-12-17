Octree
======
Octrees are used to partition (3-dimentional) space. By using an octree, particles physically close to each other are
assigned nearby memory positions.

In the source code, we exploit Pthreads and openMP technologies, as well as Cilk, C's extension, to make the process faster

Process
=======
Creation of an octree is done in 4 steps:
  
  1. A hash code for each point's coordinate is calculated
  2. A [morton code](http://en.wikipedia.org/wiki/Z-order_curve) is computed for each point
  3. Most significant bit radix sort is used to sort the morton codes
  4. Particles are rearranged in memory, so nearby particles are arranged in nearby positions of memory
  
Versions
========

Original - contains the initial code for the assignment

Cilk - implements a few improvements using Intel Cilk (under development)

openMP - implements a parallelized version using openMP - mainly parallel for constructs are used

Pthreads - implements the same parallelization modeled with openMP

Notes
=====
  1. This is a homework assignment for Parallel and Distributed Systems in Aristotle University of Thessaloniki.
  2. To compile the Cilk code, you need [GCC Cilkplus](https://www.cilkplus.org/build-gcc-cilkplus) or icc. The Makefile assumes GCC Cilkplus is in ~/cilkplus-install.
  3. For the rest, normal gcc is used.
