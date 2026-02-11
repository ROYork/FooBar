/**
 * @file test_socket_address.cpp
 * @brief Unit tests for socket_address class
 */

#include <gtest/gtest.h>
#include <fb/socket_address.h>
#include <stdexcept>

using namespace fb;

class socket_address_Test : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(socket_address_Test, Construction_BasicIPv4)
{
    socket_address addr("127.0.0.1", 80);
    
    EXPECT_EQ(addr.host(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 80);
    EXPECT_EQ(addr.family(), socket_address::Family::IPv4);
    EXPECT_FALSE(addr.to_string().empty());
}

TEST_F(socket_address_Test, Construction_ServiceName)
{
    socket_address addr("localhost", "http");
    
    EXPECT_EQ(addr.host(), "127.0.0.1");  // localhost resolves to 127.0.0.1
    EXPECT_EQ(addr.port(), 80);           // http service is port 80
    EXPECT_EQ(addr.family(), socket_address::Family::IPv4);
}

TEST_F(socket_address_Test, Construction_IPv6)
{
    socket_address addr("::1", 80);
    
    EXPECT_EQ(addr.host(), "::1");
    EXPECT_EQ(addr.port(), 80);
    EXPECT_EQ(addr.family(), socket_address::Family::IPv6);
}

TEST_F(socket_address_Test, Construction_FamilyOnly)
{
    socket_address addr(socket_address::Family::IPv4);
    
    EXPECT_EQ(addr.family(), socket_address::Family::IPv4);
    EXPECT_EQ(addr.port(), 0);
}

TEST_F(socket_address_Test, CopyConstruction)
{
    socket_address original("192.168.1.1", 8080);
    socket_address copy(original);
    
    EXPECT_EQ(copy.host(), original.host());
    EXPECT_EQ(copy.port(), original.port());
    EXPECT_EQ(copy.family(), original.family());
}

TEST_F(socket_address_Test, Assignment)
{
    socket_address addr1("127.0.0.1", 80);
    socket_address addr2("192.168.1.1", 443);
    
    addr2 = addr1;
    
    EXPECT_EQ(addr2.host(), addr1.host());
    EXPECT_EQ(addr2.port(), addr1.port());
    EXPECT_EQ(addr2.family(), addr1.family());
}

TEST_F(socket_address_Test, Comparison_Equality)
{
    socket_address addr1("127.0.0.1", 80);
    socket_address addr2("127.0.0.1", 80);
    socket_address addr3("127.0.0.1", 8080);
    
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_FALSE(addr1 == addr3);
    EXPECT_TRUE(addr1 != addr3);
    EXPECT_FALSE(addr1 != addr2);
}

TEST_F(socket_address_Test, Comparison_LessThan)
{
    socket_address addr1("127.0.0.1", 80);
    socket_address addr2("127.0.0.1", 8080);
    socket_address addr3("192.168.1.1", 80);
    
    EXPECT_TRUE(addr1 < addr2);   // Same host, different port
    EXPECT_TRUE(addr1 < addr3);   // Different host
    EXPECT_FALSE(addr2 < addr1);
}

TEST_F(socket_address_Test, InvalidAddressThrows)
{
    EXPECT_THROW(socket_address("invalid.host.name", 80), std::runtime_error);
    EXPECT_THROW(socket_address("999.999.999.999", 80), std::runtime_error);
}

TEST_F(socket_address_Test, ToString)
{
    socket_address addr("127.0.0.1", 8080);
    std::string str = addr.to_string();
    
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("127.0.0.1"), std::string::npos);
    EXPECT_NE(str.find("8080"), std::string::npos);
}

TEST_F(socket_address_Test, PortRange)
{
    // Test valid port ranges
    EXPECT_NO_THROW(socket_address("127.0.0.1", 1));
    EXPECT_NO_THROW(socket_address("127.0.0.1", 65535));
    
    // Test port 0 (any port)
    EXPECT_NO_THROW(socket_address("127.0.0.1", 0));
}
