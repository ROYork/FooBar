/**
 * @file file_server.cpp
 * @brief File Transfer Server Example using FB_NET
 * 
 * This example demonstrates a file transfer server that can receive files
 * from clients and send files to clients. It showcases large data transfers
 * using socket_stream for efficient binary data handling.
 * 
 * Features demonstrated:
 * - TCP server for file transfer operations
 * - socket_stream usage for large binary data transfers
 * - File upload and download protocols
 * - Progress tracking for large transfers
 * - Binary data handling and integrity checking
 * 
 * Usage:
 *   ./file_server [port] [directory]
 *   Defaults: port=8080, directory=./files/
 *   
 *   Protocol:
 *   - UPLOAD <filename> <filesize> - Upload file to server
 *   - DOWNLOAD <filename> - Download file from server
 *   - LIST - List available files
 */

#include <fb/fb_net.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <memory>
#include <thread>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

using namespace fb;

/**
 * @brief File Transfer Connection Handler
 */
class FileTransferConnection : public tcp_server_connection
{
public:
    FileTransferConnection(tcp_client socket, const socket_address& client_address, 
                          const std::string& base_directory)
        : tcp_server_connection(std::move(socket), client_address)
        , m_base_directory(base_directory)
    {
        std::cout << "[" << get_timestamp() << "] Client connected: " 
                  << client_address.to_string() << std::endl;
    }

    void run() override
    {
        try {
            // Set socket timeouts
            socket().set_receive_timeout(std::chrono::seconds(300)); // 5 minutes
            socket().set_send_timeout(std::chrono::seconds(60));     // 1 minute
            
            // Send welcome message
            send_welcome();
            
            // Handle client commands
            while (!stop_requested()) {
                std::string command = receive_command();
                if (command.empty()) {
                    break; // Connection closed
                }
                
                if (!handle_command(command)) {
                    break; // Error or quit command
                }
            }
            
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Connection error: " << ex.what() << std::endl;
            send_response("ERROR Connection error: " + std::string(ex.what()));
        }
        
        std::cout << "[" << get_timestamp() << "] Client disconnected: " 
                  << client_address().to_string() << std::endl;
    }

private:
    std::string m_base_directory;
    static constexpr size_t BUFFER_SIZE = 65536; // 64KB buffer

    void send_welcome()
    {
        std::string welcome = R"(FB_NET File Transfer Server v1.0
Available commands:
  LIST                    - List available files
  DOWNLOAD <filename>     - Download file from server
  UPLOAD <filename> <size> - Upload file to server  
  QUIT                    - Disconnect

Ready for commands.
)";
        send_response(welcome);
    }

    std::string receive_command()
    {
        try {
            std::string line;
            socket().receive(line, 1024);
            
            // Remove trailing whitespace
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            
            std::cout << "[" << get_timestamp() << "] Command from " 
                      << client_address().to_string() << ": " << line << std::endl;
            
            return line;
        } catch (const std::exception&) {
            return ""; // Connection closed or error
        }
    }

    void send_response(const std::string& response)
    {
        try {
            std::string msg = response;
            if (!msg.empty() && msg.back() != '\n') {
                msg += "\n";
            }
            socket().send(msg);
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Failed to send response: " << ex.what() << std::endl;
        }
    }

    bool handle_command(const std::string& command)
    {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        // Convert to uppercase
        std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        
        try {
            if (cmd == "QUIT" || cmd == "EXIT") {
                send_response("Goodbye!");
                return false;
            }
            else if (cmd == "LIST") {
                handle_list_command();
            }
            else if (cmd == "DOWNLOAD") {
                std::string filename;
                iss >> filename;
                if (filename.empty()) {
                    send_response("ERROR Missing filename");
                } else {
                    handle_download_command(filename);
                }
            }
            else if (cmd == "UPLOAD") {
                std::string filename;
                size_t filesize;
                if (!(iss >> filename >> filesize)) {
                    send_response("ERROR Missing filename or filesize");
                } else {
                    handle_upload_command(filename, filesize);
                }
            }
            else {
                send_response("ERROR Unknown command: " + cmd);
            }
            
            return true;
            
        } catch (const std::exception& ex) {
            send_response("ERROR " + std::string(ex.what()));
            return true; // Continue despite error
        }
    }

    void handle_list_command()
    {
        std::ostringstream response;
        response << "Files in " << m_base_directory << ":\n";
        
        try {
            if (!std::filesystem::exists(m_base_directory)) {
                response << "Directory does not exist.\n";
                send_response(response.str());
                return;
            }
            
            size_t file_count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(m_base_directory)) {
                if (entry.is_regular_file()) {
                    auto size = entry.file_size();
                    response << "  " << std::setw(20) << std::left << entry.path().filename().string()
                             << " " << std::setw(12) << std::right << size << " bytes"
                             << "\n";
                    file_count++;
                }
            }
            
            if (file_count == 0) {
                response << "No files found.\n";
            } else {
                response << "Total: " << file_count << " files\n";
            }
            
        } catch (const std::exception& ex) {
            response << "ERROR listing files: " << ex.what() << "\n";
        }
        
        send_response(response.str());
    }

    void handle_download_command(const std::string& filename)
    {
        // Sanitize filename (prevent directory traversal)
        std::string safe_filename = sanitize_filename(filename);
        if (safe_filename.empty()) {
            send_response("ERROR Invalid filename");
            return;
        }
        
        std::string filepath = m_base_directory + "/" + safe_filename;
        
        try {
            if (!std::filesystem::exists(filepath)) {
                send_response("ERROR File not found: " + safe_filename);
                return;
            }
            
            std::ifstream file(filepath, std::ios::binary);
            if (!file.is_open()) {
                send_response("ERROR Cannot open file: " + safe_filename);
                return;
            }
            
            // Get file size
            file.seekg(0, std::ios::end);
            const auto end_pos = file.tellg();
            if (end_pos < 0) {
                send_response("ERROR Failed to determine file size");
                return;
            }
            const auto filesize = static_cast<std::size_t>(end_pos);
            file.seekg(0, std::ios::beg);
            
            // Send file info
            send_response("OK " + std::to_string(filesize));
            
            // Send file data
            std::vector<char> buffer(BUFFER_SIZE);
            std::size_t total_sent = 0;
            auto start_time = std::chrono::steady_clock::now();
            
            while (!file.eof() && total_sent < filesize) {
                file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                const auto bytes_read = file.gcount();
                
                if (bytes_read > 0) {
                    if (bytes_read >
                        static_cast<std::streamsize>((std::numeric_limits<int>::max)())) {
                        throw std::length_error("Chunk size exceeds socket send capacity");
                    }
                    
                    socket().send_bytes_all(buffer.data(), static_cast<int>(bytes_read));
                    total_sent += static_cast<std::size_t>(bytes_read);
                    
                    // Progress update
                    if ((total_sent % (1024 * 1024) == 0 || total_sent == filesize) &&
                        filesize > 0) {
                        const auto progress =
                            (static_cast<double>(total_sent) * 100.0) /
                            static_cast<double>(filesize);
                        const auto previous_flags = std::cout.flags();
                        const auto previous_precision = std::cout.precision();
                        std::cout << "[DOWNLOAD] " << safe_filename << " - " << std::fixed
                                  << std::setprecision(1) << progress << "% (" << total_sent << "/"
                                  << filesize << " bytes)" << std::endl;
                        std::cout.flags(previous_flags);
                        std::cout.precision(previous_precision);
                    }
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            const auto duration_ms = duration.count();
            double speed = 0.0;
            if (duration_ms > 0) {
                speed = (static_cast<double>(total_sent) * 1000.0) /
                        (static_cast<double>(duration_ms) * 1024.0 * 1024.0);
            }
            
            const auto previous_flags = std::cout.flags();
            const auto previous_precision = std::cout.precision();
            std::cout << "[DOWNLOAD] Completed " << safe_filename << " - " << total_sent
                      << " bytes in " << duration_ms << "ms (" << std::fixed
                      << std::setprecision(2) << speed << " MB/s)" << std::endl;
            std::cout.flags(previous_flags);
            std::cout.precision(previous_precision);
            
        } catch (const std::exception& ex) {
            send_response("ERROR Transfer failed: " + std::string(ex.what()));
        }
    }

    void handle_upload_command(const std::string& filename, size_t filesize)
    {
        // Sanitize filename
        std::string safe_filename = sanitize_filename(filename);
        if (safe_filename.empty()) {
            send_response("ERROR Invalid filename");
            return;
        }
        
        // Check file size limits (e.g., 100MB max)
        if (filesize > 100 * 1024 * 1024) {
            send_response("ERROR File too large (max 100MB)");
            return;
        }
        
        std::string filepath = m_base_directory + "/" + safe_filename;
        
        try {
            // Create directory if needed
            std::filesystem::create_directories(m_base_directory);
            
            std::ofstream file(filepath, std::ios::binary);
            if (!file.is_open()) {
                send_response("ERROR Cannot create file: " + safe_filename);
                return;
            }
            
            // Acknowledge ready to receive
            send_response("OK Ready to receive " + std::to_string(filesize) + " bytes");
            
            // Receive file data
            std::vector<char> buffer(BUFFER_SIZE);
            std::size_t total_received = 0;
            auto start_time = std::chrono::steady_clock::now();
            
            while (total_received < filesize) {
                const auto remaining = filesize - total_received;
                const auto chunk_size = (std::min)(buffer.size(), remaining);
                const auto request_size = static_cast<int>(
                    (std::min)(chunk_size,
                               static_cast<std::size_t>((std::numeric_limits<int>::max)())));
                const int bytes_received = socket().receive_bytes(buffer.data(), request_size);
                
                if (bytes_received <= 0) {
                    throw std::runtime_error("Connection closed during transfer");
                }
                
                file.write(
                    buffer.data(),
                    static_cast<std::streamsize>(bytes_received));
                total_received += static_cast<std::size_t>(bytes_received);
                
                // Progress update
                if ((total_received % (1024 * 1024) == 0 || total_received == filesize) &&
                    filesize > 0) {
                    const auto progress =
                        (static_cast<double>(total_received) * 100.0) /
                        static_cast<double>(filesize);
                    const auto previous_flags = std::cout.flags();
                    const auto previous_precision = std::cout.precision();
                    std::cout << "[UPLOAD] " << safe_filename << " - " << std::fixed
                              << std::setprecision(1) << progress << "% (" << total_received << "/"
                              << filesize << " bytes)" << std::endl;
                    std::cout.flags(previous_flags);
                    std::cout.precision(previous_precision);
                }
            }
            
            file.close();
            
            auto end_time = std::chrono::steady_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            const auto duration_ms = duration.count();
            double speed = 0.0;
            if (duration_ms > 0) {
                speed = (static_cast<double>(total_received) * 1000.0) /
                        (static_cast<double>(duration_ms) * 1024.0 * 1024.0);
            }
            
            const auto previous_flags = std::cout.flags();
            const auto previous_precision = std::cout.precision();
            std::cout << "[UPLOAD] Completed " << safe_filename << " - " << total_received
                      << " bytes in " << duration_ms << "ms (" << std::fixed
                      << std::setprecision(2) << speed << " MB/s)" << std::endl;
            std::cout.flags(previous_flags);
            std::cout.precision(previous_precision);
            
            send_response("OK Upload completed successfully");
            
        } catch (const std::exception& ex) {
            // Clean up partial file
            try {
                std::filesystem::remove(filepath);
            } catch (...) {}
            
            send_response("ERROR Upload failed: " + std::string(ex.what()));
        }
    }

    std::string sanitize_filename(const std::string& filename)
    {
        // Remove directory separators and other dangerous characters
        std::string safe = filename;
        
        // Remove path separators
        size_t pos = safe.find_last_of("/\\");
        if (pos != std::string::npos) {
            safe = safe.substr(pos + 1);
        }
        
        // Check for empty or dangerous names
        if (safe.empty() || safe == "." || safe == ".." || safe.find_first_of("<>:\"|?*") != std::string::npos) {
            return "";
        }
        
        return safe;
    }

    std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_value = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_value);
        
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        return std::string(buffer);
    }
};

int main(int argc, char* argv[])
{
    std::cout << "FB_NET File Transfer Server Example" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Initialize FB_NET library
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        int port = 8080;
        std::string directory = "./files";
        
        if (argc > 1) port = std::atoi(argv[1]);
        if (argc > 2) directory = argv[2];
        
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number. Using default port 8080." << std::endl;
            port = 8080;
        }
        
        // Create base directory
        std::filesystem::create_directories(directory);
        
        // Create server socket
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.set_reuse_address(true);
        server_socket.bind(socket_address("0.0.0.0", static_cast<std::uint16_t>(port)));
        server_socket.listen(10);
        
        std::cout << "File transfer server listening on " << server_socket.address().to_string() << std::endl;
        std::cout << "Base directory: " << std::filesystem::absolute(directory).string() << std::endl;
        std::cout << "Test with: ./file_client localhost " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;
        
        // Connection factory
        auto connection_factory = [directory](tcp_client socket, const socket_address& client_addr) {
            return std::make_unique<FileTransferConnection>(std::move(socket), client_addr, directory);
        };
        
        // Create and start TCP server
        tcp_server server(std::move(server_socket), connection_factory, 10, 50);
        server.start();
        
        std::cout << "File server started with " << server.thread_count() << " threads" << std::endl;
        
        // Keep server running and show statistics
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            std::cout << "[STATS] Active connections: " << server.active_connections()
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
