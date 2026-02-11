# udp_handler - UDP Packet Handler Base Class

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Construction](#construction)
- [Core Methods](#core-methods)
- [Statistics and Monitoring](#statistics-and-monitoring)
- [Virtual Hooks](#virtual-hooks)
- [Packet Processing Pipeline](#packet-processing-pipeline)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)
- [Thread Safety](#thread-safety)
- [Error Handling](#error-handling)
- [Performance Considerations](#performance-considerations)
- [See Also](#see-also)

---

## Overview

The `udp_handler` class is the base class for processing individual UDP datagrams in a multi-threaded server environment. It provides a complete packet processing pipeline with validation, hooks, statistics tracking, and exception handling.

**Key Features:**
- **Pure Virtual Interface** - Implement `handle_packet()` to define packet processing logic
- **Packet Validation** - Built-in validation pipeline with customizable hooks
- **Statistics Tracking** - Automatic tracking of packets, bytes, and errors
- **Exception Safety** - Robust exception handling with `noexcept` guarantees
- **Address Filtering** - Virtual `can_handle_address()` for IP-based filtering
- **Size Limits** - Virtual `max_packet_size()` for packet size validation
- **Lifecycle Hooks** - Pre/post processing hooks for custom logic
- **Move-Only Semantics** - Efficient resource management

**Usage Modes:**
1. **Factory Pattern** - Create new handler instance for each packet (isolated state)
2. **Shared Pattern** - Reuse single thread-safe handler (shared state)

**Include:** `<fb/udp_handler.h>`
**Namespace:** `fb`
**Platform Support:** Windows, macOS, Linux, BSD

---

## Quick Start

### Basic Handler Implementation

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

class EchoHandler : public udp_handler {
    udp_socket& m_socket;  // For sending responses
public:
    EchoHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // Echo the packet back to sender
        m_socket.send_to(buffer, length, sender_address);

        std::cout << "Echoed " << length << " bytes to "
                  << sender_address.to_string() << "\n";
    }
};

// Usage with udp_server:
udp_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8888));

udp_socket& sock_ref = sock;
auto factory = [&sock_ref](const udp_server::PacketData& packet) {
    return std::make_unique<EchoHandler>(sock_ref);
};

udp_server server(std::move(sock), factory);
server.start();
```

---

## Construction

### Default Constructor

```cpp
udp_handler();
```

Constructs a handler and initializes statistics tracking.

**Initializes:**
- `packets_processed` = 0
- `bytes_processed` = 0
- `error_count` = 0
- `creation_time` = current time
- `last_packet_time` = current time

---

### Move Semantics

```cpp
udp_handler(udp_handler&& other) noexcept = default;
udp_handler& operator=(udp_handler&& other) noexcept = default;
```

Handlers are **move-only** (no copy construction or assignment).

**Example:**
```cpp
class MyHandler : public udp_handler {
    std::unique_ptr<SomeResource> m_resource;
public:
    MyHandler(MyHandler&&) noexcept = default;  // Inherits move
};

MyHandler handler1;
MyHandler handler2 = std::move(handler1);  // OK
// MyHandler handler3 = handler1;  // ERROR: deleted copy constructor
```

---

### Virtual Destructor

```cpp
virtual ~udp_handler() = default;
```

Virtual destructor for proper polymorphic deletion.

---

## Core Methods

### handle_packet() (Pure Virtual)

```cpp
virtual void handle_packet(const void* buffer, std::size_t length,
                          const socket_address& sender_address) = 0;
```

**Pure virtual method** that must be implemented by derived classes to define packet processing logic.

**Parameters:**
- `buffer` - Pointer to packet data
- `length` - Packet size in bytes
- `sender_address` - Source address of the packet

**Called by:** `process_packet()` after validation and pre-processing hooks

**Example:**
```cpp
class MyHandler : public udp_handler {
public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string data(static_cast<const char*>(buffer), length);
        std::cout << "Received: " << data << " from "
                  << sender_address.to_string() << "\n";

        // Process the packet...
    }
};
```

---

### process_packet()

```cpp
bool process_packet(const void* buffer, std::size_t length,
                   const socket_address& sender_address) noexcept;
```

**Main entry point** for packet processing. This is the wrapper method that implements the complete processing pipeline.

**Parameters:**
- `buffer` - Pointer to packet data
- `length` - Packet size in bytes
- `sender_address` - Source address

**Returns:** `true` if packet was processed successfully, `false` otherwise

**Pipeline Steps:**
1. Validate input parameters (null buffer, zero length)
2. Check address filtering via `can_handle_address()`
3. Check size limits via `max_packet_size()`
4. Validate packet data via `validate_packet()`
5. Call pre-processing hook `on_packet_received()`
6. Call `handle_packet()` (your implementation)
7. Call post-processing hook `on_packet_processed()`
8. Update statistics
9. Handle exceptions via `handle_exception()`

**Exception Safety:** `noexcept` - all exceptions are caught internally

**Example:**
```cpp
MyHandler handler;

// Called by udp_server for each packet
bool success = handler.process_packet(data, len, sender_addr);
if (!success) {
    std::cout << "Packet processing failed\n";
}
```

---

### handler_name()

```cpp
virtual std::string handler_name() const;
```

Returns a human-readable handler name for logging and debugging.

**Returns:** RTTI-based class name by default

**Override:** Provide custom name for better logging

**Example:**
```cpp
class MyHandler : public udp_handler {
public:
    std::string handler_name() const override {
        return "MyCustomHandler v1.0";
    }
};

MyHandler handler;
std::cout << "Using: " << handler.handler_name() << "\n";
// Output: Using: MyCustomHandler v1.0
```

---

### max_packet_size()

```cpp
virtual std::size_t max_packet_size() const;
```

Reports the maximum packet size accepted by this handler.

**Returns:** Maximum size in bytes, or **0 for unlimited** (default)

**Override:** Enforce size limits to prevent processing oversized packets

**Example:**
```cpp
class LimitedHandler : public udp_handler {
public:
    std::size_t max_packet_size() const override {
        return 1024;  // Reject packets > 1KB
    }

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // length is guaranteed to be <= 1024
    }
};
```

---

### can_handle_address()

```cpp
virtual bool can_handle_address(const socket_address& sender_address) const;
```

Determines if the handler will accept packets from the specified sender.

**Parameters:**
- `sender_address` - Source endpoint to evaluate

**Returns:** `true` by default (accept all); override to apply filtering

**Use Cases:**
- IP whitelisting/blacklisting
- Subnet filtering
- Port-based routing

**Example:**
```cpp
class WhitelistHandler : public udp_handler {
    std::unordered_set<std::string> m_allowed_ips;
public:
    WhitelistHandler(std::unordered_set<std::string> allowed)
        : m_allowed_ips(std::move(allowed)) {}

    bool can_handle_address(const socket_address& sender_address) const override {
        return m_allowed_ips.count(sender_address.host()) > 0;
    }

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Only called for whitelisted IPs
    }
};
```

---

## Statistics and Monitoring

### packets_processed()

```cpp
std::uint64_t packets_processed() const;
```

Returns the total number of successfully processed packets.

**Example:**
```cpp
std::cout << "Processed: " << handler.packets_processed() << " packets\n";
```

---

### bytes_processed()

```cpp
std::uint64_t bytes_processed() const;
```

Returns the total number of bytes processed by the handler.

**Example:**
```cpp
std::cout << "Processed: " << handler.bytes_processed() << " bytes\n";
```

---

### error_count()

```cpp
std::uint64_t error_count() const;
```

Returns the total number of packets that resulted in errors.

**Incremented when:**
- Input validation fails
- Address filtering rejects packet
- Size limit exceeded
- Custom validation fails
- Exception thrown in `handle_packet()`

**Example:**
```cpp
std::cout << "Errors: " << handler.error_count() << "\n";

double error_rate = static_cast<double>(handler.error_count()) /
                   (handler.packets_processed() + handler.error_count());
std::cout << "Error rate: " << (error_rate * 100) << "%\n";
```

---

### creation_time()

```cpp
std::chrono::steady_clock::time_point creation_time() const;
```

Returns the timestamp when the handler was created.

**Example:**
```cpp
auto now = std::chrono::steady_clock::now();
auto age = std::chrono::duration_cast<std::chrono::seconds>(
    now - handler.creation_time()).count();
std::cout << "Handler age: " << age << " seconds\n";
```

---

### last_packet_time()

```cpp
std::chrono::steady_clock::time_point last_packet_time() const;
```

Returns the timestamp of the most recent packet handled.

**Example:**
```cpp
auto now = std::chrono::steady_clock::now();
auto idle = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - handler.last_packet_time()).count();
std::cout << "Idle for: " << idle << " ms\n";
```

---

### reset_statistics()

```cpp
void reset_statistics();
```

Resets all tracked statistics back to zero and updates timestamps.

**Resets:**
- `packets_processed` → 0
- `bytes_processed` → 0
- `error_count` → 0
- `creation_time` → current time
- `last_packet_time` → current time

**Example:**
```cpp
handler.reset_statistics();
std::cout << "Statistics reset\n";
```

---

## Virtual Hooks

### handle_exception()

```cpp
virtual void handle_exception(const std::exception& ex,
                              const socket_address& sender_address) noexcept;
```

Exception handler invoked when packet processing fails.

**Parameters:**
- `ex` - Exception instance
- `sender_address` - Source endpoint that triggered the exception

**Default:** Does nothing (ignores exception)

**Override:** Log errors, send alerts, or implement retry logic

**Example:**
```cpp
class LoggingHandler : public udp_handler {
    std::ofstream m_log;
public:
    void handle_exception(const std::exception& ex,
                         const socket_address& sender_address) noexcept override
    {
        m_log << "[ERROR] From " << sender_address.to_string()
              << ": " << ex.what() << "\n";
        m_log.flush();
    }
};
```

---

### on_packet_received()

```cpp
virtual void on_packet_received(const void* buffer, std::size_t length,
                                const socket_address& sender_address) noexcept;
```

Hook executed **before** `handle_packet()` runs.

**Parameters:** Mirror those passed to `handle_packet()`

**Default:** Does nothing

**Use Cases:**
- Logging packet arrivals
- Updating arrival timestamps
- Pre-processing or decryption

**Example:**
```cpp
class TimestampHandler : public udp_handler {
    std::chrono::steady_clock::time_point m_arrival_time;

protected:
    void on_packet_received(const void* buffer, std::size_t length,
                           const socket_address& sender_address) noexcept override
    {
        m_arrival_time = std::chrono::steady_clock::now();
    }

    void on_packet_processed(const void* buffer, std::size_t length,
                            const socket_address& sender_address) noexcept override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_arrival_time;
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        std::cout << "Processing time: " << ms << " μs\n";
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Process packet...
    }
};
```

---

### on_packet_processed()

```cpp
virtual void on_packet_processed(const void* buffer, std::size_t length,
                                 const socket_address& sender_address) noexcept;
```

Hook executed **after** packet has been processed successfully.

**Parameters:** Mirror those passed to `handle_packet()`

**Default:** Does nothing

**Use Cases:**
- Post-processing or encryption
- Sending confirmations
- Updating completion timestamps

---

### validate_packet()

```cpp
virtual bool validate_packet(const void* buffer, std::size_t length,
                             const socket_address& sender_address) const;
```

Validates a packet before handing it to `handle_packet()`.

**Parameters:**
- `buffer` - Packet payload
- `length` - Payload length
- `sender_address` - Source endpoint

**Returns:** `true` to process packet, `false` to drop

**Default:** Accepts every packet (returns `true`)

**Use Cases:**
- Protocol validation (magic numbers, checksums)
- Content filtering
- Rate limiting

**Example:**
```cpp
class ProtocolHandler : public udp_handler {
protected:
    bool validate_packet(const void* buffer, std::size_t length,
                        const socket_address& sender_address) const override
    {
        if (length < 4) {
            return false;  // Too small
        }

        // Check magic number (first 4 bytes)
        const uint32_t* magic = static_cast<const uint32_t*>(buffer);
        if (*magic != 0xDEADBEEF) {
            return false;  // Invalid protocol
        }

        return true;  // Valid packet
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Packet is guaranteed to be valid
    }
};
```

---

## Packet Processing Pipeline

The `process_packet()` method implements this complete pipeline:

```
1. Input Validation
   ↓ (buffer != null && length > 0)
2. Address Filtering
   ↓ (can_handle_address())
3. Size Check
   ↓ (length <= max_packet_size())
4. Custom Validation
   ↓ (validate_packet())
5. Pre-Processing Hook
   ↓ (on_packet_received())
6. Packet Processing
   ↓ (handle_packet()) ← YOUR CODE
7. Post-Processing Hook
   ↓ (on_packet_processed())
8. Statistics Update
   ↓ (packets_processed++, bytes_processed += length)
9. Return Success
```

**If any step fails:**
```
Exception or Validation Failure
   ↓
Exception Handler
   ↓ (handle_exception())
Statistics Update
   ↓ (error_count++)
Return Failure (false)
```

---

## Complete Examples

### Example 1: Simple Echo Handler

```cpp
class EchoHandler : public udp_handler {
    udp_socket& m_socket;
public:
    EchoHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        m_socket.send_to(buffer, length, sender_address);
    }
};
```

---

### Example 2: Protocol Handler with Validation

```cpp
class ProtocolHandler : public udp_handler {
    udp_socket& m_socket;

protected:
    bool validate_packet(const void* buffer, std::size_t length,
                        const socket_address& sender_address) const override
    {
        if (length < sizeof(uint32_t)) return false;

        const uint32_t* header = static_cast<const uint32_t*>(buffer);
        return (*header == 0x50524F54);  // "PROT" magic number
    }

public:
    ProtocolHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // Skip magic number
        const char* data = static_cast<const char*>(buffer) + sizeof(uint32_t);
        std::size_t data_len = length - sizeof(uint32_t);

        std::string message(data, data_len);
        std::cout << "Protocol message: " << message << "\n";

        // Send ACK
        std::string ack = "ACK";
        m_socket.send_to(ack.c_str(), ack.length(), sender_address);
    }
};
```

---

### Example 3: Rate-Limited Handler

```cpp
class RateLimitedHandler : public udp_handler {
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> m_requests;
    mutable std::mutex m_mutex;
    size_t m_max_per_second = 10;

protected:
    bool validate_packet(const void* buffer, std::size_t length,
                        const socket_address& sender_address) const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string client_ip = sender_address.host();

        auto now = std::chrono::steady_clock::now();
        auto& times = m_requests[client_ip];

        // Remove old entries
        while (!times.empty() && now - times.front() > std::chrono::seconds(1)) {
            times.pop_front();
        }

        if (times.size() >= m_max_per_second) {
            return false;  // Rate limit exceeded
        }

        times.push_back(now);
        return true;
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // Process packet (rate limit already enforced)
    }
};
```

---

### Example 4: Logging Handler with Hooks

```cpp
class LoggingHandler : public udp_handler {
    std::ofstream m_log;
    std::chrono::steady_clock::time_point m_start_time;

protected:
    void on_packet_received(const void* buffer, std::size_t length,
                           const socket_address& sender_address) noexcept override
    {
        m_start_time = std::chrono::steady_clock::now();
        m_log << "[RX] " << sender_address.to_string() << " - " << length << " bytes\n";
    }

    void on_packet_processed(const void* buffer, std::size_t length,
                            const socket_address& sender_address) noexcept override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_start_time;
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        m_log << "[TX] Processing time: " << us << " μs\n";
        m_log.flush();
    }

    void handle_exception(const std::exception& ex,
                         const socket_address& sender_address) noexcept override
    {
        m_log << "[ERROR] " << sender_address.to_string() << ": " << ex.what() << "\n";
        m_log.flush();
    }

public:
    LoggingHandler(const std::string& log_file) {
        m_log.open(log_file, std::ios::app);
    }

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string data(static_cast<const char*>(buffer), length);
        m_log << "[PACKET] " << data << "\n";
    }
};
```

---

### Example 5: Stateful Handler with Context

```cpp
class SessionHandler : public udp_handler {
    struct Session {
        uint64_t packet_count = 0;
        std::chrono::steady_clock::time_point last_seen;
    };

    std::unordered_map<std::string, Session> m_sessions;
    std::mutex m_mutex;
    udp_socket& m_socket;

public:
    SessionHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string key = sender_address.to_string();
        auto& session = m_sessions[key];
        session.packet_count++;
        session.last_seen = std::chrono::steady_clock::now();

        std::ostringstream response;
        response << "Session packet #" << session.packet_count;
        std::string resp_str = response.str();

        m_socket.send_to(resp_str.c_str(), resp_str.length(), sender_address);
    }

    size_t active_sessions() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_sessions.size();
    }
};
```

---

## Common Patterns

### Pattern 1: IP Whitelisting

```cpp
class WhitelistHandler : public udp_handler {
    std::unordered_set<std::string> m_whitelist;

protected:
    bool can_handle_address(const socket_address& sender_address) const override {
        return m_whitelist.count(sender_address.host()) > 0;
    }

public:
    WhitelistHandler(std::unordered_set<std::string> whitelist)
        : m_whitelist(std::move(whitelist)) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Only whitelisted IPs reach here
    }
};
```

---

### Pattern 2: Checksum Validation

```cpp
class ChecksumHandler : public udp_handler {
protected:
    bool validate_packet(const void* buffer, std::size_t length,
                        const socket_address& sender_address) const override
    {
        if (length < sizeof(uint32_t)) return false;

        const uint8_t* data = static_cast<const uint8_t*>(buffer);
        uint32_t received_checksum = *reinterpret_cast<const uint32_t*>(data);
        uint32_t computed_checksum = compute_checksum(data + sizeof(uint32_t),
                                                      length - sizeof(uint32_t));

        return received_checksum == computed_checksum;
    }

    uint32_t compute_checksum(const uint8_t* data, size_t len) const {
        // Simple checksum implementation
        uint32_t sum = 0;
        for (size_t i = 0; i < len; ++i) {
            sum += data[i];
        }
        return sum;
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Checksum is valid
    }
};
```

---

### Pattern 3: Performance Monitoring

```cpp
class MonitoredHandler : public udp_handler {
    std::chrono::steady_clock::time_point m_start;
    std::vector<int64_t> m_processing_times;
    mutable std::mutex m_mutex;

protected:
    void on_packet_received(const void* buffer, std::size_t length,
                           const socket_address& sender_address) noexcept override {
        m_start = std::chrono::steady_clock::now();
    }

    void on_packet_processed(const void* buffer, std::size_t length,
                            const socket_address& sender_address) noexcept override {
        auto elapsed = std::chrono::steady_clock::now() - m_start;
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_processing_times.push_back(us);
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override {
        // Process packet...
    }

    double average_processing_time() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_processing_times.empty()) return 0.0;

        int64_t sum = std::accumulate(m_processing_times.begin(),
                                      m_processing_times.end(), 0LL);
        return sum / static_cast<double>(m_processing_times.size());
    }
};
```

---

## Thread Safety

### Factory Pattern (Isolated Handlers)
Each packet gets its own handler instance - **no synchronization needed**.

```cpp
auto factory = [](const udp_server::PacketData& packet) {
    return std::make_unique<MyHandler>();  // New instance per packet
};
```

**Thread Safety:** ✅ Automatic (isolated state)

---

### Shared Pattern (Single Handler)
Single handler processes all packets - **must be thread-safe**.

```cpp
class ThreadSafeHandler : public udp_handler {
    std::atomic<uint64_t> m_counter{0};
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, int> m_data;

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        m_counter++;  // Atomic - OK

        std::lock_guard<std::mutex> lock(m_mutex);  // Protect shared data
        m_data[sender_address.host()]++;
    }
};

auto handler = std::make_shared<ThreadSafeHandler>();
udp_server server(std::move(sock), handler);  // Shared across threads
```

**Thread Safety:** ⚠️ Must use synchronization primitives

---

## Error Handling

### Exception Safety Levels

**`process_packet()` - `noexcept`**
- Catches all exceptions
- Never throws
- Calls `handle_exception()` for errors
- Returns `false` on failure

**`handle_packet()` - Can throw**
- Your implementation can throw exceptions
- Exceptions are caught by `process_packet()`
- Error count is incremented
- `handle_exception()` is called

**All hooks - `noexcept`**
- `on_packet_received()` - must not throw
- `on_packet_processed()` - must not throw
- `handle_exception()` - must not throw
- `validate_packet()` - can throw (caught by process_packet)

---

### Custom Error Handling

```cpp
class ErrorLoggingHandler : public udp_handler {
    std::ofstream m_error_log;

protected:
    void handle_exception(const std::exception& ex,
                         const socket_address& sender_address) noexcept override
    {
        m_error_log << "[" << timestamp() << "] "
                   << sender_address.to_string() << ": "
                   << ex.what() << "\n";
        m_error_log.flush();
    }

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        if (length < 10) {
            throw std::runtime_error("Packet too small");
        }
        // Process...
    }
};
```

---

## Performance Considerations

### Handler Creation Overhead

**Factory Pattern:**
- Creates new handler for each packet
- Higher memory allocation overhead
- Better for stateful handlers
- No synchronization needed

**Shared Pattern:**
- Reuses single handler
- Lower memory overhead
- Requires thread synchronization
- Better for stateless processing

---

### Statistics Overhead

Statistics are tracked for every packet:
- Packet count (atomic increment)
- Byte count (atomic add)
- Timestamp update

For maximum performance, override `update_statistics()` (private) or disable tracking in derived class.

---

### Validation Overhead

The pipeline executes multiple checks:
1. Null/empty check
2. Address filter
3. Size limit
4. Custom validation
5. Pre/post hooks

**Optimize by:**
- Return early in `validate_packet()`
- Avoid expensive operations in hooks
- Use simple address checks

---

## See Also

- [udp_server](udp_server.md) - Multi-threaded UDP server
- [udp_socket](udp_socket.md) - UDP socket for datagram communication
- [socket_address](socket_address.md) - Network address representation
- [Exception Handling](exceptions.md) - Network exception hierarchy

---

**[Back to Index](index.md)** | **[Previous: udp_server](udp_server.md)** | **[Next: udp_client](udp_client.md)**
