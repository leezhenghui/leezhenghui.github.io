---
layout: post
title: CallTracer 
categories: [linux]
tags: [linux, c/c++, debug, toolkit]
fullview: true
comments: true
---

# CallTracer

[`CallTracer`](https://github.com/leezhenghui/calltracer) is an instrument toolkit and aimed to provide an easy way for native(C/C++) program debugging, in particular, it can work as an utility to record and layout the program execution details in various straight-forward representation ways.(e.g: sequencing diagram, flamegraph)

It is compiled as a shared-lib and linked into a C/C++ program to enable the tracer. So far, it provides an user-friendly integration features for the program which is built upon `gyp` or `waf`, and also provides an out-of-box utility tool to convert the call stack into **file names** and **source code line**, finally be converted and presented by an appropriate visualizer, including: [seqdiag](http://blockdiag.com/en/seqdiag/), [diagrams](https://github.com/francoislaberge/diagrams) and [flamegraph](https://github.com/brendangregg/FlameGraph)(default). 

## Features

- Executable ELF (non-pie)

- Static library

- Dynamic linking library

- Dynamic loading library

- Tracing forked process

- Multiple threads

- Seqdiag

- Diagrams

- Flamegraph

> ![Tips]({{ site.url }}/assets/ico/tip.png)
>
> If you are familiar with [`flamegraph`](https://github.com/brendangregg/FlameGraph), you might know about it originally is used for sampling data without an order guarantee. In calltracer, the generated flamegraph are for tracer data, and we will ensure the call stack sequencing following the tracer generated order. 

Notable, turning on the func-trace will introduce significant performance impact,  please avoid using it on a production environment. 

## TODO

- Dynamic Tracer support, enable/disable the tracer on-the-fly

- Support `dlclose` 

- Code refine/refactor

## Prerequisites for run the example

- Linux OS (Tested on Ubuntu variants)

- Have `addr2line` command installed on your system

- Have `node.js` runtime on your environment

- Have `seqdiag` command installed if you want to generate seqdiag style sequencing diagram

## Example

The sample is just used to demonstrate the usages of the tool. To make the sample cover mores situations, e.g: `executable ELF`, `static-lib`, `dynamic linking shared-lib`, `dynamic loading shared-lib`, `forked process` and `multiple-threads`, I am trying to split the sample into various modules with different lib types, this definitely does not make a sense in a real-life program.

![Sample Components]({{ site.url }}/assets/materials/calltracer/example-design.jpeg)


## How to run

The project is using GYP as the compile tool.

```sh
git clone https://github.com/leezhenghui/calltracer.git 
git submodule update --init

make clean
make 
make run-debug 

```

Using below command to conver the trace log into a visualizer view:

```
  cd ./tools/iseq/ 
  npm install 
  ./tools/iseq/iseq

```

## Visualizer 

### FlameGraph (default)

![FlameGraph Example]({{ site.url }}/assets/materials/calltracer/example-flamegraph.svg)

> ![Tips]({{ site.url }}/assets/ico/tip.png)
>
> Open the original image and click(zoon in) the flame block you are interested in and get more detailed invocation information followed by that point, e.g: invocation seq, invocation occurs timestamp, file name and source code line. 

### Seqdiag

<img src="{{ site.url }}/assets/materials/calltracer/example.png" width="800" height="500">


### Diagrams 

<img src="{{ site.url }}/assets/materials/calltracer/example.svg" width="800" height="500">

## Join us

If you are interested in this project, please feel free to let me know,  any bug-report/comments/suggestions/contribution on this project is appreciated. :-)

