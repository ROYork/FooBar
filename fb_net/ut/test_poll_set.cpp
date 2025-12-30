#include <gtest/gtest.h>
#include <fb/poll_set.h>
#include <fb/tcp_client.h>
#include <fb/server_socket.h>
#include <fb/udp_socket.h>
#include <thread>
#include <chrono>

using namespace fb;

class PollSetTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PollSetTest, BasicConstruction) {
    poll_set poller;
    EXPECT_TRUE(poller.empty());
    EXPECT_EQ(poller.count(), 0);
}

TEST_F(PollSetTest, AddRemoveSockets) {
    poll_set poller;
    tcp_client socket1(socket_address::Family::IPv4);
    tcp_client socket2(socket_address::Family::IPv4);

    // Add sockets
    poller.add(socket1, poll_set::POLL_READ);
    EXPECT_FALSE(poller.empty());
    EXPECT_EQ(poller.count(), 1);
    EXPECT_TRUE(poller.has(socket1));

    poller.add(socket2, poll_set::POLL_READ | poll_set::POLL_WRITE);
    EXPECT_EQ(poller.count(), 2);
    EXPECT_TRUE(poller.has(socket2));

    // Remove sockets
    poller.remove(socket1);
    EXPECT_EQ(poller.count(), 1);
    EXPECT_FALSE(poller.has(socket1));
    EXPECT_TRUE(poller.has(socket2));

    poller.remove(socket2);
    EXPECT_TRUE(poller.empty());
    EXPECT_EQ(poller.count(), 0);
}

TEST_F(PollSetTest, UpdateSocketMode) {
    poll_set poller;
    tcp_client socket(socket_address::Family::IPv4);

    poller.add(socket, poll_set::POLL_READ);
    EXPECT_EQ(poller.get_mode(socket), poll_set::POLL_READ);

    poller.update(socket, poll_set::POLL_WRITE);
    EXPECT_EQ(poller.get_mode(socket), poll_set::POLL_WRITE);

    poller.update(socket, poll_set::POLL_READ | poll_set::POLL_WRITE);
    EXPECT_EQ(poller.get_mode(socket), poll_set::POLL_READ | poll_set::POLL_WRITE);
}

TEST_F(PollSetTest, Clear) {
    poll_set poller;
    tcp_client socket1(socket_address::Family::IPv4);
    tcp_client socket2(socket_address::Family::IPv4);

    poller.add(socket1, poll_set::POLL_READ);
    poller.add(socket2, poll_set::POLL_READ);
    EXPECT_EQ(poller.count(), 2);

    poller.clear();
    EXPECT_TRUE(poller.empty());
    EXPECT_EQ(poller.count(), 0);
}

TEST_F(PollSetTest, PollTimeout) {
    poll_set poller;
    tcp_client socket(socket_address::Family::IPv4);

    poller.add(socket, poll_set::POLL_READ);

    // Poll with timeout, should return 0 (no events)
    int result = poller.poll(std::chrono::milliseconds(100));
    EXPECT_EQ(result, 0);
}

TEST_F(PollSetTest, PollReadEvent) {
    // Create server socket
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    // Connect client
    tcp_client client(socket_address::Family::IPv4);
    std::thread connect_thread([&client, server_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        client.connect(server_addr, std::chrono::seconds(2));
    });

    // Use poll_set to detect incoming connection
    poll_set poller;
    poller.add(server, poll_set::POLL_READ);

    int result = poller.poll(std::chrono::seconds(2));
    EXPECT_GT(result, 0);

    const auto& events = poller.events();
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].mode & poll_set::POLL_READ, poll_set::POLL_READ);

    // Accept the connection
    tcp_client accepted = server.accept_connection();
    accepted.close();
    connect_thread.join();
}

TEST_F(PollSetTest, PollMultipleSockets) {
    // Create two server sockets
    server_socket server1(socket_address::Family::IPv4);
    server1.bind(socket_address("127.0.0.1", 0));
    server1.listen();
    socket_address addr1 = server1.address();

    server_socket server2(socket_address::Family::IPv4);
    server2.bind(socket_address("127.0.0.1", 0));
    server2.listen();
    socket_address addr2 = server2.address();
    EXPECT_NE(addr1, addr2);

    // Add both to poll_set
    poll_set poller;
    poller.add(server1, poll_set::POLL_READ);
    poller.add(server2, poll_set::POLL_READ);

    // Connect to first server
    std::thread client_thread([addr1]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        tcp_client client(socket_address::Family::IPv4);
        client.connect(addr1, std::chrono::seconds(2));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // Poll should detect event on server1
    int result = poller.poll(std::chrono::seconds(2));
    EXPECT_GT(result, 0);

    const auto& events = poller.events();
    EXPECT_GE(events.size(), 1);

    bool found_server1 = false;
    for (const auto& event : events) {
        if (event.socket_ptr == &server1) {
            found_server1 = true;
            EXPECT_EQ(event.mode & poll_set::POLL_READ, poll_set::POLL_READ);
        }
    }
    EXPECT_TRUE(found_server1);

    client_thread.join();
}

TEST_F(PollSetTest, UDPPolling) {
    udp_socket receiver(socket_address::Family::IPv4);
    receiver.bind(socket_address("127.0.0.1", 0));
    socket_address receiver_addr = receiver.address();

    poll_set poller;
    poller.add(receiver, poll_set::POLL_READ);

    // Send UDP packet in background
    std::thread sender_thread([receiver_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        udp_socket sender(socket_address::Family::IPv4);
        sender.send_to("test", receiver_addr);
    });

    // Poll should detect incoming UDP packet
    int result = poller.poll(std::chrono::seconds(2));
    EXPECT_GT(result, 0);

    const auto& events = poller.events();
    EXPECT_GE(events.size(), 1);
    EXPECT_EQ(events[0].socket_ptr, &receiver);
    EXPECT_EQ(events[0].mode & poll_set::POLL_READ, poll_set::POLL_READ);

    sender_thread.join();
}

TEST_F(PollSetTest, PollWithEventVector) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    poll_set poller;
    poller.add(server, poll_set::POLL_READ);

    // Connect client
    std::thread client_thread([server_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        tcp_client client(socket_address::Family::IPv4);
        client.connect(server_addr, std::chrono::seconds(2));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // Poll with explicit event vector
    std::vector<SocketEvent> events;
    int result = poller.poll(events, std::chrono::seconds(2));

    EXPECT_GT(result, 0);
    EXPECT_GE(events.size(), 1);
    EXPECT_EQ(events[0].socket_ptr, &server);

    client_thread.join();
}

TEST_F(PollSetTest, MoveSemantics) {
    poll_set poller1;
    tcp_client socket(socket_address::Family::IPv4);
    poller1.add(socket, poll_set::POLL_READ);

    EXPECT_EQ(poller1.count(), 1);

    // Move constructor
    poll_set poller2(std::move(poller1));
    EXPECT_EQ(poller2.count(), 1);
    EXPECT_TRUE(poller2.has(socket));

    // Move assignment
    poll_set poller3;
    poller3 = std::move(poller2);
    EXPECT_EQ(poller3.count(), 1);
    EXPECT_TRUE(poller3.has(socket));
}

TEST_F(PollSetTest, PlatformInfo) {
    // Just verify these methods exist and don't crash
    const char* method = poll_set::polling_method();
    EXPECT_NE(method, nullptr);

    bool efficient = poll_set::efficient_polling();
    // On Linux (epoll) or BSD (kqueue), should be true
    // On Windows (select), might be false
    (void)efficient; // Suppress unused warning
}

TEST_F(PollSetTest, ClearEvents) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    poll_set poller;
    poller.add(server, poll_set::POLL_READ);

    // Connect and poll
    std::thread client_thread([server_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        tcp_client client(socket_address::Family::IPv4);
        client.connect(server_addr, std::chrono::seconds(2));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    int result = poller.poll(std::chrono::seconds(2));
    EXPECT_GT(result, 0);
    EXPECT_GT(poller.events().size(), 0);

    // Clear events
    poller.clear_events();
    EXPECT_EQ(poller.events().size(), 0);

    client_thread.join();
}
