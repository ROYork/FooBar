#include "fb/fb_core.h"

namespace fb
{

const library_info& get_library_info()
{
  static const library_info info{
      "fb_core", "0.1.0", "Foundational utilities for the FooBar library"};
  return info;
}

} // namespace fb
