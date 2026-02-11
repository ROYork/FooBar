# Cross-Platform Compatibility Guide

This document describes platform-specific behaviors and how fb_net handles them across Linux, Windows, and macOS.

## Socket Address Functions

### getsockname() / getpeername()

**Issue:** Different behavior for unbound/unconnected sockets

| Platform | Unbound Socket Behavior |
|----------|------------------------|
| Linux/macOS | Returns wildcard address (0.0.0.0:0) |
| Windows | Returns error WSAEINVAL (10022) |

**Solution:** On Windows, we catch WSAEINVAL and return a wildcard address to match Unix behavior.

**Code Location:** `fb_net/src/socket_base.cpp` - `address()` and `peer_address()` methods

---

## Non-Blocking Connect

### SO_ERROR Reliability

**Issue:** Error code availability after non-blocking connect

| Platform | SO_ERROR Behavior |
|----------|------------------|
| Linux/macOS | Reliable - contains correct error code |
| Windows | Unreliable - may be 0 even when connection failed |

**Solution:** On Windows, after `poll()` indicates the socket is ready, we perform a second `connect()` call to get the actual error status. The second call will:
- Return WSAEISCONN if connection succeeded
- Return the actual error code if connection failed

**Code Location:** `fb_net/src/socket_base.cpp` - `connect()` with timeout

**References:**
- [Microsoft: Non-blocking sockets](https://docs.microsoft.com/en-us/windows/win32/winsock/winsock-nonblocking-sockets)

---

## Address Reuse (SO_REUSEADDR)

### Semantic Differences

**Issue:** SO_REUSEADDR has drastically different meanings across platforms

| Platform | SO_REUSEADDR Behavior | Security Risk |
|----------|----------------------|--------------|
| Linux/macOS | Allows binding to TIME_WAIT ports, but NOT active ports | Low |
| Windows | Allows COMPLETE duplicate binding to active ports | **HIGH - Port Hijacking** |

**Unix Behavior:**
```
Server 1: bind(8080) + listen()     ✓ Success
Server 2: bind(8080) with SO_REUSEADDR=false  ✗ EADDRINUSE
```

**Windows Default Behavior:**
```
Server 1: bind(8080) + listen()     ✓ Success
Server 2: bind(8080) with SO_REUSEADDR=false  ✓ Success (!)
```

### Solution

On Windows, we use **SO_EXCLUSIVEADDRUSE** instead of disabling SO_REUSEADDR:

```cpp
// When user calls set_reuse_address(false):
#ifdef _WIN32
    setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, 0);
#endif
```

**SO_EXCLUSIVEADDRUSE:**
- Windows-specific option (available since Windows 2000)
- Prevents ANY address reuse
- Must be set BEFORE bind()
- Provides Unix-like protection against port hijacking

**Code Location:** `fb_net/src/socket_base.cpp` - `set_reuse_address()`

**References:**
- [Microsoft: Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE](https://docs.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse)

---

## Error Code Mapping

### Platform-Specific Error Codes

| Standard Error | Linux | Windows | macOS |
|----------------|-------|---------|-------|
| Connection Refused | ECONNREFUSED (111) | WSAECONNREFUSED (10061) | ECONNREFUSED (61) |
| Timeout | ETIMEDOUT (110) | WSAETIMEDOUT (10060) | ETIMEDOUT (60) |
| Would Block | EAGAIN (11) | WSAEWOULDBLOCK (10035) | EAGAIN (35) |
| Invalid Argument | EINVAL (22) | WSAEINVAL (10022) | EINVAL (22) |

**Solution:** `make_error_code_from_native()` maps platform-specific codes to `std::errc` standard codes.

**Code Location:** `fb_net/src/socket_base.cpp` - `make_error_code_from_native()`

---

## Testing Considerations

### Platform-Specific Tests

Some tests may need platform-specific expectations:

1. **Address Reuse Tests**: Windows behavior is more permissive
2. **Timeout Tests**: May have different timing characteristics
3. **Error Code Tests**: Should expect `std::errc` codes, not platform-specific codes

### Example: Platform-Aware Test

```cpp
TEST(SocketTest, BindPortInUse) {
    server_socket server1(AF_INET);
    server1.bind(socket_address("127.0.0.1", 0));
    auto addr = server1.address();
    server1.listen();

    server_socket server2(AF_INET);
    server2.set_reuse_address(false);

#ifdef _WIN32
    // Windows with SO_EXCLUSIVEADDRUSE should prevent duplicate bind
    EXPECT_THROW(server2.bind(addr), std::system_error);
#else
    // Unix with SO_REUSEADDR=false should prevent duplicate bind
    EXPECT_THROW(server2.bind(addr), std::system_error);
#endif
}
```

---

## Best Practices for Cross-Platform Development

### 1. Always Test on All Target Platforms

Don't assume Unix behavior on Windows or vice versa.

### 2. Use Standard Error Codes

Compare against `std::errc` codes, not platform-specific codes:

```cpp
// ✓ Good
if (ex.code() == std::make_error_code(std::errc::connection_refused))

// ✗ Bad
if (ex.code().value() == ECONNREFUSED)  // Platform-specific!
```

### 3. Set Socket Options Before Operations

Some options (like SO_EXCLUSIVEADDRUSE) must be set before bind():

```cpp
socket.set_reuse_address(false);  // Must come before bind()
socket.bind(address);
```

### 4. Handle Timing Differences

Network operations may have different timing characteristics:
- Windows may buffer more aggressively
- Timeouts may round differently
- Use generous timeouts in tests (>100ms)

### 5. Document Platform Differences

When encountering platform-specific behavior:
1. Document it in code comments
2. Add to this guide
3. Write platform-aware tests

---

## Future Considerations

### macOS Specific Issues

While macOS generally follows BSD/Unix semantics, be aware of:
- `SO_REUSEPORT` availability and behavior
- Potential differences in socket buffer sizes
- IPv6 socket binding differences

### Windows-Specific Features

Windows supports additional features that may be useful:
- `WSAPoll()` as an alternative to `select()`
- Completion ports for high-performance servers
- `AcceptEx()` for optimized accept operations

---

## Version History

- **v1.0** (2025-01): Initial cross-platform compatibility implementation
  - Fixed getsockname()/getpeername() for unbound sockets
  - Implemented reliable SO_ERROR checking on Windows
  - Added SO_EXCLUSIVEADDRUSE support for proper address reuse semantics

---

## References

- [Microsoft Winsock Documentation](https://docs.microsoft.com/en-us/windows/win32/winsock/)
- [POSIX Socket API](https://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html)
- [Berkeley Socket API](https://en.wikipedia.org/wiki/Berkeley_sockets)
- [SO_REUSEADDR vs SO_EXCLUSIVEADDRUSE](https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ)
