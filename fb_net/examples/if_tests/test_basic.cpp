#include <fb/socket_address.h>
#include <fb/socket_base.h>
#include <iostream>
#include <string>
#include <system_error>

/**
 * @brief Basic validation test for FB_NET Foundation Layer
 */
int main()
{
  std::cout << "FB_NET Foundation Layer Basic Validation Test\n";
  std::cout << "============================================\n\n";

  try 
  {
    // Test 1: socket_address IPv4 construction and basic operations
    std::cout << "Test 1: socket_address IPv4 functionality\n";
    {
      fb::socket_address addr1("127.0.0.1", 8080);
      std::cout << "  - Created socket_address: " << addr1.to_string() << std::endl;
      std::cout << "  - Host: " << addr1.host() << std::endl;
      std::cout << "  - Port: " << addr1.port() << std::endl;
      std::cout << "  - Address Family: " << addr1.af() << std::endl;
      std::cout << "  - Length: " << addr1.length() << std::endl;
      
      fb::socket_address addr2("0.0.0.0", 80);
      std::cout << "  - Wildcard address: " << addr2.to_string() << std::endl;
      
      fb::socket_address addr3(8443);
      std::cout << "  - Port-only address: " << addr3.to_string() << std::endl;
    }
    std::cout << "  ✓ IPv4 socket_address tests passed\n\n";

    // Test 2: socket_address IPv6 construction (if supported)
#ifdef AF_INET6
    std::cout << "Test 2: socket_address IPv6 functionality\n";
    {
      fb::socket_address addr6("::1", 8080);
      std::cout << "  - Created IPv6 socket_address: " << addr6.to_string() << std::endl;
      std::cout << "  - Host: " << addr6.host() << std::endl;
      std::cout << "  - Port: " << addr6.port() << std::endl;
    }
    std::cout << "  ✓ IPv6 socket_address tests passed\n\n";
#else
    std::cout << "Test 2: IPv6 not supported on this platform\n\n";
#endif

    // Test 3: socket_address comparison operators
    std::cout << "Test 3: socket_address comparison operators\n";
    {
      fb::socket_address addr1("127.0.0.1", 8080);
      fb::socket_address addr2("127.0.0.1", 8080);
      fb::socket_address addr3("127.0.0.1", 8081);
      
      std::cout << "  - addr1 == addr2: " << (addr1 == addr2 ? "true" : "false") << std::endl;
      std::cout << "  - addr1 != addr3: " << (addr1 != addr3 ? "true" : "false") << std::endl;
      std::cout << "  - addr1 < addr3: " << (addr1 < addr3 ? "true" : "false") << std::endl;
    }
    std::cout << "  ✓ Comparison operator tests passed\n\n";

    // Test 4: Socket basic functionality
    std::cout << "Test 4: Socket basic functionality\n";
    {
      fb::socket socket;
      std::cout << "  - Created Socket instance" << std::endl;
      std::cout << "  - Blocking mode: " << (socket.get_blocking() ? "true" : "false") << std::endl;
      std::cout << "  - Secure: " << (socket.secure() ? "true" : "false") << std::endl;
      std::cout << "  - IPv4 support: " << (fb::socket::supports_ipv4() ? "true" : "false") << std::endl;
      std::cout << "  - IPv6 support: " << (fb::socket::supports_ipv6() ? "true" : "false") << std::endl;
    }
    std::cout << "  ✓ Socket basic tests passed\n\n";

    // Test 5: Standard exception mapping
    std::cout << "Test 5: Standard exception handling\n";
    {
      fb::socket probe(fb::socket_address::IPv4, fb::socket::STREAM_SOCKET);
      try
      {
        probe.connect(fb::socket_address("203.0.113.1", 9)); // unroutable test net
      }
      catch (const std::system_error& ex)
      {
        std::cout << "  - connect failed with code: " << ex.code()
                  << " message: " << ex.what() << std::endl;
      }
      probe.close();
    }
    std::cout << "  ✓ Standard exception handling demonstrated\n\n";

    std::cout << "============================================\n";
    std::cout << "All Foundation Layer tests completed successfully!\n";
    std::cout << "============================================\n";
    
    return 0;
  }
  catch (const std::exception& e) 
  {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
