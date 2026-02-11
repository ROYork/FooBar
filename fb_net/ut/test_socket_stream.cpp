#include <gtest/gtest.h>
#include <fb/socket_stream.h>
#include <fb/server_socket.h>
#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

using namespace fb;

class socket_streamTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(socket_streamTest, BasicConstruction) {
    tcp_client socket(socket_address::Family::IPv4);
    socket_stream stream(socket);

    EXPECT_EQ(&stream.socket(), &socket);
    EXPECT_NE(stream.rdbuf(), nullptr);
}

TEST_F(socket_streamTest, ConnectAndClose) {
    // Create server
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    // Connect with socket_stream
    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));
    EXPECT_TRUE(stream.is_connected());
    EXPECT_FALSE(stream.bad());

    stream.close();
    server_thread.join();
}

TEST_F(socket_streamTest, OutputOperator) {
    // Create echo server
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));

            // Echo back what we receive
            std::string data;
            client.receive(data, 1024);
            client.send(data);
        } catch (...) {}
    });

    // Connect and send using stream operators
    socket_stream stream(server_addr, std::chrono::seconds(2));

    stream << "Hello, stream!" << std::flush;
    EXPECT_TRUE(stream.good());

    // Read back
    std::string response;
    stream >> response;
    EXPECT_EQ(response, "Hello,");  // operator>> reads until whitespace

    server_thread.join();
}

TEST_F(socket_streamTest, GetLine) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));
            client.send("Line 1\nLine 2\nLine 3\n");
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));

    std::string line1, line2, line3;
    std::getline(stream, line1);
    std::getline(stream, line2);
    std::getline(stream, line3);

    EXPECT_EQ(line1, "Line 1");
    EXPECT_EQ(line2, "Line 2");
    EXPECT_EQ(line3, "Line 3");

    server_thread.join();
}

TEST_F(socket_streamTest, FormattedIO) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));

            // Receive and echo
            std::string data;
            client.receive(data, 1024);
            client.send(data);
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));

    // Send formatted data
    int num = 42;
    double pi = 3.14159;
    std::string text = "test";

    stream << num << " " << pi << " " << text << std::endl;

    // Read back
    int num_back;
    double pi_back;
    std::string text_back;

    stream >> num_back >> pi_back >> text_back;

    EXPECT_EQ(num_back, num);
    EXPECT_NEAR(pi_back, pi, 0.00001);
    EXPECT_EQ(text_back, text);

    server_thread.join();
}

TEST_F(socket_streamTest, BufferSize) {
    tcp_client socket(socket_address::Family::IPv4);
    socket_stream stream(socket, 4096);

    EXPECT_EQ(stream.buffer_size(), 4096);

    stream.set_buffer_size(8192);
    EXPECT_EQ(stream.buffer_size(), 8192);
}

TEST_F(socket_streamTest, SocketOptions) {
    tcp_client socket(socket_address::Family::IPv4);
    socket_stream stream(socket);

    EXPECT_NO_THROW(stream.set_no_delay(true));
    EXPECT_NO_THROW(stream.set_keep_alive(true));
    EXPECT_NO_THROW(stream.set_send_timeout(std::chrono::seconds(5)));
    EXPECT_NO_THROW(stream.set_receive_timeout(std::chrono::seconds(5)));
}

TEST_F(socket_streamTest, PeerAddress) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));

    socket_address peer = stream.peer_address();
    EXPECT_EQ(peer.host(), "127.0.0.1");
    EXPECT_EQ(peer.port(), server_addr.port());

    server_thread.join();
}

TEST_F(socket_streamTest, ReadWrite) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));

            char buffer[100];
            int received = client.receive_bytes(buffer, sizeof(buffer));
            client.send_bytes(buffer, received);
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));

    // Write data
    const char* data = "Test data 123";
    const auto length = std::strlen(data);
    stream.write(data, static_cast<std::streamsize>(length));
    stream.flush();

    // Read back
    char buffer[100] = {0};
    stream.read(buffer, static_cast<std::streamsize>(length));

    EXPECT_STREQ(buffer, data);

    server_thread.join();
}

TEST_F(socket_streamTest, StreamState) {
    tcp_client socket(socket_address::Family::IPv4);
    socket_stream stream(socket);

    // Initially should be good (not connected, but stream is valid)
    EXPECT_TRUE(stream.good());
    EXPECT_FALSE(stream.eof());
    EXPECT_FALSE(stream.fail());
    EXPECT_FALSE(stream.bad());
}

TEST_F(socket_streamTest, MultipleWrites) {
    server_socket server(socket_address::Family::IPv4);
    server.bind(socket_address("127.0.0.1", 0));
    server.listen();
    socket_address server_addr = server.address();

    std::thread server_thread([&server]() {
        try {
            tcp_client client = server.accept_connection(std::chrono::seconds(2));

            // Simple echo of all data
            char buffer[1024];
            int received = client.receive_bytes(buffer, sizeof(buffer));
            if (received > 0) {
                client.send_bytes(buffer, received);
            }
        } catch (...) {}
    });

    socket_stream stream(server_addr, std::chrono::seconds(2));

    // Write and immediately read to avoid deadlock
    stream << "Test line" << std::endl;
    stream.flush();

    std::string result;
    std::getline(stream, result);

    // Just verify we got some data back
    EXPECT_FALSE(result.empty());

    server_thread.join();
}
