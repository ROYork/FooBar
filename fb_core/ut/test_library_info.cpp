#include <gtest/gtest.h>

#include <fb/fb_core.h>

TEST(LibraryInfoTest, ReturnsStaticMetadata)
{
  const auto& info = fb::get_library_info();

  EXPECT_EQ(info.name, "fb_core");
  EXPECT_FALSE(info.version.empty());
  EXPECT_FALSE(info.tagline.empty());
}
