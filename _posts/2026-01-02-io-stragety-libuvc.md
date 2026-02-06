--- 
layout: post
title: IO Strategy - The internals of Libuvc
categories: [io-strategy]
tags: [I/O, linux, libuv, coroutine, vibe-coding]
series: [io-strategy]
comments: true
---

{% include common/series.html %}

## Table of Contents

* Kramdown table of contents
{:toc .toc}

It has been a long time since my [last post](https://leezhenghui.github.io/io-strategy/2018/12/01/io-strategy-dive-into-libev-libeio.html) in this series. In the previous articles, we explored the depths of `libev` and `libeio`. Today, we are not just analyzing the famous `libuv` (the spiritual successor to the libev+libeio combo), but exploring a derivative project born from a "Vibe Coding" experiment: **Libuvc**.

## Background: The "Vibe Coding" Experiment

`libuv` is the powerhouse behind Node.js, providing a cross-platform asynchronous I/O abstraction. While `libev` and `libeio` laid the groundwork, `libuv` unified them into a robust production-grade library.

However, `libuv`'s maturity means strict API/ABI stability, making it hard to experiment with radical architectural changes like pluggable coroutines or mixing modern Linux features like `io_uring` into the core loop.

**Libuvc** is a learning project created in a 2-day immersive "Vibe Coding" session (AI-assisted pair programming). The goal was to verify if AI agents could assist in complex system-level programming tasks—specifically, refactoring a 50k+ LOC C codebase into a modular, coroutine-native runtime.

The result is `libuvc`: a stripped-down, highly modular version of `libuv` that serves as a playground for advanced I/O strategies.

## Feature Highlights

Libuvc focuses on two transformative features that distinguish it from the classic libuv:

1.  **Modular & Pluggable Design (SPI-driven)**: Unlike the monolithic nature of traditional event libraries, Libuvc abstracts IO backends and File System engines into Service Provider Interfaces (SPIs). This allows for swapping or fallback between implementations (e.g., `epoll`, `io_uring`, `kqueue`) without modifying the core loop logic.
2.  **Sync-over-Async Style API**: By leveraging lightweight coroutines, the library transforms complex asynchronous callback chains into a natural, linear programming flow. Developers get the high-concurrency benefits of non-blocking I/O with the code readability of synchronous logic, with the request context implicitly managed on the coroutine stack.

## Architecture: Modularization & Flexibility

The core philosophy of `libuvc` is **pluggability**. Unlike standard `libuv` where backends are largely determined at compile-time by the OS, `libuvc` introduces strict SPIs (Service Provider Interfaces) for key components.

### Visual Comparison: From Monolithic to Modular

Below are the architecture diagrams illustrating the shift from the traditional `libuv` structure to the modular `libuvc` design.

**Before: Traditional Libuv Architecture**

<img src="{{ site.url }}/assets/materials/libuvc/libuv.png" alt="Libuv Architecture" width="900">

**After: Libuvc (Modular & Coroutine-native)**

<img src="{{ site.url }}/assets/materials/libuvc/libuvc.png" alt="Libuvc Architecture" width="900">

### 1. The Matrix of Backends

We decoupled the event loop and the file system engine from their specific implementations. While the **IO Backend** is explicitly selected (or auto-detected) during loop initialization without runtime fallback, the **Async-FS Engine** is designed with a fallback mechanism (e.g., automatically falling back to `threadpool` if `io_uring` is unavailable or fails).

| Platform | Loop Backend | Async-FS Engine | Coroutine Provider |
| :--- | :--- | :--- | :--- |
| **Linux** | `epoll`, `iouring` | `threadpool`, `iouring` (w/ fallback) | `ucontext`, `fcontext`, `photon`  |
| **macOS** | `kqueue` | `threadpool` | `ucontext`, `fcontext`, `photon` |

This design allows for interesting combinations. For example, on Linux, you can run:
*   **Legacy Mode:** `epoll` (Network) + `threadpool` (File I/O).
*   **Modern Mode:** `epoll` (Network) + `iouring` (File I/O).
*   **Bleeding Edge:** `iouring` (Network + File I/O).

### 2. "One Loop, One Wait Point"

A critical design principle in `libuvc` is preserving the "One Loop" property. Even when mixing `epoll` for networking and `io_uring` for file system operations, we ensure there is only **one wait point** (blocking call) in the event loop.

If `io_uring` is used for FS but `epoll` for network, the `io_uring` completion queue is treated as just another event source polled by the main loop, ensuring we don't block on two different file descriptors.

## Coroutines: Sync-over-Async

The "C" in `libuvc` stands for Coroutines. Writing callback-heavy code in C (as seen in standard `libuv`) often leads to "Callback Hell" and complex context management (`req->data`).

`libuvc` introduces a "Sync-over-Async" layer. It wraps the asynchronous `uv_*` callbacks into synchronous-looking `uvc_*` APIs using coroutines (currently `ucontext` or `fcontext`).

### Code Comparison: Ping-Pong Server

Let's look at the difference in style.

**Classic Libuv (Callback Style)**

In standard `libuv`, you have to manually manage state structs, chain callbacks, and handle memory lifecycles carefully across async boundaries.

```c
/* Standard libuv: Context passing hell */
typedef struct {
  uv_tcp_t handle;
  uv_write_t write_req;
  int count;
} server_client_t;

static void on_server_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  server_client_t* cli = stream->data; // Retrieve context
  
  if (nread > 0) {
    cli->count++;
    uv_write(&cli->write_req, stream, &resp, 1, after_server_write);
    return;
  }
  // ... Error handling and freeing memory is manual and scattered
}
```

**Libuvc (Coroutine Style)**

In `libuvc`, the logic is linear. The `uvc_read` and `uvc_write` calls yield execution to other coroutines while waiting for I/O, but look synchronous to the programmer.

```c
/* Libuvc: Linear, "Sync" flow */
static void echo_conn(void* arg) {
  uv_tcp_t* handle = arg;
  char buf[16];
  ssize_t nread, nwritten;

  for (int i = 0; i < 3; i++) {
    // Looks blocking, but actually yields!
    if (uvc_read((uv_stream_t*) handle, buf, sizeof(buf), 1000, &nread) != 0) break;
    
    printf("got: %.*s\n", (int)nread, buf);
    
    uvc_write((uv_stream_t*) handle, &sendbuf, 1, 1000, &nwritten);
  }

  uvc_close_wait((uv_handle_t*) handle); // Clean shutdown
  free(handle);
}
```

### Key API Evolution

The transformation from `libuv` to `libuvc` isn't just about syntax; it's about inverting the control flow. Here is a mapping of the critical APIs:

| Operation | Libuv (Async/Callback) | Libuvc (Sync-Style/Coroutine) | Change Highlight |
| :--- | :--- | :--- | :--- |
| **Connect** | `uv_tcp_connect(req, handle, addr, cb)` | `uvc_connect(handle, addr, timeout, &status)` | Removes `uv_connect_t` req; adds `timeout`; returns status inline. |
| **Accept** | `uv_accept(server, client)` (inside `uv_listen` cb) | `uvc_accept(server, client, flags, &status)` | Can be called in a loop; blocks coroutine until connection arrives. |
| **Read** | `uv_read_start(stream, alloc_cb, read_cb)` | `uvc_read(stream, buf, len, timeout, &nread)` | No `alloc_cb` needed; reads directly into user buffer; returns bytes read. |
| **Write** | `uv_write(req, stream, bufs, n, cb)` | `uvc_write(stream, bufs, n, timeout, &nwritten)` | Removes `uv_write_t` req; waits for flush/ack. |
| **Sleep** | `uv_timer_start(timer, cb, timeout, 0)` | `uvc_sleep(timeout)` | No timer handle management; simple "sleep" semantics. |
| **Close** | `uv_close(handle, close_cb)` | `uvc_close_wait(handle)` | Blocks until the handle is fully closed and resources released. |

**The Semantic Shift:**
1.  **Removal of `req` structs:** In `libuv`, almost every operation needs a `uv_req_t` to track the request lifecycle. In `libuvc`, this state is implicitly managed by the coroutine stack.
2.  **Timeout Support:** Most `uvc_*` APIs accept a `timeout` parameter natively, whereas in `libuv`, implementing timeouts for individual operations (like a specific write) requires auxiliary `uv_timer_t` handles.
3.  **Return Values:** Instead of receiving status codes in callbacks, `uvc` functions populate status pointers (e.g., `&nread`, `&status`) and return control only when the operation completes (or fails).

## Cross-Platform Development: The Mocking Story

One of the biggest challenges in system programming is developing Linux-specific features (like `epoll` or `io_uring`) on a macOS machine (the "Vibe Coding" environment). We implemented two distinct mocking strategies to allow Linux logic to compile and "run" on macOS.

### 1. Epoll Mock: "Syscall Shim Based Mock"
For `epoll`, we use a **Syscall Shim**. We compile the *real* `src/unix/io-backend/io-backend-epoll.c` (the Linux implementation) on macOS.
*   **How:** We intercept `epoll_create`, `epoll_ctl`, and `epoll_wait` calls.
*   **Result:** The code *thinks* it's running on Linux. It manages the red-black trees and state logic exactly as it would on production. The "kernel" it talks to is actually a user-space simulation in `src/unix/mocks/linux-stub.c`.
*   **Benefit:** High test coverage of the actual backend logic.

### 2. Iouring Mock: "io-backend Stub Mock"
For `io_uring`, mocking the complex shared-memory ring buffer mechanism in user space was too heavy.
*   **How:** We replace the entire backend with a stub (`src/unix/mocks/io-backend-iouring-stub.c`).
*   **Result:** It compiles and satisfies the linker, but it doesn't perform real I/O.
*   **Benefit:** Allows us to verify the "Mix-and-Match" compilation logic and V-Table routing without implementing a full Linux kernel simulator.

## Performance Analysis

We ran benchmarks on macOS (M1/M2) comparing native `kqueue` performance against the coroutine abstraction.

| Implementation | Backend | RPS (Req/s) | Notes |
| :--- | :--- | :--- | :--- |
| **Raw Libuv** | kqueue | **48,661** | Baseline (Callback based) |
| **UVC (fcontext)** | kqueue | **24,872** | Fast coroutine switching |
| **UVC (ucontext)** | kqueue | **24,441** | Standard (slightly slower) |
| **UVC (Mock)** | epoll-mock | **9,687** | Functional mock (Slow) |

**Key Takeaways:**
1.  **The Cost of Abstraction:** Switching from callbacks to coroutines incurs a ~50% throughput penalty in this micro-benchmark. This is the price of the context switching and scheduling overhead. For CPU-bound or mixed workloads, this gap often narrows, but for raw echo-server throughput, callbacks still rule.
2.  **Context Switching:** `fcontext` (assembly based) is slightly faster than `ucontext` (system call based), but the difference is marginal compared to the overall loop overhead.
3.  **Mock Performance:** The mocks are significantly slower, confirming they are fit for logic verification but not performance profiling.

## Conclusion

The `libuvc` experiment confirms that "Vibe Coding" is viable for system-level programming. We successfully refactored a massive C codebase, implemented complex architectural patterns (SPIs, Coroutines, Mocks), and verified them with benchmarks—all in a short sprint.

While `libuvc` isn't intended for production (please stick to official `libuv`!), it serves as an excellent educational resource for understanding the internals of event loops, `io_uring`, and how to build modern async runtimes in C.

Reflecting on this journey, the capability of Vibe Coding exceeded my initial expectations. It suggests a future where detailed technical "how-to" articles like this one might become obsolete. Instead, engineers should shift their focus towards **architectural aesthetics** and **boundary judgment**, leveraging AI to handle the implementation details.

{% include common/series.html %}
