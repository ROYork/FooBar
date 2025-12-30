/**
 * @file chat_server.cpp
 * @brief Multi-client Chat Server Example using FB_NET
 * 
 * This example demonstrates a multi-threaded chat server that handles multiple
 * clients simultaneously. Clients can join chat rooms, send messages to all
 * connected clients, and receive real-time message broadcasts.
 * 
 * Features demonstrated:
 * - Multi-client TCP server with connection management
 * - Message broadcasting to all connected clients
 * - Thread-safe client list management
 * - Chat protocol implementation
 * - Graceful connection handling and cleanup
 * 
 * Usage:
 *   ./chat_server [port]
 *   Default port: 9090
 *   
 *   Test with multiple chat_client instances or telnet
 */

#include <fb/fb_net.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <sstream>
#include <system_error>

using namespace fb;

/**
 * @brief Chat Room Manager - handles client list and message broadcasting
 */
class ChatRoom
{
public:
    struct Client {
        std::string nickname;
        std::string address;
        tcp_client socket;
        std::chrono::steady_clock::time_point join_time;
        std::atomic<bool> active{true};
        
        Client(std::string nick, std::string addr, tcp_client sock)
            : nickname(std::move(nick))
            , address(std::move(addr))
            , socket(std::move(sock))
            , join_time(std::chrono::steady_clock::now())
        {}
    };

    static ChatRoom& instance() {
        static ChatRoom room;
        return room;
    }

    void add_client(std::shared_ptr<Client> client) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients.push_back(client);
        
        std::cout << "[ROOM] " << client->nickname << " (" << client->address 
                  << ") joined. Total clients: " << m_clients.size() << std::endl;
        
        // Notify other clients
        std::string join_msg = "*** " + client->nickname + " joined the chat ***";
        broadcast_message(join_msg, client.get());
        
        // Send welcome message to new client
        send_welcome_message(client);
    }

    void remove_client(Client* client) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        
        auto it = std::find_if(m_clients.begin(), m_clients.end(),
            [client](const std::weak_ptr<Client>& weak_client) {
                if (auto shared_client = weak_client.lock()) {
                    return shared_client.get() == client;
                }
                return false;
            });
        
        if (it != m_clients.end()) {
            if (auto shared_client = it->lock()) {
                std::cout << "[ROOM] " << shared_client->nickname << " (" 
                          << shared_client->address << ") left. Total clients: " 
                          << (m_clients.size() - 1) << std::endl;
                
                // Notify other clients
                std::string leave_msg = "*** " + shared_client->nickname + " left the chat ***";
                broadcast_message(leave_msg, shared_client.get());
            }
            
            m_clients.erase(it);
        }
        
        // Clean up dead weak_ptr references
        cleanup_dead_clients();
    }

    void broadcast_message(const std::string& message, Client* sender = nullptr) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        
        std::string timestamp = get_timestamp();
        std::string formatted_msg;
        
        if (sender) {
            formatted_msg = "[" + timestamp + "] <" + sender->nickname + "> " + message + "\n";
        } else {
            formatted_msg = "[" + timestamp + "] " + message + "\n";
        }
        
        std::cout << "[BROADCAST] " << formatted_msg;
        
        for (auto& weak_client : m_clients) {
            if (auto client = weak_client.lock()) {
                if (client.get() != sender && client->active.load()) {
                    try {
                        client->socket.send(formatted_msg);
                    } catch (const std::exception& ex) {
                        std::cout << "[ERROR] Failed to send to " << client->nickname 
                                  << ": " << ex.what() << std::endl;
                        client->active.store(false);
                    }
                }
            }
        }
    }

    std::vector<std::string> get_client_list() {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        std::vector<std::string> list;
        
        for (auto& weak_client : m_clients) {
            if (auto client = weak_client.lock()) {
                if (client->active.load()) {
                    auto duration = std::chrono::steady_clock::now() - client->join_time;
                    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                    
                    list.push_back(client->nickname + " (" + client->address + 
                                 ", online " + std::to_string(seconds) + "s)");
                }
            }
        }
        
        return list;
    }

private:
    std::vector<std::weak_ptr<Client>> m_clients;
    std::mutex m_clients_mutex;

    void send_welcome_message(std::shared_ptr<Client> client) {
        std::string welcome = R"(
======================================
   Welcome to FB_NET Chat Server!
======================================
Connected as: )" + client->nickname + R"(
Server: FB_NET Chat v1.0

Commands:
  /help     - Show this help
  /list     - List online users
  /quit     - Leave the chat

Type your message and press Enter to chat!
======================================

)";
        
        try {
            client->socket.send(welcome);
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Failed to send welcome to " << client->nickname 
                      << ": " << ex.what() << std::endl;
        }
    }

    void cleanup_dead_clients() {
        m_clients.erase(
            std::remove_if(m_clients.begin(), m_clients.end(),
                [](const std::weak_ptr<Client>& weak_client) {
                    return weak_client.expired();
                }), 
            m_clients.end()
        );
    }

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_value = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_value);
        
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        return std::string(buffer);
    }
};

/**
 * @brief Chat Client Connection Handler
 */
class ChatConnection : public tcp_server_connection
{
public:
    ChatConnection(tcp_client socket, const socket_address& client_address)
        : tcp_server_connection(std::move(socket), client_address)
    {
    }

    void run() override
    {
        try {
            // Set socket timeouts
            socket().set_receive_timeout(std::chrono::seconds(300)); // 5 minutes
            socket().set_send_timeout(std::chrono::seconds(30));
            
            // Get client nickname
            std::string nickname = get_client_nickname();
            if (nickname.empty()) {
                return; // Failed to get nickname
            }
            
            // Create client object and add to chat room
            auto client = std::make_shared<ChatRoom::Client>(
                nickname, client_address().to_string(), std::move(socket()));
            
            ChatRoom::instance().add_client(client);
            
            // Handle client messages
            handle_client_messages(client);
            
            // Remove client from chat room
            ChatRoom::instance().remove_client(client.get());
            
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Connection error: " << ex.what() << std::endl;
        }
    }

private:
    std::string get_client_nickname()
    {
        try {
            socket().send("Welcome to FB_NET Chat Server!\nEnter your nickname: ");
            
            std::string nickname;
            socket().receive(nickname, 256);
            
            // Clean up nickname (remove whitespace and newlines)
            nickname.erase(std::remove_if(nickname.begin(), nickname.end(),
                [](char c) { return std::isspace(c); }), nickname.end());
            
            if (nickname.empty() || nickname.length() > 50) {
                socket().send("Invalid nickname. Connection closed.\n");
                return "";
            }
            
            // Check for reserved names and commands
            if (nickname[0] == '/' || nickname == "server" || nickname == "system") {
                socket().send("Reserved nickname. Please choose another.\n");
                return "";
            }
            
            return nickname;
            
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Failed to get nickname: " << ex.what() << std::endl;
            return "";
        }
    }

    void handle_client_messages(std::shared_ptr<ChatRoom::Client> client)
    {
        while (!stop_requested() && client->active.load()) {
            try {
                std::string message;
                int received = client->socket.receive(message, 1024);
                
                if (received <= 0) {
                    break; // Connection closed
                }
                
                // Clean up message
                message.erase(std::remove_if(message.begin(), message.end(),
                    [](char c) { return c == '\r' || c == '\n'; }), message.end());
                
                if (message.empty()) {
                    continue;
                }
                
                // Handle chat commands
                if (message[0] == '/') {
                    handle_command(client, message);
                } else {
                    // Regular chat message - broadcast to all clients
                    ChatRoom::instance().broadcast_message(message, client.get());
                }
                
            } catch (const std::system_error&) {
                // Send keepalive
                try {
                    client->socket.send(""); // Empty message as keepalive
                } catch (...) {
                    break; // Connection lost
                }
            } catch (const std::exception& ex) {
                std::cout << "[ERROR] Message handling error for " << client->nickname 
                          << ": " << ex.what() << std::endl;
                break;
            }
        }
    }

    void handle_command(std::shared_ptr<ChatRoom::Client> client, const std::string& command)
    {
        try {
            if (command == "/quit" || command == "/exit") {
                client->socket.send("Goodbye!\n");
                client->active.store(false);
            }
            else if (command == "/help") {
                std::string help = R"(Available commands:
/help     - Show this help message
/list     - List all online users
/quit     - Leave the chat
/exit     - Same as /quit

Just type your message to chat with everyone!
)";
                client->socket.send(help);
            }
            else if (command == "/list") {
                auto client_list = ChatRoom::instance().get_client_list();
                std::ostringstream oss;
                oss << "Online users (" << client_list.size() << "):\n";
                for (const auto& user : client_list) {
                    oss << "  " << user << "\n";
                }
                client->socket.send(oss.str());
            }
            else {
                client->socket.send("Unknown command. Type /help for available commands.\n");
            }
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Command handling error for " << client->nickname 
                      << ": " << ex.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[])
{
    std::cout << "FB_NET Chat Server Example" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Initialize FB_NET library
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        int port = 9090;
        if (argc > 1) {
            port = std::atoi(argv[1]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Invalid port number. Using default port 9090." << std::endl;
                port = 9090;
            }
        }
        
        // Create server socket
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.set_reuse_address(true);
        server_socket.bind(socket_address("0.0.0.0", static_cast<std::uint16_t>(port)));
        server_socket.listen(50);
        
        std::cout << "Chat server listening on " << server_socket.address().to_string() << std::endl;
        std::cout << "Clients can connect using: ./chat_client localhost " << port << std::endl;
        std::cout << "Or with telnet: telnet localhost " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;
        
        // Connection factory
        auto connection_factory = [](tcp_client socket, const socket_address& client_addr) {
            return std::make_unique<ChatConnection>(std::move(socket), client_addr);
        };
        
        // Create and start TCP server
        tcp_server server(std::move(server_socket), connection_factory, 50, 200);
        server.start();
        
        std::cout << "Chat server started with " << server.thread_count() << " threads" << std::endl;
        std::cout << "Ready to accept chat clients..." << std::endl;
        
        // Keep server running and show periodic statistics
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            auto client_list = ChatRoom::instance().get_client_list();
            std::cout << "[STATS] Active connections: " << server.active_connections()
                      << ", Chat clients: " << client_list.size()
                      << ", Total handled: " << server.total_connections() << std::endl;
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    
    // Cleanup FB_NET library
    fb::cleanup_library();
    return 0;
}
