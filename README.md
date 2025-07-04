# nngx

## Overview / 概述

**nngx** is a high-performance C++ wrapper library for [NNG (Nanomsg Next Generation)](https://github.com/nanomsg/nng), providing a modern, type-safe, and easy-to-use interface for building distributed messaging applications. The library extends NNG's capabilities with advanced features like asynchronous message handling, thread-safe operations, and comprehensive protocol support. 该库为构建分布式消息传递应用程序提供现代化、类型安全且易于使用的接口。

## Features & Usage / 特性与用法

### Key Features

- **Minimal API, Modern C++**: Just inherit protocol classes and override callbacks for high-performance distributed messaging. Unified interface for all protocols.  
  **极简 API，现代 C++ 风格**：只需继承协议类并重写回调，即可实现高性能分布式通信，所有协议统一接口。
- **Async Send & AIO Reuse**: Automatic AIO resource reuse, high-concurrency async send (`async_send`), thread-safe, no manual async management.  
  **异步发送与 AIO 复用**：内部自动复用 AIO 资源，支持高并发异步发送，无需手动管理异步对象，线程安全。
- **Flexible Message Handling**: Support for raw and code-based message dispatch, customizable routing and logic.  
  **消息处理灵活**：支持原始消息和带 code 的消息分发，可自定义消息路由与处理逻辑。
- **Service Abstraction**: `Service<T>` template for high-level orchestration and parallelism.  
  **服务化抽象**：提供 Service<T> 模板，轻松实现高层服务编排与并发处理。
- **Thread & Exception Safety**: Recursive mutexes and atomics, all interfaces thread-safe, exceptions auto-caught.  
  **线程安全与异常安全**：全面使用递归互斥锁和原子操作，所有接口均为线程安全，异常自动捕获。
- **RAII Resource Management**: All resources auto-managed, no manual cleanup.  
  **自动资源管理**：全面 RAII 设计，Socket/Msg/Aio 等资源自动释放，无需手动清理。

---

### Typical Usage / 典型用法

#### 1. Request/Response
```cpp
// Server / 服务端
class MyResponse : public nng::Response {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code code, nng::Msg& msg) override {
        std::string data = msg.chop_string();
        nng::Msg reply; reply.append_string("Hello, " + data);
        this->async_send(code, std::move(reply)); // async reply / 异步回复
        return {};
    }
};
MyResponse resp; resp.start("tcp://*:5555"); resp.dispatch();

// Client / 客户端
nng::Request req; req.start("tcp://localhost:5555");
nng::Msg msg; msg.append_string("World");
auto fut = req.async_send(0, std::move(msg)); // async send & wait / 异步发送并等待回复
auto reply = fut.get();
```

#### 2. Push/Pull
```cpp
// Pull side / Pull 端
class MyPull : public nng::Pull<> {
    bool _On_raw_message(nng::Msg& msg) override {
        std::cout << msg.chop_string() << std::endl;
        return true;
    }
};
MyPull pull; pull.start("tcp://*:6000"); std::thread([&]{ pull.dispatch(); }).detach();

// Push side / Push 端
nng::Push<> push; push.start("tcp://localhost:6000");
push.async_send(nng_iov{(void*)"Data", 4});
```

#### 3. Pub/Sub
```cpp
// Subscriber / 订阅者
class MySub : public nng::Subscriber<> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        std::cout << msg.chop_string() << std::endl;
        return {};
    }
};
MySub sub; sub.start("tcp://localhost:7000"); sub.subscribe(""); sub.dispatch();

// Publisher / 发布者
nng::Publisher pub; pub.start("tcp://*:7000");
nng::Msg msg; msg.append_string("Broadcast");
pub.async_send(0, std::move(msg));
```

#### 4. Survey/Respond
```cpp
// Respond side / Respond 端
class MyRespond : public nng::Respond<> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        nng::Msg reply; reply.append_string("Reply");
        this->async_send(0, std::move(reply));
        return {};
    }
};
MyRespond resp; resp.start("tcp://*:8000"); resp.dispatch();

// Survey side / Survey 端
nng::Survey survey; survey.start("tcp://localhost:8000");
nng::Msg msg; msg.append_string("Survey?");
survey.async_send(0, std::move(msg));
```

#### 5. Service & Parallel / 服务化与并发
```cpp
// Parallel service / 并发服务端
class MyService : public nng::Service<nng::Response> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        nng::Msg reply; reply.append_string("Service Reply");
        this->async_send(0, std::move(reply));
        return {};
    }
};
MyService svc; svc.start("tcp://*:9000", 8); svc.dispatch();
```

#### 6. Pair (Bidirectional Communication / 点对点双向通信)
```cpp
// Listener side / 监听端
class MyPairListener : public nng::Pair<nng::Listener> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code code, nng::Msg& msg) override {
        std::cout << "Listener got: " << msg.chop_string() << std::endl;
        return {};
    }
};
MyPairListener listener; listener.start("tcp://*:6001"); listener.dispatch();

// Dialer side / 拨号端
class MyPairDialer : public nng::Pair<nng::Dialer> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        std::cout << "Dialer got: " << msg.chop_string() << std::endl;
        return {};
    }
};
MyPairDialer dialer; dialer.start("tcp://localhost:6001"); dialer.dispatch();
```

#### 7. Bus (Many-to-Many / 多对多总线)
```cpp
class MyBus : public nng::Bus {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        std::cout << "Bus received: " << msg.chop_string() << std::endl;
        return {};
    }
};
MyBus bus1, bus2, bus3;
bus1.start("tcp://*:7001");
bus2.start("tcp://localhost:7001");
bus3.start("tcp://localhost:7001");
bus1.dial("tcp://localhost:7001");
bus2.dial("tcp://*:7001");
bus3.dial("tcp://*:7001");
bus1.send(0, nng::Msg::to_msg("BusMsg"));
```

#### 8. ResponseParallel (并发应答)
```cpp
class MyResponseParallel : public nng::ResponseParallel {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        msg.realloc(0); msg.append_string("ParallelReply");
        return 123;
    }
};
MyResponseParallel resp; resp.start("tcp://*:8001", 8); // 8 并发
// Client can use Request::simple_send for sync/async requests
```

#### 9. Service<T> (服务化并发)
```cpp
class MyService : public nng::Service<nng::Response> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code, nng::Msg& msg) override {
        nng::Msg reply; reply.append_string("ServiceReply");
        this->async_send(0, std::move(reply));
        return {};
    }
};
MyService svc; svc.start("tcp://*:9001", 4); svc.dispatch(); // 4 并发
```

#### 10. Raw Message (原始消息处理)
```cpp
class MyRaw : public nng::Response {
    bool _On_raw_message(nng::Msg& msg) override {
        auto code = msg.chop_u32();
        std::cout << "Raw code: " << code << std::endl;
        return true;
    }
};
MyRaw raw; raw.start("tcp://*:9002"); raw.dispatch();
```

#### 11. Huge Message / High Concurrency (大消息/高并发压力测试)
```cpp
class MyPull : public nng::Service<nng::Pull<nng::Dialer>> {
    nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code code, nng::Msg& msg) override {
        // Statistic and check big messages / 统计和校验大消息
        return {};
    }
};
MyPull pull; pull.start_dispatch("tcp://*:9003");
nng::Push<nng::Listener> push; push.start("tcp://localhost:9003");
for (int i = 0; i < 10000; ++i) {
    nng::Msg m(0); m.insert_string("BigData");
    push.async_send(1, std::move(m));
}
```

#### 12. Msg Utility (消息工具类)
```cpp
std::string s = "TestString";
nng::Msg m = nng::Msg::to_msg(s);
assert(nng::Msg::to_string(m) == s);
```

---

### Design Highlights / 设计亮点

- **AIO Reuse & High Concurrency**: All async_send calls reuse underlying AIOs, maximizing concurrency, no async details exposed to user.  
  **AIO 复用与高并发**：所有 async_send 自动复用底层 AIO，极大提升并发性能，用户无需关心异步细节。
- **Simple Inheritance & Callback**: Just inherit and override, unified usage for all protocols, minimal learning curve.  
  **极简继承与回调**：只需继承协议类并重写回调，所有协议统一用法，极大降低学习和维护成本。
- **Thread Safety**: All async and queue operations are lock-protected, suitable for multi-threaded scenarios.  
  **线程安全**：所有异步操作和消息队列均加锁保护，适合多线程高并发场景。
- **Service & Extensibility**: Service<T> enables high-level orchestration, easy to extend and integrate.  
  **服务化与扩展性**：Service<T> 支持高层业务编排，易于扩展和集成。

---

For more detailed examples, see `nngx-caller/main.cpp`.  
如需更详细示例，请参考 `nngx-caller/main.cpp`。

## Key Features / 主要特性

- **Modern C++ Design / 现代 C++ 设计**: Built with C++17/20 features including RAII, smart pointers, and modern STL containers
- **Asynchronous Messaging / 异步消息处理**: Full support for async I/O operations with callback-based and promise-based APIs
- **Thread Safety / 线程安全**: Comprehensive thread-safe operations using recursive mutexes and atomic operations
- **Protocol Support / 协议支持**: Complete implementation of all NNG protocols:
  - **Request/Response**: Synchronous request-reply pattern / 同步请求-回复模式
  - **Pair**: Bidirectional communication between two peers / 两个对等点之间的双向通信
  - **Push/Pull**: Pipeline pattern for load distribution / 用于负载分配的管道模式
  - **Publisher/Subscriber**: One-to-many broadcast messaging / 一对多广播消息传递
  - **Survey/Respond**: One-to-many request-reply pattern / 一对多请求-回复模式
  - **Bus**: Many-to-many peer communication / 多对多对等通信
- **Message Handling / 消息处理**: Advanced message processing with code-based routing and raw message support
- **Service Framework / 服务框架**: High-level service abstraction for building scalable applications
- **Exception Safety / 异常安全**: Comprehensive error handling with custom exception types
- **Memory Management / 内存管理**: Automatic resource management with RAII principles

## Architecture / 架构

The library is organized into several core components / 该库组织为几个核心组件:

- **Core Classes / 核心类**: `Socket`, `Msg`, `Aio`, `Ctx` - Fundamental building blocks
- **Protocol Implementations / 协议实现**: `Request`, `Response`, `Pair`, `Push`, `Pull`, `Publisher`, `Subscriber`, `Survey`, `Respond`, `Bus`
- **Async Framework / 异步框架**: `AsyncContext`, `AsyncSender`, `AsyncSenderNoReturn`, `AsyncSenderWithReturn`
- **Service Layer / 服务层**: `Service<T>` template for high-level application development
- **Utilities / 工具**: Helper functions and utilities for common operations

## Quick Start / 快速开始

```cpp
#include "nngx.h"

// Simple Request/Response example / 简单的请求/响应示例
class MyResponse : public nng::Response {
private:
    virtual nng::Msg::_Ty_msg_result _On_message(nng::Msg::_Ty_msg_code code, nng::Msg& msg) override {
        std::string data = msg.chop_string();
        std::cout << "Received: " << data << std::endl; // 收到
        
        // Send reply / 发送回复
        nng::Msg reply;
        reply.append_string("Hello from server!"); // 来自服务器的问候！
        this->async_send(code, std::move(reply));
        return {};
    }
};

int main() {
    MyResponse response;
    response.start("tcp://localhost:5555");
    response.dispatch(); // Start message loop / 启动消息循环
    return 0;
}
```

## nngUtil Utility / nngUtil 工具说明

- **Cross-platform IPC Path Fix / 跨平台 IPC 路径修正**  
  On Linux/macOS, `_Pre_address` automatically prepends `/tmp/` to `ipc://` addresses if not already present, ensuring IPC sockets are always created in a writable and accessible directory.  
  **在 Linux/macOS 下，自动为 ipc:// 路径加上 `/tmp/` 前缀，保证 socket 路径可写、可访问。**
  On Windows, the function leaves the path unchanged, as Windows uses named pipes.

- **IPC Permission Handling / IPC 权限处理**  
  On Windows, `_Pre_start_listen` sets a security descriptor for the IPC listener if the process is running as SYSTEM or Administrator, so that user-level processes can also access the IPC endpoint.  
  **在 Windows 下，若进程为 SYSTEM 或管理员权限，自动设置安全描述符，允许普通用户进程访问 IPC。**
  On Linux, always sets the IPC file permissions to `0777` (world-readable/writable), so that both root and non-root processes can communicate via IPC sockets.  
  **在 Linux 下，监听端始终设置 IPC 文件权限为 0777，确保 root 和普通用户进程都能访问。**

- **Initialization/Uninitialization / 初始化与反初始化**  
  `nng::util::initialize()` and `uninitialize()` are used to set up and tear down these hooks and NNG global state.  
  **用于设置上述路径和权限钩子，并初始化/反初始化 NNG 全局状态。**

### Practical Usage in main.cpp / main.cpp 实践用法

Always call `nng::util::initialize();` at the start of your program, and `nng::util::uninitialize();` at the end.  
This ensures all IPC addresses and permissions are handled correctly on all platforms, avoiding common issues with socket file paths and access rights.  
**在 main.cpp 中，程序入口处调用 `nng::util::initialize()`，退出前调用 `uninitialize()`，可自动解决 Linux/Windows 下的 IPC 路径和权限问题，避免因权限或路径导致的通信失败。**

**Summary:**  
nngUtil solves two major cross-platform problems for IPC:  
1. Ensures IPC socket paths are always valid and accessible (especially on Linux/macOS).  
2. Ensures IPC permissions allow communication between processes with different privileges (root/user, SYSTEM/user, etc).

## Building / 构建

The project uses CMake for build configuration / 项目使用 CMake 进行构建配置:

```bash
mkdir build && cd build
cmake ..
make
```

## Dependencies / 依赖项

- **NNG**: Nanomsg Next Generation library (v1.11+) / 库 (v1.11+)
- **C++17/20**: Modern C++ compiler support / 现代 C++ 编译器支持
- **CMake**: Build system (v3.8+) / 构建系统 (v3.8+)

## Testing / 测试

The project includes comprehensive test suites demonstrating all protocol patterns and features. Run the test executable to verify functionality / 项目包含全面的测试套件，演示所有协议模式和功能。运行测试可执行文件以验证功能:
