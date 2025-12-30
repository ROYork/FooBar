/**
 * @file test_stream_socket.cpp
 * @brief Unit tests for tcp_client class
 */

#include <gtest/gtest.h>
#include <fb/tcp_client.h>
#include <fb/server_socket.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <system_error>
#include <thread>

using namespace fb;

class tcp_clientTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    // Helper to create a simple echo server for testing
    std::unique_ptr<std::thread> create_echo_server(int port, std::atomic<bool>* running = nullptr)
    {
        return std::make_unique<std::thread>([port, running]() {
            try {
                server_socket server(socket_address::Family::IPv4);
                server.bind(socket_address("127.0.0.1", static_cast<std::uint16_t>(port)));
                server.listen();

                // Set timeouts so we can check the running flag periodically
                server.set_receive_timeout(std::chrono::milliseconds(500));

                while (!running || running->load())
                {
                    try
                    {
                        // Accept with timeout so we can check running flag
                        tcp_client client =
                            server.accept_connection(std::chrono::milliseconds(500));

                        std::string data;
                        int received = client.receive(data, 1024);
                        if (received > 0)
                        {
                            client.send(data); // Echo back
                        }
                        client.close();

                        if (running && !running->load())
                        {
                            break;
                        }
                    }
                    catch (const std::system_error &ex)
                    {
                        if (ex.code() == std::make_error_code(std::errc::timed_out))
                        {
                            if (running && !running->load())
                            {
                                break;
                            }
                            continue;
                        }
                        break;
                    }
                    catch (const std::exception &)
                    {
                        // Other errors - stop the server
                        break;
                    }
                }
            }
            catch (const std::exception&)
            {
                // Server failed to start
            }
        });
    }
};

TEST_F(tcp_clientTest, Construction)
{
    tcp_client socket(socket_address::Family::IPv4);
    EXPECT_EQ(socket.address().family(), socket_address::Family::IPv4);
}

TEST_F(tcp_clientTest, SocketOptions)
{
    tcp_client socket(socket_address::Family::IPv4);
    
    EXPECT_NO_THROW(socket.set_reuse_address(true));
    EXPECT_NO_THROW(socket.set_keep_alive(true));
    EXPECT_NO_THROW(socket.set_no_delay(true));
    EXPECT_NO_THROW(socket.set_blocking(false));
    EXPECT_NO_THROW(socket.set_blocking(true));
}

TEST_F(tcp_clientTest, Timeouts)
{
    tcp_client socket(socket_address::Family::IPv4);
    
    EXPECT_NO_THROW(socket.set_send_timeout(std::chrono::seconds(10)));
    EXPECT_NO_THROW(socket.set_receive_timeout(std::chrono::seconds(15)));
}

TEST_F(tcp_clientTest, ConnectionRefused)
{
    tcp_client socket(socket_address::Family::IPv4);

    try
    {
        socket.connect(socket_address("127.0.0.1", 65432),
                       std::chrono::milliseconds(500));
        FAIL() << "Expected std::system_error for refused connection";
    }
    catch (const std::system_error &ex)
    {
#ifdef _WIN32
        // On Windows, connection refusal may manifest as different error codes
        // depending on timing and Windows version. Accept any connection-related error.
        bool is_connection_error =
            ex.code() == std::make_error_code(std::errc::connection_refused) ||
            ex.code() == std::make_error_code(std::errc::connection_reset) ||
            ex.code() == std::make_error_code(std::errc::connection_aborted) ||
            ex.code() == std::make_error_code(std::errc::timed_out) ||
            ex.code().category() == std::system_category();  // Windows error codes
        EXPECT_TRUE(is_connection_error)
            << "Expected connection error, got: " << ex.code() << " - " << ex.what();
#else
        // Unix/Linux: Should specifically be connection_refused
        EXPECT_EQ(ex.code(), std::make_error_code(std::errc::connection_refused))
            << "Unexpected error code: " << ex.code();
#endif
    }
}

TEST_F(tcp_clientTest, EchoTest)
{
    const int test_port = 15001;
    std::atomic<bool> server_running{true};
    
    // Start echo server
    auto server_thread = create_echo_server(test_port, &server_running);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        tcp_client client(socket_address::Family::IPv4);
        client.connect(socket_address("127.0.0.1", test_port), std::chrono::seconds(2));
        
        std::string test_message = "Hello, tcp_client!";
        client.send(test_message);
        
        std::string response;
        client.receive(response, 1024);
        
        EXPECT_EQ(response, test_message);
        
        client.close();
        
    } catch (const std::exception& ex) {
        FAIL() << "Echo test failed: " << ex.what();
    }
    
    server_running = false;
    server_thread->join();
}

TEST_F(tcp_clientTest, SendReceiveBytes)
{
    const int test_port = 15002;
    std::atomic<bool> server_running{true};
    
    auto server_thread = create_echo_server(test_port, &server_running);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        tcp_client client(socket_address::Family::IPv4);
        client.connect(socket_address("127.0.0.1", test_port), std::chrono::seconds(2));
        
        const char* test_data = "Binary data test";
        const auto data_len = std::strlen(test_data);
        ASSERT_LE(data_len, static_cast<std::size_t>((std::numeric_limits<int>::max)()));
        
        int sent = client.send_bytes(test_data, static_cast<int>(data_len));
        EXPECT_EQ(sent, static_cast<int>(data_len));
        
        char buffer[1024];
        int received = client.receive_bytes(buffer, sizeof(buffer));
        EXPECT_EQ(received, static_cast<int>(data_len));
        ASSERT_GE(received, 0);
        EXPECT_EQ(std::string(buffer, static_cast<std::size_t>(received)), test_data);
        
        client.close();
        
    } catch (const std::exception& ex) {
        FAIL() << "Bytes test failed: " << ex.what();
    }
    
    server_running = false;
    server_thread->join();
}

TEST_F(tcp_clientTest, MoveSemantics)
{
    tcp_client socket1(socket_address::Family::IPv4);
    socket1.set_reuse_address(true);
    
    // Move constructor
    tcp_client socket2 = std::move(socket1);
    EXPECT_EQ(socket2.address().family(), socket_address::Family::IPv4);
    
    // Move assignment
    tcp_client socket3(socket_address::Family::IPv4);
    socket3 = std::move(socket2);
    EXPECT_EQ(socket3.address().family(), socket_address::Family::IPv4);
}

TEST_F(tcp_clientTest, PeerAddress)
{
    const int test_port = 15003;
    std::atomic<bool> server_running{true};
    
    auto server_thread = create_echo_server(test_port, &server_running);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        tcp_client client(socket_address::Family::IPv4);
        client.connect(socket_address("127.0.0.1", test_port));
        
        socket_address peer = client.peer_address();
        EXPECT_EQ(peer.host(), "127.0.0.1");
        EXPECT_EQ(peer.port(), test_port);
        
        client.close();
        
    } catch (const std::exception& ex) {
        FAIL() << "Peer address test failed: " << ex.what();
    }
    
    server_running = false;
    server_thread->join();
}

TEST_F(tcp_clientTest, ReceiveTimeout)
{
    const std::uint16_t test_port = 15004;
    std::atomic<bool> server_running{true};

    auto server_thread = std::make_unique<std::thread>([&]() {
        server_socket server(socket_address::Family::IPv4);
        server.bind(socket_address("127.0.0.1", test_port));
        server.listen();
        tcp_client client = server.accept_connection();
        while (server_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tcp_client socket(socket_address::Family::IPv4);
    socket.connect(socket_address("127.0.0.1", test_port));
    socket.set_receive_timeout(std::chrono::milliseconds(100));

    std::string data;
    EXPECT_THROW(socket.receive(data, 1024), std::system_error);

    server_running = false;
    server_thread->join();
}
