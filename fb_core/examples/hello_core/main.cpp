#include <iostream>
#include <fb/fb_core.h>

int main()
{
  const auto& info = fb::get_library_info();

  std::cout << "FB Core Library\n";
  std::cout << "---------------------\n";
  std::cout << "Name:    " << info.name << '\n';
  std::cout << "Version: " << info.version << '\n';
  std::cout << "Tagline: " << info.tagline << '\n';

  return 0;
}
