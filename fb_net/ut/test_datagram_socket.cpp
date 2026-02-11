#include <gtest/gtest.h>
#include <fb/udp_socket.h>
#include <chrono>
#include <cstring>
#include <limits>
#include <system_error>
#include <thread>

using namespace fb;

class udp_socketTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(udp_socketTest, Construction) {
    udp_socket socket(socket_address::Family::IPv4);
    EXPECT_EQ(socket.address().family(), socket_address::Family::IPv4);
}

TEST_F(udp_socketTest, BindAndAddress) {
    udp_socket socket(socket_address::Family::IPv4);
    EXPECT_NO_THROW(socket.bind(socket_address("127.0.0.1", 0)));
    EXPECT_GT(socket.address().port(), 0);
}

TEST_F(udp_socketTest, SendToReceiveFrom) {
    udp_socket sender(socket_address::Family::IPv4);
    udp_socket receiver(socket_address::Family::IPv4);

    receiver.bind(socket_address("127.0.0.1", 0));
    socket_address receiver_addr = receiver.address();

    std::string test_message = "UDP test message";

    // Send from sender to receiver
    int sent = sender.send_to(test_message, receiver_addr);
    EXPECT_GT(sent, 0);

    // Receive on receiver
    receiver.set_receive_timeout(std::chrono::seconds(2));
    std::string received;
    socket_address sender_addr;
    int received_bytes = receiver.receive_from(received, 1024, sender_addr);

    EXPECT_GT(received_bytes, 0);
    EXPECT_EQ(received, test_message);
    EXPECT_GT(sender_addr.port(), 0);
}

TEST_F(udp_socketTest, SendToReceiveFromBytes) {
    udp_socket sender(socket_address::Family::IPv4);
    udp_socket receiver(socket_address::Family::IPv4);

    receiver.bind(socket_address("127.0.0.1", 0));
    socket_address receiver_addr = receiver.address();

    const char* test_data = "Binary UDP data";
    const auto data_len = std::strlen(test_data);
    ASSERT_LE(data_len, static_cast<std::size_t>((std::numeric_limits<int>::max)()));

    // Send bytes
    int sent = sender.send_to(test_data, static_cast<int>(data_len), receiver_addr);
    EXPECT_EQ(sent, static_cast<int>(data_len));

    // Receive bytes
    receiver.set_receive_timeout(std::chrono::seconds(2));
    char buffer[1024];
    socket_address sender_addr;
    int received = receiver.receive_from(buffer, static_cast<int>(sizeof(buffer)), sender_addr);

    EXPECT_EQ(received, static_cast<int>(data_len));
    ASSERT_GE(received, 0);
    EXPECT_EQ(std::string(buffer, static_cast<std::size_t>(received)), test_data);
}

TEST_F(udp_socketTest, ConnectedMode) {
    udp_socket socket1(socket_address::Family::IPv4);
    udp_socket socket2(socket_address::Family::IPv4);

    socket1.bind(socket_address("127.0.0.1", 0));
    socket2.bind(socket_address("127.0.0.1", 0));

    // Connect socket1 to socket2
    socket1.connect(socket2.address());

    // Send using connected mode (no address needed)
    std::string test_message = "Connected UDP";
    int sent = socket1.send(test_message);
    EXPECT_GT(sent, 0);

    // Receive on socket2
    socket2.set_receive_timeout(std::chrono::seconds(2));
    std::string received;
    socket_address sender_addr;
    int received_bytes = socket2.receive_from(received, 1024, sender_addr);

    EXPECT_GT(received_bytes, 0);
    EXPECT_EQ(received, test_message);
}

TEST_F(udp_socketTest, DisconnectClearsDerivedAndBaseConnectionState) {
    udp_socket socket1(socket_address::Family::IPv4);
    udp_socket socket2(socket_address::Family::IPv4);

    socket1.bind(socket_address("127.0.0.1", 0));
    socket2.bind(socket_address("127.0.0.1", 0));

    socket1.connect(socket2.address());
    EXPECT_TRUE(socket1.is_connected());
    EXPECT_TRUE(static_cast<const socket_base&>(socket1).is_connected());

    socket1.disconnect();
    EXPECT_FALSE(socket1.is_connected());
    EXPECT_FALSE(static_cast<const socket_base&>(socket1).is_connected());
}

TEST_F(udp_socketTest, Broadcast) {
    udp_socket sender(socket_address::Family::IPv4);

    // Enable broadcast
    EXPECT_NO_THROW(sender.set_broadcast(true));

    // Note: Actual broadcast testing is platform/network dependent
    // Just verify the option can be set
}

TEST_F(udp_socketTest, ReceiveTimeout) {
    udp_socket socket(socket_address::Family::IPv4);
    socket.bind(socket_address("127.0.0.1", 0));
    socket.set_receive_timeout(std::chrono::milliseconds(100));

    std::string data;
    socket_address sender;

    // Should timeout since no data is being sent
    EXPECT_THROW(socket.receive_from(data, 1024, sender), std::system_error);
}

TEST_F(udp_socketTest, LoopbackMultiplePackets) {
    udp_socket socket(socket_address::Family::IPv4);
    socket.bind(socket_address("127.0.0.1", 0));
    socket_address addr = socket.address();

    const std::size_t num_packets = 5;
    std::vector<std::string> messages;

    // Send multiple packets to self
    for (std::size_t i = 0; i < num_packets; ++i) {
        std::string msg = "Packet " + std::to_string(i);
        messages.push_back(msg);
        socket.send_to(msg, addr);
    }

    // Receive all packets
    socket.set_receive_timeout(std::chrono::seconds(2));
    for (std::size_t i = 0; i < num_packets; ++i) {
        std::string received;
        socket_address sender;
        int bytes = socket.receive_from(received, 1024, sender);

        EXPECT_GT(bytes, 0);
        EXPECT_EQ(received, messages[i]);
    }
}

TEST_F(udp_socketTest, MoveSemantics) {
    udp_socket socket1(socket_address::Family::IPv4);
    socket1.bind(socket_address("127.0.0.1", 0));
    socket_address addr = socket1.address();

    // Move constructor
    udp_socket socket2(std::move(socket1));
    EXPECT_EQ(socket2.address().port(), addr.port());

    // Move assignment
    udp_socket socket3(socket_address::Family::IPv4);
    socket3 = std::move(socket2);
    EXPECT_EQ(socket3.address().port(), addr.port());
}

TEST_F(udp_socketTest, IPv6) {
    try {
        udp_socket socket(socket_address::Family::IPv6);
        socket.bind(socket_address("::1", 0));

        socket_address addr = socket.address();
        EXPECT_EQ(addr.family(), socket_address::Family::IPv6);
        EXPECT_GT(addr.port(), 0);

        // Test IPv6 loopback send/receive
        std::string test_msg = "IPv6 test";
        socket.send_to(test_msg, addr);

        socket.set_receive_timeout(std::chrono::seconds(2));
        std::string received;
        socket_address sender;
        socket.receive_from(received, 1024, sender);

        EXPECT_EQ(received, test_msg);
    } catch (const std::exception&) {
        // IPv6 might not be available on all systems
        GTEST_SKIP() << "IPv6 not available on this system";
    }
}
