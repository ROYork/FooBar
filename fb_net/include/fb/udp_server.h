#pragma once

#include <fb/udp_socket.h>
#include <fb/udp_handler.h>
#include <fb/socket_address.h>
#include <fb/fb_signal.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

namespace fb {

/**
 * @brief Multi-threaded UDP server implementation.
 *
 * The udp_server manages incoming UDP packets using a configurable thread pool.
 * It receives packets on a udp_socket and dispatches them to UDPHandler
 * instances running in worker threads. The server provides packet management,
 * graceful shutdown, and configurable limits.
 *
 * @warning Move Operations: The server must be stopped before moving. Moving a running
 *          server results in undefined behavior because worker threads cannot be
 *          transferred between instances. Always call stop() before move construction
 *          or move assignment.
 */
class udp_server
{
public:
    struct PacketData
    {
        std::vector<std::uint8_t> buffer;
        socket_address sender_address;
        std::chrono::steady_clock::time_point received_time;
        
        PacketData(const void* data, std::size_t length, const socket_address& sender)
         : buffer(static_cast<const std::uint8_t*>(data), 
                  static_cast<const std::uint8_t*>(data) + length),
           sender_address(sender),
           received_time(std::chrono::steady_clock::now())
        {
        }
    };

    using HandlerFactory = std::function<std::unique_ptr<udp_handler>(const PacketData& packet_data)>;

    udp_server();

    udp_server(udp_socket server_socket, 
               HandlerFactory handler_factory,
               std::size_t max_threads = 0,
               std::size_t max_queued = 0);

    udp_server(udp_socket server_socket, 
               std::shared_ptr<udp_handler> handler,
               std::size_t max_threads = 0,
               std::size_t max_queued = 0);

    udp_server(const udp_server&) = delete;

    udp_server(udp_server&& other) noexcept;

    udp_server& operator=(const udp_server&) = delete;

    udp_server& operator=(udp_server&& other) noexcept;

    virtual ~udp_server();

    void start();

    // Note: timeout is best-effort; stop may block until worker threads exit.
    void stop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(5000));

    bool is_running() const;

    void set_server_socket(udp_socket server_socket);
    void set_handler_factory(HandlerFactory factory);
    void set_shared_handler(std::shared_ptr<udp_handler> handler);
    void set_max_threads(std::size_t max_threads);
    void set_max_queued(std::size_t max_queued);
    void set_packet_buffer_size(std::size_t size);
    void set_packet_timeout(const std::chrono::milliseconds& timeout);

    const udp_socket& server_socket() const;

    size_t thread_count() const;
    uint64_t total_packets() const;
    uint64_t processed_packets() const;
    uint64_t dropped_packets() const;

    size_t queued_packets() const;

    std::chrono::steady_clock::duration uptime() const;

    // fb::signal members for event-driven programming
    // Server lifecycle signals
    fb::signal<> onServerStarted;                          ///< Emitted when server starts
    fb::signal<> onServerStopping;                             ///< Emitted when server stops
    fb::signal<> onServerStopped;                              ///< Emitted after server stopped

    // Packet reception signals
    fb::signal<const void*, std::size_t, const socket_address&> onPacketReceived;  ///< Emitted when packet received
    fb::signal<std::size_t> onTotalPacketsChanged;                ///< Emitted when total packet count changes
    fb::signal<std::size_t> onProcessedPacketsChanged;            ///< Emitted when processed packet count changes
    fb::signal<std::size_t> onDroppedPacketsChanged;              ///< Emitted when dropped packet count changes
    fb::signal<std::size_t> onQueuedPacketsChanged;               ///< Emitted when queued packet count changes

    // Threading signals
    fb::signal<std::size_t> onWorkerThreadCreated;                ///< Emitted when worker thread created
    fb::signal<std::size_t> onWorkerThreadDestroyed;              ///< Emitted when worker thread destroyed
    fb::signal<std::size_t> onActiveThreadsChanged;               ///< Emitted when active thread count changes

    // Error signals
    fb::signal<const std::exception&, const std::string&> onException;  ///< Emitted on exception

protected:

    virtual void handle_exception(const std::exception& ex, const std::string& context) noexcept;
    virtual void on_server_starting();
    virtual void on_server_stopping();

private:
    // Server state
    udp_socket m_server_socket;
    std::shared_ptr<udp_handler> m_shared_handler;
    HandlerFactory m_handler_factory;

    std::atomic<bool> m_running;
    std::atomic<bool> m_should_stop;
    
    // Threading infrastructure
    std::thread m_receiver_thread;
    std::vector<std::thread> m_worker_threads;
    std::queue<std::unique_ptr<PacketData>> m_packet_queue;
    mutable std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    mutable std::mutex m_threads_mutex;
    
    // Configuration
    std::size_t m_max_threads;
    std::size_t m_max_queued;
    std::size_t m_packet_buffer_size;
    std::chrono::milliseconds m_packet_timeout;
    
    // Statistics
    std::atomic<std::uint64_t> m_total_packets;
    std::atomic<std::uint64_t> m_processed_packets;
    std::atomic<std::uint64_t> m_dropped_packets;
    std::chrono::steady_clock::time_point m_start_time;
    
    // Configuration validation
    bool m_has_socket;
    bool m_has_handler;
    
    // Thread pool management
    static constexpr std::size_t DEFAULT_MAX_THREADS = 10;
    static constexpr std::size_t DEFAULT_MAX_QUEUED = 1000;
    static constexpr std::size_t DEFAULT_PACKET_BUFFER_SIZE = 65507; // Max UDP payload
    static constexpr auto DEFAULT_PACKET_TIMEOUT = std::chrono::milliseconds(0);

    void receiver_thread_proc();
    void worker_thread_proc();
    void process_packet(std::unique_ptr<PacketData> packet_data);
    void cleanup_expired_packets();
    void add_worker_thread_if_needed();
    void validate_configuration() const;
    void shutdown_threads(const std::chrono::milliseconds& timeout);
    bool is_configured() const;

    HandlerFactory create_shared_handler_factory(std::shared_ptr<udp_handler> handler);
};

} // namespace fb
