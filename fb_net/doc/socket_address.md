# fb::socket_address - Network Address Abstraction

## Overview

The [`fb::socket_address`](../include/fb/socket_address.h) class provides a platform-independent representation of network endpoints with support for IPv4, IPv6, and automatic DNS resolution. It encapsulates socket address structures (`sockaddr_in`, `sockaddr_in6`) and provides a clean, type-safe interface for network addressing.

**Key Features:**
- IPv4 and IPv6 address support
- Automatic DNS name resolution
- Multiple construction formats
- String parsing and generation
- Comparison operators for container use
- Platform-independent implementation
- Exception-safe operations

## Quick Start

```cpp
#include <fb/socket_address.h>

using namespace fb;

// Create from host and port
socket_address addr1("example.com", 80);

// Create from IP and port
socket_address addr2("192.168.1.1", 8080);

// Create from string "host:port"
socket_address addr3("127.0.0.1:9000");

// Create IPv6 address
socket_address addr4(socket_address::IPv6, "::1", 8080);

// Get address components
std::string host = addr1.host();      // "example.com" (or resolved IP)
uint16_t port = addr1.port();         // 80
std::string str = addr1.to_string();  // "example.com:80"
```

---

## Address Families

### AddressFamily Enumeration

```cpp
enum class AddressFamily : int
{
    IPv4 = AF_INET,
    IPv6 = AF_INET6,      // If supported
    UNIX_LOCAL = AF_UNIX  // If supported
};

// Convenience aliases in socket_address class
class socket_address {
public:
    using Family = AddressFamily;
    static constexpr Family IPv4 = AddressFamily::IPv4;
    static constexpr Family IPv6 = AddressFamily::IPv6;  // If supported
};
```

**Supported Families:**

| Family | Description | Platform Support |
|--------|-------------|------------------|
| `IPv4` | Internet Protocol version 4 | All platforms |
| `IPv6` | Internet Protocol version 6 | Linux, macOS, Windows |
| `UNIX_LOCAL` | Unix domain sockets | Linux, macOS |

**Example:**

```cpp
// Use IPv4 explicitly
socket_address ipv4_addr(socket_address::IPv4, "192.168.1.1", 8080);

// Use IPv6 explicitly
socket_address ipv6_addr(socket_address::IPv6, "fe80::1", 8080);

// Default family (IPv4)
socket_address default_addr("127.0.0.1", 8080);
```

---

## Construction

### Default Constructor

Creates an empty/uninitialized address.

```cpp
socket_address();
```

**Example:**
```cpp
socket_address addr;
// addr is default-constructed (typically IPv4 with 0.0.0.0:0)
```

---

### Family Constructor

Creates an address for the specified family with default values.

```cpp
explicit socket_address(Family family);
```

**Parameters:**
- `family` - Address family (IPv4, IPv6)

**Example:**
```cpp
socket_address ipv4_addr(socket_address::IPv4);
socket_address ipv6_addr(socket_address::IPv6);
```

---

### Port-Only Constructor

Creates an address with INADDR_ANY and the specified port.

```cpp
explicit socket_address(std::uint16_t port);
```

**Parameters:**
- `port` - Port number (0-65535)

**Example:**
```cpp
socket_address addr(8080);
// Creates 0.0.0.0:8080 (binds to all interfaces)
```

---

### Family and Port Constructor

Creates an address for the specified family and port.

```cpp
socket_address(Family family, std::uint16_t port);
```

**Parameters:**
- `family` - Address family
- `port` - Port number

**Example:**
```cpp
socket_address addr(socket_address::IPv6, 8080);
// Creates [::]:8080 (IPv6 any address)
```

---

### Host and Port Constructor

Creates an address from host name/IP and port number.

```cpp
socket_address(std::string_view host_address, std::uint16_t port_number);
```

**Parameters:**
- `host_address` - Host name or IP address
- `port_number` - Port number

**Behavior:**
- Performs DNS resolution if `host_address` is a name
- Accepts IPv4 dotted notation (e.g., "192.168.1.1")
- Accepts IPv6 notation (e.g., "::1", "fe80::1")
- Throws ``std::system_error` (std::errc::host_unreachable)` if resolution fails

**Example:**
```cpp
// DNS resolution
socket_address addr1("example.com", 80);

// IPv4 address
socket_address addr2("192.168.1.1", 8080);

// IPv6 address
socket_address addr3("::1", 9000);

// Localhost
socket_address addr4("localhost", 3000);
```

---

### Family, Host, and Port Constructor

Creates an address with explicit address family.

```cpp
socket_address(Family family, std::string_view host_address,
               std::uint16_t port_number);
```

**Parameters:**
- `family` - Address family to use
- `host_address` - Host name or IP address
- `port_number` - Port number

**Example:**
```cpp
// Force IPv4 resolution
socket_address addr1(socket_address::IPv4, "localhost", 8080);

// Force IPv6 resolution
socket_address addr2(socket_address::IPv6, "localhost", 8080);
```

---

### String Port Constructor

Creates an address from host and port as strings.

```cpp
socket_address(std::string_view host_address, std::string_view port_number);
```

**Parameters:**
- `host_address` - Host name or IP address
- `port_number` - Port number as string (or service name)

**Example:**
```cpp
socket_address addr1("example.com", "80");
socket_address addr2("localhost", "http");  // Resolves "http" to 80
socket_address addr3("192.168.1.1", "8080");
```

---

### Family, Host, and String Port Constructor

Creates an address with explicit family and string port.

```cpp
socket_address(Family family, std::string_view host_address,
               std::string_view port_number);
```

**Example:**
```cpp
socket_address addr(socket_address::IPv4, "localhost", "http");
```

---

### Combined String Constructor

Parses "host:port" format string.

```cpp
explicit socket_address(std::string_view host_and_port);
```

**Parameters:**
- `host_and_port` - String in "host:port" format

**Supported Formats:**
- `"example.com:80"`
- `"192.168.1.1:8080"`
- `"[::1]:9000"` (IPv6 with brackets)
- `"[fe80::1%eth0]:8080"` (IPv6 with zone ID)

**Example:**
```cpp
socket_address addr1("example.com:80");
socket_address addr2("192.168.1.1:8080");
socket_address addr3("[::1]:9000");
socket_address addr4("localhost:http");
```

---

### Family and Combined String Constructor

Parses string with explicit address family.

```cpp
socket_address(Family family, std::string_view addr);
```

**Example:**
```cpp
socket_address addr1(socket_address::IPv4, "localhost:8080");
socket_address addr2(socket_address::IPv6, "[::1]:9000");
```

---

### Copy and Move Constructors

```cpp
socket_address(const socket_address& other);
socket_address(socket_address&& other) noexcept;
```

**Example:**
```cpp
socket_address addr1("example.com", 80);
socket_address addr2(addr1);              // Copy
socket_address addr3(std::move(addr1));   // Move
```

---

### Raw Socket Address Constructor

Creates from raw `sockaddr` structure.

```cpp
socket_address(const struct sockaddr* addr, socklen_t length);
```

**Parameters:**
- `addr` - Pointer to sockaddr structure
- `length` - Length of the structure

**Use Case:**
- Interoperability with raw socket APIs
- Converting from system-level socket addresses

**Example:**
```cpp
// From accept() call
struct sockaddr_storage storage;
socklen_t len = sizeof(storage);
int client_fd = accept(server_fd, (struct sockaddr*)&storage, &len);

socket_address client_addr((struct sockaddr*)&storage, len);
std::cout << "Client: " << client_addr.to_string() << std::endl;
```

---

## Member Functions

### host()

Returns the host address as a string.

```cpp
std::string host() const;
```

**Returns:** Host address in string format (IP address)

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
std::string host = addr.host();  // "192.168.1.1"
```

---

### port()

Returns the port number.

```cpp
std::uint16_t port() const;
```

**Returns:** Port number (0-65535)

**Example:**
```cpp
socket_address addr("example.com", 8080);
uint16_t port = addr.port();  // 8080
```

---

### to_string()

Returns the complete address as "host:port" string.

```cpp
std::string to_string() const;
```

**Returns:** Address in "host:port" format

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
std::string str = addr.to_string();  // "192.168.1.1:8080"
```

---

### family()

Returns the address family.

```cpp
Family family() const;
```

**Returns:** AddressFamily (IPv4, IPv6, UNIX_LOCAL)

**Example:**
```cpp
socket_address addr(socket_address::IPv6, "::1", 9000);
if (addr.family() == socket_address::IPv6) {
    std::cout << "IPv6 address" << std::endl;
}
```

---

### af()

Returns the raw address family constant.

```cpp
int af() const;
```

**Returns:** `AF_INET`, `AF_INET6`, etc.

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
int af = addr.af();  // AF_INET
```

---

### addr()

Returns pointer to the underlying `sockaddr` structure.

```cpp
const struct sockaddr* addr() const;
```

**Returns:** Pointer to sockaddr structure

**Use Case:**
- Passing to raw socket APIs (`bind()`, `connect()`, etc.)

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
::bind(sockfd, addr.addr(), addr.length());
```

---

### length()

Returns the length of the underlying socket address structure.

```cpp
socklen_t length() const;
```

**Returns:** Structure length in bytes

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
socklen_t len = addr.length();  // sizeof(sockaddr_in)
```

---

## Operators

### Comparison Operators

```cpp
bool operator==(const socket_address& other) const;
bool operator!=(const socket_address& other) const;
bool operator<(const socket_address& other) const;
```

**Use Cases:**
- Comparing addresses for equality
- Using addresses as map/set keys
- Sorting addresses

**Example:**
```cpp
socket_address addr1("192.168.1.1", 8080);
socket_address addr2("192.168.1.1", 8080);
socket_address addr3("192.168.1.2", 8080);

if (addr1 == addr2) {
    std::cout << "Addresses are equal" << std::endl;
}

// Use in containers
std::set<socket_address> unique_addresses;
unique_addresses.insert(addr1);
unique_addresses.insert(addr2);  // Won't be added (duplicate)
unique_addresses.insert(addr3);  // Will be added

std::map<socket_address, std::string> address_map;
address_map[addr1] = "Server 1";
address_map[addr3] = "Server 2";
```

---

### Stream Insertion Operator

```cpp
std::ostream& operator<<(std::ostream& ostr, const socket_address& address);
```

**Use Case:**
- Printing addresses to output streams

**Example:**
```cpp
socket_address addr("192.168.1.1", 8080);
std::cout << "Address: " << addr << std::endl;
// Output: Address: 192.168.1.1:8080
```

---

## Complete Examples

### Example 1: DNS Resolution

```cpp
#include <fb/socket_address.h>
#include <iostream>

void demonstrate_dns_resolution()
{
    using namespace fb;

    try {
        // Resolve domain name
        socket_address addr("example.com", 80);

        std::cout << "Domain: example.com:80" << std::endl;
        std::cout << "Resolved to: " << addr.host() << ":" << addr.port() << std::endl;
        std::cout << "Family: " << (addr.family() == socket_address::IPv4 ? "IPv4" : "IPv6") << std::endl;

    } catch (const `std::system_error` (std::errc::host_unreachable)& e) {
        std::cerr << "DNS resolution failed: " << e.what() << std::endl;
    }
}
```

---

### Example 2: Address Parsing

```cpp
#include <fb/socket_address.h>
#include <iostream>
#include <vector>

void demonstrate_address_parsing()
{
    using namespace fb;

    std::vector<std::string> address_strings = {
        "192.168.1.1:8080",
        "example.com:80",
        "[::1]:9000",
        "localhost:http",
        "[fe80::1%eth0]:8080"
    };

    std::cout << "Parsing Addresses:\n";
    std::cout << "------------------\n";

    for (const auto& addr_str : address_strings) {
        try {
            socket_address addr(addr_str);
            std::cout << "Input:    " << addr_str << std::endl;
            std::cout << "Host:     " << addr.host() << std::endl;
            std::cout << "Port:     " << addr.port() << std::endl;
            std::cout << "Family:   " << (addr.family() == socket_address::IPv4 ? "IPv4" : "IPv6") << std::endl;
            std::cout << "String:   " << addr.to_string() << std::endl;
            std::cout << std::endl;

        } catch (const std::system_error& e) {
            std::cerr << "Failed to parse '" << addr_str << "': " << e.what() << std::endl;
        }
    }
}
```

---

### Example 3: Server Binding Addresses

```cpp
#include <fb/socket_address.h>
#include <iostream>

void demonstrate_server_addresses()
{
    using namespace fb;

    // Bind to all interfaces
    socket_address any_ipv4(8080);
    std::cout << "Bind to all IPv4 interfaces: " << any_ipv4.to_string() << std::endl;
    // Output: 0.0.0.0:8080

    socket_address any_ipv6(socket_address::IPv6, 8080);
    std::cout << "Bind to all IPv6 interfaces: " << any_ipv6.to_string() << std::endl;
    // Output: [::]:8080

    // Bind to localhost only
    socket_address localhost_ipv4("127.0.0.1", 8080);
    std::cout << "Bind to IPv4 localhost: " << localhost_ipv4.to_string() << std::endl;
    // Output: 127.0.0.1:8080

    socket_address localhost_ipv6(socket_address::IPv6, "::1", 8080);
    std::cout << "Bind to IPv6 localhost: " << localhost_ipv6.to_string() << std::endl;
    // Output: [::1]:8080

    // Bind to specific interface
    socket_address specific("192.168.1.100", 8080);
    std::cout << "Bind to specific interface: " << specific.to_string() << std::endl;
    // Output: 192.168.1.100:8080
}
```

---

### Example 4: Container Usage

```cpp
#include <fb/socket_address.h>
#include <iostream>
#include <set>
#include <map>

void demonstrate_container_usage()
{
    using namespace fb;

    // Use in std::set for unique addresses
    std::set<socket_address> servers;
    servers.insert(socket_address("192.168.1.1", 8080));
    servers.insert(socket_address("192.168.1.2", 8080));
    servers.insert(socket_address("192.168.1.1", 8080));  // Duplicate, won't be added

    std::cout << "Unique servers: " << servers.size() << std::endl;

    // Use in std::map for address-to-data mapping
    std::map<socket_address, std::string> server_names;
    server_names[socket_address("192.168.1.1", 8080)] = "Primary Server";
    server_names[socket_address("192.168.1.2", 8080)] = "Backup Server";

    for (const auto& [addr, name] : server_names) {
        std::cout << addr.to_string() << " -> " << name << std::endl;
    }
}
```

---

### Example 5: IPv4 vs IPv6

```cpp
#include <fb/socket_address.h>
#include <iostream>

void demonstrate_ipv4_vs_ipv6()
{
    using namespace fb;

    // Dual-stack server addresses
    socket_address ipv4_addr(socket_address::IPv4, "0.0.0.0", 8080);
    socket_address ipv6_addr(socket_address::IPv6, "::", 8080);

    std::cout << "IPv4 any address: " << ipv4_addr.to_string() << std::endl;
    std::cout << "IPv6 any address: " << ipv6_addr.to_string() << std::endl;

    // Force specific IP version for DNS resolution
    try {
        socket_address ipv4_only(socket_address::IPv4, "google.com", 80);
        std::cout << "Google (IPv4): " << ipv4_only.to_string() << std::endl;
    } catch (const `std::system_error` (std::errc::host_unreachable)& e) {
        std::cerr << "IPv4 resolution failed: " << e.what() << std::endl;
    }

    try {
        socket_address ipv6_only(socket_address::IPv6, "google.com", 80);
        std::cout << "Google (IPv6): " << ipv6_only.to_string() << std::endl;
    } catch (const `std::system_error` (std::errc::host_unreachable)& e) {
        std::cerr << "IPv6 resolution failed: " << e.what() << std::endl;
    }
}
```

---

## Common Patterns

### Pattern 1: Flexible Server Binding

```cpp
socket_address create_server_address(const std::string& bind_address, uint16_t port)
{
    if (bind_address.empty() || bind_address == "0.0.0.0") {
        // Bind to all IPv4 interfaces
        return socket_address(port);
    } else if (bind_address == "::") {
        // Bind to all IPv6 interfaces
        return socket_address(socket_address::IPv6, port);
    } else {
        // Bind to specific address
        return socket_address(bind_address, port);
    }
}
```

---

### Pattern 2: Address Validation

```cpp
bool is_valid_address(const std::string& addr_str)
{
    try {
        socket_address addr(addr_str);
        return true;
    } catch (const std::system_error&) {
        return false;
    }
}
```

---

### Pattern 3: Address List from Configuration

```cpp
std::vector<socket_address> parse_server_list(const std::vector<std::string>& server_strings)
{
    std::vector<socket_address> servers;

    for (const auto& server_str : server_strings) {
        try {
            servers.push_back(socket_address(server_str));
        } catch (const std::system_error& e) {
            std::cerr << "Invalid address '" << server_str << "': " << e.what() << std::endl;
        }
    }

    return servers;
}
```

---

### Pattern 4: Connection with Fallback

```cpp
bool try_connect_with_fallback(tcp_client& socket, const std::string& host, uint16_t port)
{
    // Try IPv4 first
    try {
        socket_address addr(socket_address::IPv4, host, port);
        socket.connect(addr);
        return true;
    } catch (const std::system_error&) {
        // IPv4 failed, try IPv6
        try {
            socket_address addr(socket_address::IPv6, host, port);
            socket.connect(addr);
            return true;
        } catch (const std::system_error&) {
            return false;
        }
    }
}
```

---

## Error Handling

### Exceptions Thrown

| Exception | When Thrown |
|-----------|-------------|
| ``std::system_error` (std::errc::host_unreachable)` | DNS resolution fails |
| ``std::runtime_error`` | Invalid address format |
| `AddressFamilyMismatchException` | Incompatible address family |
| `ServiceNotFoundException` | Service name not found |

### Error Handling Example

```cpp
void safe_address_creation(const std::string& host, uint16_t port)
{
    try {
        socket_address addr(host, port);
        std::cout << "Address created: " << addr.to_string() << std::endl;

    } catch (const `std::system_error` (std::errc::host_unreachable)& e) {
        std::cerr << "Host not found: " << e.what() << std::endl;
        // Handle DNS failure

    } catch (const `std::runtime_error`& e) {
        std::cerr << "Invalid address format: " << e.what() << std::endl;
        // Handle format error

    } catch (const std::system_error& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
        // Handle other network errors
    }
}
```

---

## Performance Considerations

### DNS Resolution Overhead

DNS resolution is performed during construction when a hostname is provided:

```cpp
// DNS lookup performed here (potentially slow)
socket_address addr1("example.com", 80);

// No DNS lookup (IP address provided)
socket_address addr2("192.168.1.1", 80);
```

**Best Practices:**
1. Cache resolved addresses when possible
2. Use IP addresses directly for known hosts
3. Perform DNS resolution in background threads for UI applications
4. Consider DNS timeout implications in time-sensitive code

---

### Address Caching Pattern

```cpp
class AddressCache {
public:
    socket_address get_or_resolve(const std::string& host, uint16_t port)
    {
        std::string key = host + ":" + std::to_string(port);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;  // Return cached address
        }

        // Resolve and cache
        socket_address addr(host, port);
        cache_[key] = addr;
        return addr;
    }

private:
    std::map<std::string, socket_address> cache_;
};
```

---

## Platform-Specific Considerations

### IPv6 Support

IPv6 support varies by platform:

```cpp
// Check IPv6 support at runtime
if constexpr (sizeof(socket_address::MAX_ADDRESS_LENGTH) > sizeof(sockaddr_in)) {
    // IPv6 is compiled in
    socket_address ipv6_addr(socket_address::IPv6, "::1", 8080);
}
```

### Windows Considerations

- Winsock must be initialized before using socket addresses
- fb_net handles initialization automatically via `fb::initialize_library()`

---

## Complete API Reference

| Member | Type | Description |
|--------|------|-------------|
| **Constructors** | | |
| `socket_address()` | Constructor | Default constructor |
| `socket_address(Family)` | Constructor | Create with address family |
| `socket_address(uint16_t port)` | Constructor | Create with port (any address) |
| `socket_address(Family, uint16_t)` | Constructor | Create with family and port |
| `socket_address(string_view, uint16_t)` | Constructor | Create with host and port |
| `socket_address(Family, string_view, uint16_t)` | Constructor | Create with family, host, port |
| `socket_address(string_view, string_view)` | Constructor | Create with host and port strings |
| `socket_address(Family, string_view, string_view)` | Constructor | Create with family, host, port strings |
| `socket_address(string_view)` | Constructor | Parse "host:port" string |
| `socket_address(Family, string_view)` | Constructor | Parse with explicit family |
| `socket_address(const sockaddr*, socklen_t)` | Constructor | Create from raw sockaddr |
| **Member Functions** | | |
| `host()` | Method | Get host address string |
| `port()` | Method | Get port number |
| `to_string()` | Method | Get "host:port" string |
| `family()` | Method | Get address family |
| `af()` | Method | Get raw address family constant |
| `addr()` | Method | Get pointer to sockaddr structure |
| `length()` | Method | Get sockaddr structure length |
| **Operators** | | |
| `operator==` | Operator | Equality comparison |
| `operator!=` | Operator | Inequality comparison |
| `operator<` | Operator | Less-than comparison (for containers) |
| **Constants** | | |
| `MAX_ADDRESS_LENGTH` | Constant | Maximum address structure size |

---

## See Also

- [`socket_base.md`](socket_base.md) - Base socket class
- [`tcp_client.md`](tcp_client.md) - TCP client sockets
- [`exceptions.md`](exceptions.md) - Network exception hierarchy
- [`examples.md`](examples.md) - Practical usage examples

---

*socket_address - Type-safe network addressing for modern C++*
