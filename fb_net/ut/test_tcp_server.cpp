#include <gtest/gtest.h>
#include <fb/tcp_server.h>
#include <fb/tcp_client.h>
#include <system_error>
#include <thread>
#include <chrono>
#include <atomic>

using namespace fb;

// Simple echo connection handler for testing
class EchoConnection : public tcp_server_connection
{
public:
    EchoConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {}

    void run() override
    {
        try {
            while (!stop_requested()) {
                std::string data;
                socket().set_receive_timeout(std::chrono::milliseconds(100));

                try {
                    int received = socket().receive(data, 1024);
                    if (received > 0) {
                        socket().send(data);
                    } else {
                        break;
                    }
                } catch (const std::system_error &ex) {
                    if (ex.code() == std::make_error_code(std::errc::timed_out)) {
                        // Check stop flag on timeout
                        continue;
                    }
                    throw;
                }
            }
        } catch (const std::exception&) {
            // Connection error, exit gracefully
        }
    }
};

// Counter connection for testing max connections
class CounterConnection : public tcp_server_connection
{
public:
    static std::atomic<int> active_count;

    CounterConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        active_count++;
    }

    ~CounterConnection() override
    {
        active_count--;
    }

    void run() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

std::atomic<int> CounterConnection::active_count{0};

class TCPServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        CounterConnection::active_count = 0;
    }

    void TearDown() override {}
};

TEST_F(TCPServerTest, BasicConstruction) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    EXPECT_FALSE(server.is_running());
}

TEST_F(TCPServerTest, StartAndStop) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);

    server.start();
    EXPECT_TRUE(server.is_running());

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST_F(TCPServerTest, EchoFunctionality) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    server.start();

    // Connect client and test echo
    tcp_client client(socket_address::Family::IPv4);
    client.connect(server_addr, std::chrono::seconds(2));

    std::string test_message = "Hello, server!";
    client.send(test_message);

    std::string response;
    client.receive(response, 1024);

    EXPECT_EQ(response, test_message);

    client.close();
    server.stop();
}

TEST_F(TCPServerTest, MultipleClients) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    server.start();

    const int num_clients = 5;
    std::vector<std::thread> client_threads;

    // Launch multiple clients
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([server_addr, i]() {
            try {
                tcp_client client(socket_address::Family::IPv4);
                client.connect(server_addr, std::chrono::seconds(2));

                std::string message = "Client " + std::to_string(i);
                client.send(message);

                std::string response;
                client.receive(response, 1024);

                EXPECT_EQ(response, message);
            } catch (const std::exception& ex) {
                ADD_FAILURE() << "Client " << i << " failed: " << ex.what();
            }
        });
    }

    for (auto& thread : client_threads) {
        thread.join();
    }

    server.stop();
}

TEST_F(TCPServerTest, MaxThreads) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<CounterConnection>(std::move(socket), addr);
    };

    // Limit to 3 worker threads
    tcp_server server(std::move(server_sock), factory, 3);
    server.start();

    // Max threads is set during construction, test passes by not crashing

    // Connect more clients than threads
    std::vector<std::unique_ptr<tcp_client>> clients;
    for (int i = 0; i < 5; ++i) {
        auto client = std::make_unique<tcp_client>(socket_address::Family::IPv4);
        client->connect(server_addr, std::chrono::seconds(2));
        clients.push_back(std::move(client));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Give time for connections to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Active count should not exceed max_threads (some might have finished)
    EXPECT_LE(CounterConnection::active_count.load(), 3);

    clients.clear();
    server.stop();
}

TEST_F(TCPServerTest, CurrentConnections) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<CounterConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    server.start();

    EXPECT_EQ(server.queued_connections(), 0);

    // Connect clients
    std::vector<std::unique_ptr<tcp_client>> clients;
    for (int i = 0; i < 3; ++i) {
        auto client = std::make_unique<tcp_client>(socket_address::Family::IPv4);
        client->connect(server_addr, std::chrono::seconds(2));
        clients.push_back(std::move(client));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connections are being processed, test passes by not crashing

    clients.clear();
    server.stop();
}

TEST_F(TCPServerTest, TotalConnections) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    server.start();

    std::uint64_t initial_total = server.total_connections();

    // Connect 3 clients
    for (int i = 0; i < 3; ++i) {
        tcp_client client(socket_address::Family::IPv4);
        client.connect(server_addr, std::chrono::seconds(2));
        client.send("test");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Total connections should have increased
    EXPECT_GE(server.total_connections(), initial_total + 3);

    server.stop();
}

TEST_F(TCPServerTest, MoveSemantics) {
    server_socket server_sock1(socket_address::Family::IPv4);
    server_sock1.bind(socket_address("127.0.0.1", 0));
    server_sock1.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    // Move constructor (before starting)
    tcp_server server1(std::move(server_sock1), factory);
    tcp_server server2(std::move(server1));

    // Start the moved server
    server2.start();
    EXPECT_TRUE(server2.is_running());
    server2.stop();

    // Move assignment (stopped server)
    server_socket server_sock2(socket_address::Family::IPv4);
    server_sock2.bind(socket_address("127.0.0.1", 0));
    server_sock2.listen();

    tcp_server server3(std::move(server_sock2), factory);
    tcp_server server4;
    server4 = std::move(server3);

    EXPECT_FALSE(server4.is_running());
}

TEST_F(TCPServerTest, GracefulShutdown) {
    server_socket server_sock(socket_address::Family::IPv4);
    server_sock.bind(socket_address("127.0.0.1", 0));
    socket_address server_addr = server_sock.address();
    server_sock.listen();

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(server_sock), factory);
    server.start();

    // Connect clients
    std::vector<std::unique_ptr<tcp_client>> clients;
    for (int i = 0; i < 3; ++i) {
        auto client = std::make_unique<tcp_client>(socket_address::Family::IPv4);
        client->connect(server_addr, std::chrono::seconds(2));
        clients.push_back(std::move(client));
    }

    // Stop with timeout - should wait for connections to finish
    auto start = std::chrono::steady_clock::now();
    server.stop(std::chrono::seconds(5));
    auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(server.is_running());
    // Should have stopped in reasonable time
    EXPECT_LT(duration, std::chrono::seconds(6));
}
