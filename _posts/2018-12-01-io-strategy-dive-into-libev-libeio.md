---
layout: post
title: Boost I/O Strategy In Web Server - Dive into Libev and Libeio 
categories: [io-strategy]
tags: [I/O, linux, perf, eventloop]
series: [io-strategy]
comments: true
---

{% include common/series.html %}

## Table of Contents

* Kramdown table of contents
{:toc .toc}

It has been a little bit long time since my [last post](https://leezhenghui.github.io/io-strategy/2017/11/11/io-strategy-motivation.html) for this series. My apologize for the later update. 

Following my original plan, the 2nd post in this series should be an introduction of I/O models followed by recap of [c10k](https://en.wikipedia.org/wiki/C10k_problem) problem, and after that we will explore some of well-used frameworks/products based on different programming languages and take a closer look at the details from implementation perspective. Recently, I was invited to give a presentation on the topic of libev based eventloop in our team, this change my mind on the order, so we see this topic comes here. The content in this post is more suitable to the audiences who already have the backgroud knowledge of system I/O models(Blocking, Non-Blocking, Sync and Async), reactor pattern and some system programming experiences.

## Libev

libev - a high performance full-featured event-loop written in C.

> From [man](https://linux.die.net/man/3/ev)
>
> Libev supports "select", "poll", the Linux-specific "epoll", the BSD-specific "kqueue" and the Solaris-specific event port mechanisms for file descriptor events ("ev\_io"), the Linux "inotify" interface (for "ev\_stat"), Linux eventfd/signalfd (for faster and cleaner inter-thread wakeup ("ev\_async")/signal handling ("ev\_signal")) relative timers ("ev\_timer"), absolute timers with customised rescheduling ("ev\_periodic"), synchronous signals ("ev\_signal"), process status change events ("ev\_child"), and event watchers dealing with the event-loop mechanism itself ("ev\_idle", "ev\_embed", "ev\_prepare" and "ev\_check" watchers) as well as file watchers ("ev\_stat") and even limited support for fork events ("ev\_fork").

### Components architecture

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-overall-clean.png" alt="multiple-stakholders">

- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/convertor.png" alt="convertor" width="30" height="30">

  The convertor in above diagram is used to represent the mechanisms which transform a non-pollable resource to a pollable operation fashion, such that be able to adopte to the event-loop processing.

  The non-pollable resoruces would be:

    - The target file fd is not supported by the underlaying demultiplexer(in libev code,we call it `backend`), e.g: the regular disk file is not supported by epoll in linux.

      > ![Tips]({{ site.url }}/assets/ico/tip.png)
      >
      > For the fd of regular disk file, it can't work with epoll, a EPERM error will returned if you do that. BTW, `poll` and `select` based backend can support disk file fd, but the read/write operation actually running in a blocking I/O mode, that is definitely we need to be aware of, and in a single thread event-loop application, we need to avoid this kind of usage, as it may potentially introduce "world-stop" due to the I/O blocking operation(espcially for a disk file `read` call, I guess `write` operation may be better than `read` operation in this case, as it just put the data to memory buffer and return), and slow down the whole iteration of event processing.

    - The resources run in an ultimate async manner,  adopt to the pollable mode. e.g: signal.
 
- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/blockingio-convertor.png" alt="blockingio-convertor" width="110" height="50">
  
  For regular disk file fd, which does not work in a real non-blocking manner, it is not supported by epoll directly. To unify the programming interface shape for all of fd, libev make some additional efforts to adopt it on epoll based demultiplexer, including provide fault-tolerance logic for EPERM" error and handle these I/O fds in a separate data model and put them to event-loop queue.(please refer to code line 120 and line 222 for more details).  Please keep in mind, run disk file fd in this way will get a poor performance. AFAIK, the best practice on blocking I/O operation is, convert it to an async fashion operation(either via multi-thread or signal) and adopt to event-loop processing via a `eventfd` or `pipe` file.

- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/signal-convertor.png" alt="signal-convertor" width="110" height="50">

  There are two kinds of underlying techologies can be used in the convertor to adopt signal to the event-loop processing.

    - [A] signalfd

    - [B] self-pipe pattern, using either an eventfd or built-in pipe.

      With self-pipe pattern, one side of pipe is written by by an async resource handler, an other side is polled by the backend in the loop.

- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/child-convertor.png" alt="child-convertor" width="110" height="50">

  Unix signals child process termination with `SIGCHLD`, the ev\_child actually initail a ev\_signal and listen on SIGCHLD, generate/consume child event accordingly. In libev implementation (L2466, childcb method), try to avoid in-lined loop in the callback method, instead puting a sig\_child event to event pending queue to make sure the childcb are called again in the same tick until all children have been reaped. Refer to the workflow for the details in later of this post. 

- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/async-convertor.png" alt="async-convertor" width="110" height="50">

  ev\_async provide a generic way to enable an naturally async source(e.g: the source is triggerred by signal or running in an other thread) adopt to the event\_loop processing. Under the hood, the convertor actually leverage self-pipe pattern(either eventfd or built-in pipe) to achieve the transformation.

- <img src="{{ site.url }}/assets/materials/inside-libev-libeio/fsstat-convertor.png" alt="fsstat-convertor" width="110" height="50">

  There two approaches to get stat of a file:

  - [A] inotify, it is suitable to local file system, and linux kernel version >= 2.6.25

  - [B] timer based, it is used by remote file system or linux kernel version < 2.6.25

### Framework Workflow 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/eventloop-overall.png" alt="multiple-stakholders">

### Data sturcture

#### Backend

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-data-structure-backend.png" alt="multiple-stakholders">

#### Timer

Libev is using heap data structure to achieve the efficient query/store on ordered elements. 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/heaps.bmp" alt="heap">

#### Watcher and Pending Queue 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/inside-nodejs-libev-data-struct.png" alt="heap">

### Event Workflow

#### EV\_IO

- Non-Blocking I/O

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-pipe.png" alt="multiple-stakholders">

- Blocking I/O

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-disk-file.png" alt="multiple-stakholders">

#### EV\_Timer

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-timer.png" alt="multiple-stakholders">

#### EV\_SIGNAL

- Approach-A(signalfd)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-signal-signalfdbased.png" alt="multiple-stakholders">

- Approach-B(built-in pipe pattern)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-signal-pipebased.png" alt="multiple-stakholders">

#### EV\_Async

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-async.png" alt="multiple-stakholders">

#### EV\_CHILD

- Approach-A(signalfd)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-child-signalfdbased.png" alt="multiple-stakholders">

- Approach-B(built-in pipe pattern)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-child-pipebased.png" alt="multiple-stakholders">

#### EV\_Stat

- Approach-A(inotify)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-stat-inotifybased.png" alt="multiple-stakholders">

- Approach-B(ride on timer)

  <img src="{{ site.url }}/assets/materials/inside-libev-libeio/libev-workflow-stat-timerbased.png" alt="multiple-stakholders">


## Libeio

Libeio is event-based fully asynchronous I/O library for C

> From [libeio website](http://software.schmorp.de/pkg/libeio.html) 
>
> Libeio is a full-featured asynchronous I/O library for C, modelled in similar style and spirit as libev. Features include: asynchronous read, write, open, close, stat, unlink, fdatasync, mknod, readdir etc. (basically the full POSIX API). sendfile (native on solaris, linux, hp-ux, freebsd, emulated everywehere else), readahead (emulated where not available).

### Breif workflow 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libeio-overall.png" alt="multiple-stakholders">

Briefly, libeio will wrap a sync-style operation and leverage userland thread pool to simulate an async invocation manner.

### Components Architecture 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/libeio-workflow.png" alt="multiple-stakholders">

### POSIX.AIO 

POSIX.AIO provides an asynchronous I/O interface implemented as part of glibc. It achieves this via using userland thread pool internally. But seems it is not good enough. If using callback function as completion notifiction, POSIX.AIO will spawn a new thread to do the callback. Imaging in a high concurrency case, each callback function contains a long running logic, this design will draw us back to request-per-thread mode. On the contrary, Libeio is implemented in the polling natural design. As you can see above, the callback function is executed in the original thread, instead of the thread spwaned by libeio, in such way, libeio can fully control the thread pool size and lifecycle.

BTW, besides callback notification, POSIX.AIO also supports signal notificaiton, but that also have some problems, if you are interested in this topic, please refer to [link](http://davmac.org/davpage/linux/async-io.html) for more details.

## Integration of Libev and Libeio

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/inside-nodejs-conjuction-libev-libeio.png" alt="multiple-stakholders">

## High level design of network stream abstraction layer based on libev 

> ![Note]({{ site.url }}/assets/ico/note.png)
>
> Notable, Libev is running on epoll backend in **level-triggered** mode. But the workflows described below actually are intented to adopt to epoll technology with **edge-triggered** mode.

### Inbound

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/adopt-inbound-flow-in-ev.png" alt="multiple-stakholders">

### Outbound

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/adopt-outbound-flow-in-ev.png" alt="multiple-stakholders">

## Appendix

### Detailed execution stack 

#### init

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_init.svg" alt="multiple-stakholders">

#### Timer 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_timer.svg" alt="multiple-stakholders">

#### Stdio

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_stdin.svg" alt="multiple-stakholders">

#### Regular File(a.k.a disk file)

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_diskfile.svg" alt="multiple-stakholders">

#### Async 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_async.svg" alt="multiple-stakholders">

#### Child 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_child.svg" alt="multiple-stakholders">

#### Fork 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_fork.svg" alt="multiple-stakholders">

#### Local file stat(inotify) 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_stat.svg" alt="multiple-stakholders">

#### Remote file stat 

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_remote_stat.svg" alt="multiple-stakholders">

#### Signal(w/ eventfd)

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_signal_eventfd.svg" alt="multiple-stakholders">

#### Signal(w/ pipe)

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_signal_pipe.svg" alt="multiple-stakholders">

#### Signal(w/ signalfd)

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph_signalfd.svg" alt="multiple-stakholders">

#### Libeio(custom-handler)

<img src="{{ site.url }}/assets/materials/inside-libev-libeio/flamegraph-libeio_custom.svg" alt="multiple-stakholders">

{% include common/series.html %}
