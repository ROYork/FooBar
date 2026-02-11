/**
 * @file test_main.cpp
 * @brief Main test runner for FB_NET unit tests using GTest framework
 * 
 * This file initializes the FB_NET library and runs all unit tests.
 * It also provides global test fixtures and utilities.
 */

#include <gtest/gtest.h>
#include <fb/fb_net.h>
#include <iostream>

/**
 * Global test environment for FB_NET
 */
class TBNetTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        std::cout << "Initializing FB_NET library for unit tests..." << std::endl;
        fb::initialize_library();
    }
    
    void TearDown() override
    {
        std::cout << "Cleaning up FB_NET library..." << std::endl;
        fb::cleanup_library();
    }
};

/**
 * Custom main function to set up global test environment
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new TBNetTestEnvironment);
    
    // Print test banner
    std::cout << "FB_NET Unit Test Suite" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Library Version: " << fb::Version::string << std::endl;
    std::cout << "Build Configuration: ";
#ifdef NDEBUG
    std::cout << "Release";
#else
    std::cout << "Debug";
#endif
    std::cout << std::endl;
    std::cout << "Platform: ";
#ifdef FB_NET_PLATFORM_WINDOWS
    std::cout << "Windows";
#elif defined(FB_NET_PLATFORM_MACOS)
    std::cout << "macOS";
#elif defined(FB_NET_PLATFORM_LINUX)
    std::cout << "Linux";
#else
    std::cout << "Unknown";
#endif
    std::cout << std::endl;
    std::cout << "C++ Standard: C++" << __cplusplus / 100 % 100 << std::endl;
    std::cout << std::endl;
    
    return RUN_ALL_TESTS();
}