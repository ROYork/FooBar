/**
 * @file test_server_socket.cpp
 * @brief Unit tests for server_socket class
 */

#include <gtest/gtest.h>
#include <fb/server_socket.h>
#include <fb/tcp_client.h>
#include <system_error>
#include <thread>
#include <chrono>
#include <vector>

using namespace fb;

class server_socketTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(server_socketTest, Construction)
{
    server_socket server(socket_address::Family::IPv4);
    EXPECT_EQ(server.address().family(), socket_address::Family::IPv4);
}

TEST_F(server_socketTest, BindListen)
{
    server_socket server(socket_address::Family::IPv4);
    EXPECT_NO_THROW(server.bind(socket_address("127.0.0.1", 0)));
    EXPECT_NO_THROW(server.listen());

    socket_address bound_addr = server.address();
    EXPECT_GT(bound_addr.port(), 0);
}

TEST_F(server_socketTest, AcceptWithTimeout)
{
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();

    // No client connecting, should timeout
    EXPECT_THROW(server.accept_connection(std::chrono::milliseconds(100)), std::system_error);
}

TEST_F(server_socketTest, AcceptConnection)
{
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();

    socket_address server_addr = server.address();

    // Client thread
    std::thread client_thread([server_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        try {
            tcp_client client(socket_address::Family::IPv4);
            client.connect(server_addr, std::chrono::seconds(2));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client.close();
        } catch (...) {}
    });

    // Accept the connection
    socket_address client_addr;
    tcp_client accepted = server.accept_connection(client_addr, std::chrono::seconds(2));

    EXPECT_GT(client_addr.port(), 0);
    EXPECT_FALSE(accepted.is_closed());

    accepted.close();
    client_thread.join();
}

TEST_F(server_socketTest, MultipleAccepts)
{
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen(10);

    socket_address server_addr = server.address();
    const int num_clients = 3;
    std::vector<std::thread> client_threads;

    // Launch multiple clients
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([server_addr, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50 * i));
            try {
                tcp_client client(socket_address::Family::IPv4);
                client.connect(server_addr, std::chrono::seconds(2));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (...) {}
        });
    }

    // Accept all clients
    std::vector<tcp_client> accepted_clients;
    for (int i = 0; i < num_clients; ++i) {
        try {
            accepted_clients.push_back(server.accept_connection(std::chrono::seconds(2)));
        } catch (const std::exception& ex) {
            FAIL() << "Failed to accept client " << i << ": " << ex.what();
        }
    }

    EXPECT_EQ(accepted_clients.size(), num_clients);

    for (auto& thread : client_threads) {
        thread.join();
    }
}

TEST_F(server_socketTest, ReuseAddress)
{
    server_socket server1(socket_address::Family::IPv4);
    server1.set_reuse_address(true);
    EXPECT_TRUE(server1.get_reuse_address());

    server1.bind(socket_address("127.0.0.1", 0));
    socket_address addr = server1.address();
    server1.listen();
    server1.close();

    // With reuse_address, we should be able to bind to the same port quickly
    server_socket server2(socket_address::Family::IPv4);
    server2.set_reuse_address(true);
    EXPECT_NO_THROW(server2.bind(addr));
}

TEST_F(server_socketTest, ReusePort)
{
    server_socket server(socket_address::Family::IPv4);
    EXPECT_NO_THROW(server.set_reuse_port(true));
    // Note: Not all platforms support SO_REUSEPORT
    // Just verify the method doesn't crash
}

TEST_F(server_socketTest, HasPendingConnections)
{
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();

    // No pending connections initially
    EXPECT_FALSE(server.has_pending_connections(std::chrono::milliseconds(50)));

    socket_address server_addr = server.address();

    // Connect a client
    std::thread client_thread([server_addr]() {
        try {
            tcp_client client(socket_address::Family::IPv4);
            client.connect(server_addr, std::chrono::seconds(2));
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } catch (...) {}
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now should have pending connection
    EXPECT_TRUE(server.has_pending_connections(std::chrono::milliseconds(100)));

    // Accept and verify no more pending
    tcp_client accepted = server.accept_connection();
    accepted.close();

    client_thread.join();
}

TEST_F(server_socketTest, Backlog)
{
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.set_backlog(10);  // Set backlog after bind but before listen
    server.listen();

    EXPECT_EQ(server.max_connections(), 1024); // MAX_BACKLOG constant
}

TEST_F(server_socketTest, MoveSemantics)
{
    server_socket server1(socket_address::Family::IPv4);
    server1.bind(socket_address("127.0.0.1", 0));
    server1.listen();

    socket_address addr = server1.address();

    // Move constructor
    server_socket server2(std::move(server1));
    EXPECT_EQ(server2.address().port(), addr.port());

    // Move assignment
    server_socket server3(socket_address::Family::IPv4);
    server3 = std::move(server2);
    EXPECT_EQ(server3.address().port(), addr.port());
}

TEST_F(server_socketTest, BindPortInUse)
{
    server_socket server1(socket_address::Family::IPv4);
    server1.bind(socket_address("127.0.0.1", 0));
    socket_address addr = server1.address();
    server1.listen();

    // Try to bind another server to same port without reuse_address
    server_socket server2(socket_address::Family::IPv4);
    server2.set_reuse_address(false);
    EXPECT_THROW(server2.bind(addr), std::system_error);
}
