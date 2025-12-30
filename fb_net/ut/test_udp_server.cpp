#include <gtest/gtest.h>
#include <fb/udp_server.h>
#include <fb/udp_client.h>
#include <atomic>
#include <chrono>
#include <limits>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace fb;

// Simple echo handler for testing
class EchoHandler : public udp_handler
{
public:
    udp_socket& reply_socket;

    explicit EchoHandler(udp_socket& socket) : reply_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // Echo back to sender
        if (length > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            return;
        }
        reply_socket.send_to(buffer, static_cast<int>(length), sender_address);
    }
};

// Counter handler for testing packet counts
class CounterHandler : public udp_handler
{
public:
    static std::atomic<int> packet_count;

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        packet_count++;
    }
};

std::atomic<int> CounterHandler::packet_count{0};

class UDPServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        CounterHandler::packet_count = 0;
    }

    void TearDown() override {}
};

TEST_F(UDPServerTest, BasicConstruction) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);
    EXPECT_FALSE(server.is_running());
}

TEST_F(UDPServerTest, StartAndStop) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);

    server.start();
    EXPECT_TRUE(server.is_running());

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST_F(UDPServerTest, EchoFunctionality) {
    // Create echo server
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    udp_server server(std::move(server_sock), std::make_shared<CounterHandler>());

    // Handler factory that creates echo handlers with reference to server socket
    // Must capture server by reference and access socket after it's moved
    auto factory = [&server](const udp_server::PacketData& packet) {
        return std::make_unique<EchoHandler>(const_cast<udp_socket&>(server.server_socket()));
    };

    server.set_handler_factory(factory);
    server.start();

    // Send UDP packet
    udp_client client;
    std::string test_message = "Hello, UDP server!";
    client.send_to(test_message, server_addr);

    // Receive echo - udp_client doesn't have set_receive_timeout, use direct socket
    udp_socket& client_socket = client.socket();
    client_socket.set_receive_timeout(std::chrono::seconds(2));

    std::string response;
    socket_address sender;

    try {
        client.receive_from(response, 1024, sender);
        EXPECT_EQ(response, test_message);
    } catch (const std::system_error& ex) {
        if (ex.code() == std::make_error_code(std::errc::timed_out)) {
            FAIL() << "Did not receive echo response";
        } else {
            throw;
        }
    }

    server.stop();
}

TEST_F(UDPServerTest, MultiplePackets) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);
    server.start();

    // Send multiple packets
    const int num_packets = 10;
    udp_client client;

    for (int i = 0; i < num_packets; ++i) {
        std::string message = "Packet " + std::to_string(i);
        client.send_to(message, server_addr);
    }

    // Give server time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(CounterHandler::packet_count.load(), num_packets);

    server.stop();
}

TEST_F(UDPServerTest, ConcurrentClients) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);
    server.start();

    const int num_clients = 5;
    const int packets_per_client = 3;
    std::vector<std::thread> client_threads;

    // Launch multiple clients
    for (int i = 0; i < num_clients; ++i)
    {
        client_threads.emplace_back([server_addr, i]() {
            udp_client client;
            for (int j = 0; j < packets_per_client; ++j)
            {
                std::string message = "Client " + std::to_string(i) + " Packet " + std::to_string(j);
                client.send_to(message, server_addr);
            }
        });
    }

    for (auto& thread : client_threads) {
        thread.join();
    }

    // Give server time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ(CounterHandler::packet_count.load(), num_clients * packets_per_client);

    server.stop();
}

TEST_F(UDPServerTest, MaxThreads) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));

    auto handler = std::make_shared<CounterHandler>();

    // Limit to 3 worker threads
    udp_server server(std::move(server_sock), handler, 3);
    server.start();

    // Max threads set during construction, test passes by not crashing

    server.stop();
}

TEST_F(UDPServerTest, TotalPackets) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);
    server.start();

    std::uint64_t initial_total = server.total_packets();

    // Send packets
    udp_client client;
    for (int i = 0; i < 5; ++i) {
        client.send_to("test", server_addr);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GE(server.total_packets(), initial_total + 5);

    server.stop();
}

TEST_F(UDPServerTest, MoveSemantics) {
    udp_socket server_sock1(socket_address::Family::IPv4);
    server_sock1.bind(socket_address("127.0.0.1", 0));

    auto handler = std::make_shared<CounterHandler>();

    // Move constructor (before starting)
    udp_server server1(std::move(server_sock1), handler);
    udp_server server2(std::move(server1));

    // Start the moved server
    server2.start();
    EXPECT_TRUE(server2.is_running());
    server2.stop();

    // Move assignment (stopped server)
    udp_socket server_sock2(socket_address::Family::IPv4);
    server_sock2.bind(socket_address("127.0.0.1", 0));

    udp_server server3(std::move(server_sock2), handler);
    udp_server server4;
    server4 = std::move(server3);

    EXPECT_FALSE(server4.is_running());
}

TEST_F(UDPServerTest, HandlerFactory) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    udp_server server(std::move(server_sock), std::make_shared<CounterHandler>());

    // Factory creates new handler for each packet
    auto factory = [&server](const udp_server::PacketData& packet) {
        CounterHandler::packet_count++;
        return std::make_unique<EchoHandler>(const_cast<udp_socket&>(server.server_socket()));
    };

    server.set_handler_factory(factory);
    server.start();

    // Send packets
    udp_client client;
    for (int i = 0; i < 3; ++i) {
        client.send_to("test", server_addr);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Factory should have been called 3 times
    EXPECT_GE(CounterHandler::packet_count.load(), 3);

    server.stop();
}

TEST_F(UDPServerTest, GracefulShutdown) {
    udp_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();

    auto handler = std::make_shared<CounterHandler>();

    udp_server server(std::move(server_sock), handler);
    server.start();

    // Send some packets
    udp_client client;
    for (int i = 0; i < 10; ++i) {
        client.send_to("test", server_addr);
    }

    // Stop with timeout
    auto start = std::chrono::steady_clock::now();
    server.stop(std::chrono::seconds(5));
    auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(server.is_running());
    // Should have stopped in reasonable time
    EXPECT_LT(duration, std::chrono::seconds(6));
}
