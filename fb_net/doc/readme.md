# FB_NET Documentation

## Overview

This directory contains documentation for the `fb_net` networking library including API references, practical examples, and best practices.

## Documentation Files

### Complete Documentation

1. **[index.md](index.md)** - Main documentation hub
   - Library overview and features
   - Quick Start guide
   - Build instructions
   - Component overview for all 4 phases
   - Platform support
   - Performance characteristics
   - API reference table

2. **[socket_address.md](socket_address.md)** - Network Address Abstraction
   - Complete API reference
   - All constructors with examples
   - Address family support (IPv4/IPv6)
   - DNS resolution
   - String parsing ("host:port")
   - Comparison operators
   - 5 complete practical examples
   - Common patterns
   - Error handling
   - Performance considerations

3. **[exceptions.md](exceptions.md)** - Exception Hierarchy
   - Complete exception hierarchy diagram
   - All 13 exception types documented
   - When each exception is thrown
   - Error code handling
   - Platform-specific error codes
   - 5 error handling patterns
   - Best practices
   - Complete robust client example

4. **[examples.md](examples.md)** - Practical Examples
   - 6 complete, working examples:
     1. Simple TCP Client
     2. TCP Echo Server
     3. HTTP Client
     4. Multi-threaded Chat Server
     5. UDP Ping-Pong
     6. File Transfer Over TCP
   - Best practices
   - Integration examples (logging, JSON, threading)

5. **[socket_base.md](socket_base.md)** - Base Socket Class
   - Socket types and options
   - Blocking/non-blocking modes
   - Timeouts and buffer management
   - Socket state queries

6. **[tcp_client.md](tcp_client.md)** - TCP Client Socket
   - Connection establishment
   - Data transfer methods
   - TCP-specific options (TCP_NODELAY, keepalive)
   - Graceful shutdown
   - Signal support for event-driven programming

7. **[server_socket.md](server_socket.md)** - TCP Server Socket
   - Binding and listening
   - Accepting connections
   - Server configuration

8. **[socket_stream.md](socket_stream.md)** - Stream Interface
   - iostream-compatible interface
   - Stream operations
   - Buffer management

9. **[poll_set.md](poll_set.md)** - Multi-Socket Polling
   - I/O multiplexing
   - Event monitoring
   - Efficient multi-socket handling

10. **[udp_socket.md](udp_socket.md)** - UDP Socket
    - Datagram operations
    - Broadcast and multicast
    - Connectionless communication

11. **[tcp_server.md](tcp_server.md)** - Multi-Threaded TCP Server
    - Thread pool management
    - Connection factory pattern
    - Server lifecycle
    - Configuration options

12. **[tcp_server_connection.md](tcp_server_connection.md)** - Connection Handler
    - Custom connection handlers
    - Connection lifecycle
    - Thread context

13. **[udp_server.md](udp_server.md)** - Multi-Threaded UDP Server
    - Packet dispatching
    - Handler pattern
    - Server configuration

14. **[udp_handler.md](udp_handler.md)** - UDP Packet Handler
    - Packet processing
    - Handler interface

15. **[udp_client.md](udp_client.md)** - High-Level UDP Client
    - Simplified UDP operations
    - Client-side features

### Supporting Documentation

- **[SIGNAL_INTEGRATION_SUMMARY.md](SIGNAL_INTEGRATION_SUMMARY.md)** - Signal integration across components
- **[DOCUMENTATION_STATUS.md](DOCUMENTATION_STATUS.md)** - Detailed status and metrics

## Documentation Style Guide

All fb_net documentation follows the same comprehensive style as fb_strings:

### Structure
- **Overview** - What the component is and why it's useful
- **Quick Start** - Immediate usage example
- **API Reference** - Complete function/class documentation
  - Constructor details
  - Member function documentation
  - Parameters, returns, exceptions
- **Examples** - 3-5 complete, practical examples
- **Common Patterns** - Reusable code patterns
- **Error Handling** - Exception handling strategies
- **Performance** - Performance considerations
- **Platform Notes** - Platform-specific details
- **Complete API Reference Table** - Quick lookup
- **See Also** - Cross-references

### Code Examples
- Complete and compilable
- Include error handling
- Show real-world usage
- Explain key techniques

### Formatting
- GitHub-flavored Markdown
- Code syntax highlighting (cpp)
- Tables for reference data
- Consistent headers and sections
- Cross-linking between documents

## Getting Started

### For Users
Start with **[index.md](index.md)** for the library overview, then:
1. Read the Quick Start section
2. Check **[examples.md](examples.md)** for practical usage
3. Dive into component-specific documentation as needed
4. Reference **[exceptions.md](exceptions.md)** for error handling

### For Contributors
When creating new documentation:
1. Follow the structure defined in this README
2. Match the depth and style of existing documentation
3. Include complete, working code examples
4. Cross-reference related components
5. Add performance and platform considerations

## Building Examples

All examples in the documentation can be built using:

```bash
cd fb_stage
mkdir build && cd build
cmake .. -DFB_NET_BUILD_EXAMPLES=ON
make -j$(nproc)

# Run examples
./fb_net/examples/http_server/http_server 8080
./fb_net/examples/chat_server/chat_server 9090
```


## Documentation Quality Standards

All `fb_net` documentation must meet these standards:

**Completeness**
- Every public API documented
- All parameters and return values explained
- Exceptions documented

**Clarity**
- Clear, concise explanations
- Practical examples for every feature
- Common use cases covered

**Correctness**
- Verified against actual implementation
- Code examples compile and run
- Platform differences noted

**Consistency**
- Uniform structure across all files
- Consistent terminology
- Cross-references maintained

**Comprehensiveness**
- Quick Start for beginners
- API Reference for developers
- Examples for practitioners
- Best Practices for experts

## Contributing to Documentation

To add or improve documentation:

1. **Match the Style**: Follow the structure of existing documents
2. **Include Examples**: Provide complete, working code examples
3. **Test Code**: Ensure all code examples compile and run
4. **Cross-Reference**: Link related documents
5. **Performance Notes**: Include performance considerations
6. **Platform Notes**: Document platform-specific behavior
7. **Error Handling**: Show proper exception handling

## Maintenance

This documentation is maintained alongside the `fb_net` library:

- **Version**: 1.0.0 - Complete
- **Status**: All documentation complete (11,450+ lines)
- **Coverage**: 100% of public API
- **Last Updated**: 2025
- **Maintained By**: Randy York

For questions or suggestions about the documentation, please open an issue in the project repository.

---

*fb_net - Modern C++ networking*
