/**
 * @file file_client.cpp
 * @brief File Transfer Client Example using FB_NET
 * 
 * Simple command-line client for the file transfer server.
 * 
 * Usage:
 *   ./file_client [host] [port]
 *   Defaults: localhost 8080
 */

#include <fb/fb_net.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace fb;

class FileClient
{
public:
    FileClient(const std::string& host, int port)
        : m_host(host), m_port(port), m_connected(false)
    {
    }

    bool connect()
    {
        try {
            m_socket = tcp_client(socket_address::Family::IPv4);
            m_socket.connect(socket_address(m_host, static_cast<std::uint16_t>(m_port)));
            m_connected = true;
            
            // Read welcome message
            std::string welcome;
            m_socket.receive(welcome, 4096);
            std::cout << welcome << std::endl;
            
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "Connection failed: " << ex.what() << std::endl;
            return false;
        }
    }

    void run()
    {
        if (!m_connected) return;

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit" || line == "exit") {
                break;
            }

            try {
                m_socket.send(line + "\n");
                
                std::string response;
                m_socket.receive(response, 4096);
                std::cout << response << std::endl;
                
            } catch (const std::exception& ex) {
                std::cerr << "Communication error: " << ex.what() << std::endl;
                break;
            }
        }
    }

    void disconnect()
    {
        try {
            if (m_connected) {
                m_socket.send("QUIT\n");
                m_socket.close();
            }
        } catch (...) {}
        m_connected = false;
    }

private:
    std::string m_host;
    int m_port;
    tcp_client m_socket;
    bool m_connected;
};

int main(int argc, char* argv[])
{
    std::cout << "FB_NET File Transfer Client" << std::endl;
    std::cout << "============================" << std::endl;

    fb::initialize_library();

    try {
        std::string host = "localhost";
        int port = 8080;

        if (argc > 1) host = argv[1];
        if (argc > 2) port = std::atoi(argv[2]);

        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;

        FileClient client(host, port);
        if (client.connect()) {
            std::cout << "Connected! Enter commands (LIST, DOWNLOAD <file>, UPLOAD <file> <size>, QUIT):" << std::endl;
            client.run();
            client.disconnect();
        }

    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }

    fb::cleanup_library();
    return 0;
}