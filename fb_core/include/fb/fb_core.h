#pragma once

#include <string>

// Include fb_core components
#include "stop_watch.h"
#include "timer.h"

namespace fb {

/// @brief Describes high-level metadata about the fb_core library.
struct library_info
{
  std::string name;
  std::string version;
  std::string tagline;
};

/// @brief Returns build metadata.
///
/// The data is stored in a single place so tests and examples can assert
/// the library is wired correctly without depending on implementation details.
const library_info& get_library_info();

} // namespace fb
