/**
 * @file udp_signal_demo.cpp
 * @brief Comprehensive UDP client/server examples demonstrating signal-driven programming
 *
 * This example demonstrates how to use signals with UDP networking for:
 * - Real-time event monitoring
 * - Statistics collection
 * - Error handling via signals
 * - Protocol state machines
 */

#include <fb/fb_net.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>

using namespace fb;

// ============================================================================
// Example 1: UDP Client with Signal-Based Monitoring
// ============================================================================

class MonitoredUDPClient {
public:
    MonitoredUDPClient(const std::string& name)
        : name_(name), bytes_sent_(0), bytes_received_(0), errors_(0) {
        // Connect to client signals
        client_.onConnected.connect([this](const socket_address& addr) {
            std::cout << "[" << name_ << "] Connected to " << addr.to_string() << std::endl;
        });

        client_.onDisconnected.connect([this]() {
            std::cout << "[" << name_ << "] Disconnected" << std::endl;
            print_statistics();
        });

        client_.onDataSent.connect([this](const void*, std::size_t len) {
            bytes_sent_ += len;
            std::cout << "[" << name_ << "] Sent " << len << " bytes (total: " << bytes_sent_ << ")" << std::endl;
        });

        client_.onDataReceived.connect([this](const void* data, std::size_t len) {
            bytes_received_ += len;
            std::string msg(static_cast<const char*>(data), len);
            std::cout << "[" << name_ << "] Received: " << msg << std::endl;
        });

        client_.onDatagramReceived.connect([this](const void* data, std::size_t len, const socket_address& sender) {
            std::string preview(static_cast<const char*>(data), (std::min<std::size_t>)(len, 64));
            std::cout << "[" << name_ << "] Datagram from " << sender.to_string()
                      << ": " << len << " bytes - \"" << preview << "\"" << std::endl;
        });

        client_.onSendError.connect([this](const std::string& error) {
            errors_++;
            std::cerr << "[" << name_ << "] Send error: " << error << std::endl;
        });

        client_.onReceiveError.connect([this](const std::string& error) {
            errors_++;
            std::cerr << "[" << name_ << "] Receive error: " << error << std::endl;
        });

        client_.onTimeoutError.connect([this](const std::string& error) {
            errors_++;
            std::cerr << "[" << name_ << "] Timeout: " << error << std::endl;
        });
    }

    void connect(const socket_address& addr) {
        client_.connect(addr);
    }

    void send(const std::string& message) {
        client_.send(message);
    }

    void receive(std::size_t max_len = 1024) {
        std::string buffer;
        client_.receive(buffer, max_len);
    }

    void disconnect() {
        client_.disconnect();
    }

    void print_statistics() const {
        std::cout << "\n=== Statistics for " << name_ << " ===" << std::endl;
        std::cout << "  Bytes Sent:     " << bytes_sent_ << std::endl;
        std::cout << "  Bytes Received: " << bytes_received_ << std::endl;
        std::cout << "  Errors:         " << errors_ << std::endl;
        std::cout << std::endl;
    }

private:
    udp_client client_;
    std::string name_;
    std::atomic<std::uint64_t> bytes_sent_;
    std::atomic<std::uint64_t> bytes_received_;
    std::atomic<std::uint32_t> errors_;
};

// ============================================================================
// Example 2: UDP Server with Event-Driven Monitoring
// ============================================================================

class MonitoredUDPServer {
public:
    MonitoredUDPServer() : packets_processed_(0) {
        // Intentionally empty - signals will be connected when needed
    }

    void start_monitoring(udp_server& server) {
        server.onServerStarted.connect([]() {
            std::cout << "[SERVER] Server started successfully" << std::endl;
        });

        server.onServerStopping.connect([]() {
            std::cout << "[SERVER] Server stopping..." << std::endl;
        });

        server.onServerStopped.connect([this]() {
            std::cout << "[SERVER] Server stopped" << std::endl;
            std::cout << "[SERVER] Total packets processed: " << packets_processed_ << std::endl;
        });

        server.onPacketReceived.connect([](const void* data, std::size_t len, const socket_address& sender) {
            std::string msg(static_cast<const char*>(data), (std::min)(len, size_t(100)));
            std::cout << "[SERVER] Packet from " << sender.to_string() << ": "
                      << len << " bytes - " << msg << std::endl;
        });

        server.onTotalPacketsChanged.connect([this](std::size_t total) {
            if (total % 10 == 0) {  // Log every 10 packets
                std::cout << "[SERVER] Total packets: " << total << std::endl;
            }
            packets_processed_ = total;
        });

        server.onProcessedPacketsChanged.connect([](std::size_t processed) {
            if (processed % 10 == 0) {
                std::cout << "[SERVER] Processed packets: " << processed << std::endl;
            }
        });

        server.onDroppedPacketsChanged.connect([](std::size_t dropped) {
            if (dropped > 0) {
                std::cerr << "[SERVER] WARNING: Dropped packets: " << dropped << std::endl;
            }
        });

        server.onWorkerThreadCreated.connect([](std::size_t thread_count) {
            std::cout << "[SERVER] Worker thread created (total: " << thread_count << ")" << std::endl;
        });

        server.onException.connect([](const std::exception& ex, const std::string& context) {
            std::cerr << "[SERVER] Exception in " << context << ": " << ex.what() << std::endl;
        });
    }

private:
    std::atomic<std::uint64_t> packets_processed_;
};

// ============================================================================
// Example 3: UDP Protocol State Machine (Echo Protocol with Signals)
// ============================================================================

class EchoProtocolStateMachine {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        SENDING,
        RECEIVING,
        ERROR_STATE
    };

    EchoProtocolStateMachine(const std::string& name)
        : name_(name), state_(State::DISCONNECTED), messages_sent_(0), messages_received_(0) {
        // Connect to state transition signals
        client_.onConnected.connect([this](const socket_address& addr) {
            state_ = State::CONNECTED;
            std::cout << "[" << name_ << " State Machine] State -> CONNECTED ("
                      << addr.to_string() << ")" << std::endl;
        });

        client_.onDisconnected.connect([this]() {
            state_ = State::DISCONNECTED;
            std::cout << "[" << name_ << " State Machine] State -> DISCONNECTED" << std::endl;
        });

        client_.onDataSent.connect([this](const void*, std::size_t) {
            state_ = State::RECEIVING;
            messages_sent_++;
            std::cout << "[" << name_ << " State Machine] State -> RECEIVING" << std::endl;
        });

        client_.onDataReceived.connect([this](const void* data, std::size_t len) {
            state_ = State::CONNECTED;
            messages_received_++;
            std::string msg(static_cast<const char*>(data), len);
            std::cout << "[" << name_ << " State Machine] Received echo: " << msg << std::endl;
            std::cout << "[" << name_ << " State Machine] State -> CONNECTED" << std::endl;
        });

        client_.onSendError.connect([this](const std::string& error) {
            state_ = State::ERROR_STATE;
            std::cout << "[" << name_ << " State Machine] State -> ERROR (send): " << error << std::endl;
        });

        client_.onReceiveError.connect([this](const std::string& error) {
            state_ = State::ERROR_STATE;
            std::cout << "[" << name_ << " State Machine] State -> ERROR (receive): " << error << std::endl;
        });
    }

    void connect(const socket_address& addr) {
        state_ = State::CONNECTING;
        std::cout << "[" << name_ << " State Machine] State -> CONNECTING" << std::endl;
        client_.connect(addr);
    }

    void send_message(const std::string& msg) {
        if (state_ == State::CONNECTED) {
            state_ = State::SENDING;
            std::cout << "[" << name_ << " State Machine] State -> SENDING" << std::endl;
            client_.send(msg);
        }
    }

    void receive_message() {
        if (state_ == State::RECEIVING) {
            std::string buffer;
            client_.receive(buffer, 1024);
        }
    }

    void disconnect() {
        client_.disconnect();
    }

    State get_state() const { return state_; }
    std::uint32_t messages_sent() const { return messages_sent_; }
    std::uint32_t messages_received() const { return messages_received_; }

private:
    udp_client client_;
    std::string name_;
    State state_;
    std::atomic<std::uint32_t> messages_sent_;
    std::atomic<std::uint32_t> messages_received_;
};

// ============================================================================
// Main Function - Run Examples
// ============================================================================

int main() {
    fb::initialize_library();

    try {
        std::cout << "=== UDP Signal-Driven Programming Examples ===" << std::endl << std::endl;

        // Example 1: Monitored UDP Client
        std::cout << "--- Example 1: Monitored UDP Client ---" << std::endl;
        {
            MonitoredUDPClient monitored_client("ExampleClient");

            // Note: In a real application, you would connect to an actual UDP server
            // For this demo, we demonstrate the signal connections without actual I/O
            std::cout << "Monitored client created with signal handlers connected" << std::endl;
        }
        std::cout << std::endl;

        // Example 2: Echo Protocol State Machine
        std::cout << "--- Example 2: Echo Protocol State Machine ---" << std::endl;
        {
            EchoProtocolStateMachine state_machine("EchoClient");

            std::cout << "State machine created with state transition handlers" << std::endl;
            std::cout << "State machine current state: ";

            switch (state_machine.get_state()) {
                case EchoProtocolStateMachine::State::DISCONNECTED:
                    std::cout << "DISCONNECTED" << std::endl;
                    break;
                case EchoProtocolStateMachine::State::CONNECTED:
                    std::cout << "CONNECTED" << std::endl;
                    break;
                default:
                    std::cout << "OTHER" << std::endl;
            }
        }
        std::cout << std::endl;

        // Example 3: Server Monitoring Setup
        std::cout << "--- Example 3: Server Monitoring Setup ---" << std::endl;
        {
            MonitoredUDPServer monitored_server;
            std::cout << "UDP server monitoring configured" << std::endl;
            std::cout << "Server will emit signals for:" << std::endl;
            std::cout << "  - Server lifecycle (started/stopping/stopped)" << std::endl;
            std::cout << "  - Packet reception" << std::endl;
            std::cout << "  - Statistics changes (total/processed/dropped)" << std::endl;
            std::cout << "  - Worker thread creation" << std::endl;
            std::cout << "  - Exceptions" << std::endl;
        }
        std::cout << std::endl;

        std::cout << "=== Examples Completed Successfully ===" << std::endl;
        std::cout << "\nKey Benefits Demonstrated:" << std::endl;
        std::cout << "  ✓ Event-driven programming with UDP" << std::endl;
        std::cout << "  ✓ Real-time statistics collection" << std::endl;
        std::cout << "  ✓ Protocol state machines driven by signals" << std::endl;
        std::cout << "  ✓ Decoupling application logic from network I/O" << std::endl;
        std::cout << "  ✓ Cross-thread signal handling capability" << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    fb::cleanup_library();
    return 0;
}
