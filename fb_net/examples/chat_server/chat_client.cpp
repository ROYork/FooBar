/**
 * @file chat_client.cpp
 * @brief Chat Client Example for FB_NET Chat Server
 * 
 * This client connects to the FB_NET chat server and provides a simple
 * command-line interface for chatting with other connected users.
 * 
 * Features demonstrated:
 * - tcp_client connection to TCP server
 * - Bi-directional communication (send and receive)
 * - Multi-threaded client (separate threads for sending and receiving)
 * - Real-time message display
 * 
 * Usage:
 *   ./chat_client [host] [port]
 *   Default: localhost 9090
 */

#include <atomic>
#include <fb/fb_net.h>
#include <iostream>
#include <string>
#include <thread>

using namespace fb;

class ChatClient
{
public:
  ChatClient(const std::string & host, int port) :
    m_host(host),
    m_port(port),
    m_running(false)
  {
  }

  bool connect()
  {
    try
    {
      m_socket = tcp_client(socket_address::Family::IPv4);
      m_socket.connect(socket_address(m_host, static_cast<std::uint16_t>(m_port)));
      m_running = true;
      return true;
    }
    catch (const std::exception & ex)
    {
      std::cerr << "Connection failed: " << ex.what() << std::endl;
      return false;
    }
  }

  void run()
  {
    if (!m_running)
      return;

    // Start receiver thread
    std::thread receiver_thread(&ChatClient::receiver_loop, this);

    // Handle user input in main thread
    input_loop();

    // Wait for receiver thread to finish
    m_running = false;
    receiver_thread.join();
  }

  void disconnect()
  {
    m_running = false;
    try
    {
      m_socket.close();
    }
    catch (...)
    {
      // Ignore errors during disconnect
    }
  }

private:

  std::string m_host;
  int m_port;
  tcp_client m_socket;
  std::atomic<bool> m_running;

  void receiver_loop()
  {
    while (m_running)
    {
      try
      {
        std::string message;
        int received = m_socket.receive(message, 4096);
        if (received > 0)
        {
          std::cout << message;
          std::cout.flush();
        }
        else
        {
          break; // Connection closed
        }
      }
      catch (const std::exception &)
      {
        if (m_running)
        {
          std::cout << "\n[Connection lost]" << std::endl;
        }
        break;
      }
    }
  }

  void input_loop()
  {
    std::string line;
    while (m_running && std::getline(std::cin, line))
    {
      if (line == "/quit" || line == "/exit")
      {
        break;
      }

      try
      {
        line += "\n";
        m_socket.send(line);
      }
      catch (const std::exception & ex)
      {
        std::cerr << "Send failed: " << ex.what() << std::endl;
        break;
      }
    }
  }
};

int main(int argc, char * argv[])
{
  std::cout << "FB_NET Chat Client" << std::endl;
  std::cout << "==================" << std::endl;

  // Initialize FB_NET library
  fb::initialize_library();

  try
  {
    std::string host = "localhost";
    int port         = 9090;

    if (argc > 1)
      host = argv[1];
    if (argc > 2)
      port = std::atoi(argv[2]);

    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;

    ChatClient client(host, port);
    if (client.connect())
    {
      std::cout << "Connected! Start typing messages (type /quit to exit)" << std::endl;
      client.run();
      client.disconnect();
    }
  }
  catch (const std::exception & ex)
  {
    std::cerr << "Client error: " << ex.what() << std::endl;
    return 1;
  }

  fb::cleanup_library();
  return 0;
}
