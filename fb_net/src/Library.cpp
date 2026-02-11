/**
 * @file Library.cpp
 * @brief FB_NET library initialization and cleanup implementation
 */

#include <fb/fb_net.h>
#include <atomic>
#include <mutex>
#include <system_error>

#ifdef FB_NET_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace fb {

namespace
{
std::atomic<bool> g_initialized{false};
std::mutex g_init_mutex;
std::atomic<int> g_init_count{0};
} // namespace

/**
 * @brief Initialize FB_NET library (optional)
 *
 * This function performs any necessary library initialization. On most
 * platforms, this is not required as the library initializes automatically.
 * However, on Windows, this may initialize Winsock if needed.
 *
 * @note This function is thread-safe and can be called multiple times safely
 * @throws std::system_error if initialization fails
 */
void initialize_library()
{
  std::lock_guard<std::mutex> lock(g_init_mutex);

  if (g_init_count++ > 0)
  {
    // Already initialized
    return;
  }

#ifdef FB_NET_PLATFORM_WINDOWS
  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0)
  {
    g_init_count--;
    throw std::system_error(std::error_code(result, std::system_category()),
                            "Failed to initialize Winsock");
  }

  // Verify that the WinSock DLL supports 2.2
  if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
  {
    WSACleanup();
    g_init_count--;
    throw std::system_error(std::make_error_code(std::errc::operation_not_supported),
                            "Winsock version 2.2 not supported");
  }
#endif

  g_initialized.store(true);
}

/**
 * @brief Cleanup FB_NET library (optional)
 *
 * This function performs cleanup of any library resources. This is optional
 * as cleanup happens automatically at program exit, but can be called
 * explicitly for deterministic cleanup timing.
 *
 * @note This function is thread-safe and can be called multiple times safely
 */
void cleanup_library()
{
  std::lock_guard<std::mutex> lock(g_init_mutex);

  if (g_init_count <= 0 || --g_init_count > 0)
  {
    // Not initialized or still has references
    return;
  }

#ifdef FB_NET_PLATFORM_WINDOWS
  WSACleanup();
#endif

  g_initialized.store(false);
}

/**
 * @brief Check whether the library has been initialised.
 *
 * @return True if `initialize_library()` has been called without a matching
 * cleanup.
 */
bool is_library_initialized() { return g_initialized.load(); }

} // namespace fb
