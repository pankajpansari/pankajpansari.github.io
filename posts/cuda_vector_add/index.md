---
title:  "Vector Addition - A First Look at CUDA"
description: "A simple program let's us learn a surprising amount about CUDA and GPU"
date:  "2025-06-12"
author: "Pankaj Pansari"
---

In this blog post, we'll write a simple CUDA program to add up elements from 2 arrays on the GPU.
We'll use the `nsys` profiler to identify bottlenecks and do some basic code optimizations.
We'll see how calculating bandwidth and arithmetic intensity helps us think about
the characteristics of our problem (vector-addition here). I'll intersperse some tips I've
found helpful in working with GPU on cloud VM.

This post builds upon the excellent article [An Even Easier Introduction to CUDA](https://developer.nvidia.com/blog/even-easier-introduction-cuda/).

## CPU-version: &Agrave; la CUDA

First, let's write a simple C++ program to add up elements from 2 arrays. It's short and is
in this [gist](https://gist.github.com/pankajpansari/391e5398e5718724f557128d38ac882e#file-vector_add_cpu-cpp). Let's
look at the function that does the main computation: 

```cpp
/* Function to add idx-th element from two arrays (x and y)
 and store it in sum. Written a la CUDA. */
void add_arr(int N, float *sum, float *x, float *y, int idx)
{
    if (idx < N)
      sum[idx] = y[idx] + x[idx];
}
```
We've separated out `add_arr()` from `main()` because this is the part that does the
important computation. We think of this as kernel function and this is the part which has to be 
modified to work on GPU. Note how `add_arr()` operates on `idx`-th index of the arrays which is 
a function parameter. This has a similar flavor to CUDA implementation below 
(hence, &agrave; la CUDA) and makes it straightforward for us to port the C++ implementation to CUDA version.
I got the idea for this from Jeremy Howard's [talk](https://youtu.be/nOxKexn3iBo?si=to39Uv08AjM8J1kE)

:::{.callout-tip}
### Cloud VM Tip
I made use of GPU on a RunPod VM instance. To save money, it's helpful to check correctness of our
implementation on CPU first; hence we wrote the above version. In fact, we can write a draft version
of CUDA code on local machine as well, and then debug it after spinning up the VM.
:::

## CUDA version 

When we rewrite the above program to run on GPU using CUDA, we get the code in this [gist](https://gist.github.com/pankajpansari/391e5398e5718724f557128d38ac882e#file-vector_add_gpu-cu). Note the `.cu` extension rather than `.cpp`. We discuss below the important aspects of this code.

The first thing for us to know is that in CUDA, the CPU is referred to as **host** and GPU as **device**.
However, when we refer to GPU as **device**, the CPU-GPU interaction is very different from, let's say, CPU-I/O device
interaction. It's more helpful to think of GPU as a *coprocessor* - the CPU orchestrates and launches work (kernel)
for the GPU, and the GPU executes these tasks autonomously. That's why a GPU + CPU system is also referred to
as *heterogeneous computing* system.

### Device Code

We have rewritten our kernel function for CUDA as:

```cpp
__global__ void add_arr(int N, float *sum, float *x, float *y)
{
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < N)
      sum[idx] = y[idx] + x[idx];
}
```
The specifier `__global__` is used to annotate this function to say that this will be run on GPU.
This tells the CUDA compiler (`nvcc`) to compile this function to the 
instruction set architecture (ISA) of the GPU called `PTX`. This is different from say the `x86-64` ISA for the CPU.
`nvcc` takes care of compiling `__global__` kernel functions to PTX instructions and passes on the host 
code to an underlying compiler (say `g++`) to be compiled to the CPU ISA. Specifiers like `__global__` and 
`__device__` are also useful to us to parse through the codebase and quickly get an idea as to which functions
will run on which hardware.

#### CUDA Threads

In the GPU execution model, threads are contained within warps; warps are organized within blocks; finally blocks are organized within grid. Please refer this [guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#thread-hierarchy) for a clearer picture of thread hierarchy. CUDA threads are different from CPU threads in that they're more lightweight, the 
hardware (GPU) has a greater role to play in their management rather than software (OS),
and tend to be much more homogeneous in computation than CPU threads.

```cpp
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
```
CUDA makes it possible for each running thread to obtain its unique id. `threadIdx.x` says what's the id of the
this thread within it's block; so this is unique in a block of threads. `blockIdx.x` says what's the id of this
particular block, and `blockDim.x` says how many threads are contained in a single block. This way each thread in
a kernel has its own unique id. This unique id lets each thread know the part of the inputs to act on to produce 
a specific part of the output, avoiding race conditions.


### Host Code

#### Unified Memory

CPU and GPU have separate memories. Pieces of data have to be moved between CPU and GPU memories, either explicitly by us
programmers or implicitly by CUDA runtime. The most important change we need to do to our original CPU-only code in `main()`,
is to allocate arrays `x, y, sum` in memory accessible to GPU.

```cpp
    cudaMallocManaged(&x, size);
```

The CUDA runtime allocates memory pages for `x` in a virtual address space accessible to both CPU and GPU; it's called `Unified Memory`. At this point, physical pages for `x` are not allocated either on GPU or CPU.

```cpp
    for (int i = 0; i < N; i++) {
        x[i] = 1.0f;  // First touch happens on CPU
        y[i] = 2.0f;
    }
```
We're initializing `x` and this runs on CPU because it's in `main()` (if a function specifier is not given, the default is `__host__`).
So CPU touches the memory of `x` first, a page fault occurs, and CUDA runtime allocates page for `x` in CPU main memory. Hence, page allocations happen at runtime, on demand; it's a bit like the way we do memory mapping via `mmap()` on a CPU system.

#### Prefetching

```cpp
    cudaMemPrefetchAsync(x, size, 0, 0); 
```
The kernel needs `x, y, sum` to be resident on GPU memory. By default, when the kernel starts running,
it'll encounter page faults, and memory pages for `x, y, sum` will be migrated by CUDA runtime on demand.\
The problem with this is that running GPU threads have to stall until the pages are migrated, making this inefficient.
The performance gain from prefetching is huge. For my VM with RTX A4000, `nsys` profiler showed that the kernel execution time
decreased from 9.2 ms to 32 microseconds.

However, I feel that if we have to keep track of which object is in which memory at some 
point of time, we're probably better off doing separate allocations on CPU and GPU ourselves and doing explicit copying via `cudaMemcpy()`, rather than using Unified Memory. Also, notice that these prefetches are asynchronous, so copying of `x, y, sum` can happen concurrently. Besides, no explicit synchronization is needed before calling the kernel. 

:::{.callout-tip}
### Cloud VM Tip
Most VM services allow us to select VM templates. Please make sure that the CUDA toolkit that is installed as part of template
is compatible with the GPU driver. This may be an issue with less recent GPUs like RTX A4000.
:::

#### Kernel Call

```cpp
    int blockSize = 256;
    int numBlocks = (N + blockSize - 1) / blockSize;
    add_arr<<<numBlocks, blockSize>>>(N, sum, x, y);
```
For many kernels, it's possible to judiciously select the `blockSize` and `numBlocks` so that kernel execution is optimized.
In this case, we select some plausible `blockSize` and have all blocks in a single grid. The `add_arr()` kernel call is async, which is good because it lets the CPU do some work when the kernel is running. However, if we have to process the results, we have
to explicitly synchronize after kernel call via `cudaDeviceSynchronize()`.

## Bandwidth and Arithmetic Intensity

```cpp
    sum[idx] = x[idx] + y[idx]
```

Each thread in vector addition does 2 reads (`x[idx]` and `y[idx]`) and one write (`sum[idx]`); each thread also
does one math operation (addition here). We say that the arithmetic intensity in such cases is very low. Vector addition
 is bottlenecked by memory read/write rather than math computation.

This shows up in our profiling data as well. Each array of a million floats is of size `4 bytes * 1<<20 = 4 MB`. Our code involves moving around `4 MB * 3 = 12 MB` of data between the global memory of GPU and on-chip SM memory. Because kernel execution for us takes
`32 microseconds`, the effective bandwidth of our implementation is `375 GB/s`. This is about `83%` of RTX A4000's peak bandwidth of `448 GB/s`. Our kernel is limited by how fast data can be moved within the GPU.

:::{.callout-tip}
### Cloud VM Tip
Dotfiles are plain-text files, starting with `.` like `.vimrc`, that configure vim, shell, tmux, git, and other programs.
To quickly customize our VM to feel like our local setup, we can organize dotfiles in a separate folder, under version control,
and use a script to symlink them to `~/`. Here is my dotfiles [repo](https://github.com/pankajpansari/dotfiles), with the setup script.
:::


------------------------------------------------------------------------

*Thanks to Eric for new ideas and inspiration.*