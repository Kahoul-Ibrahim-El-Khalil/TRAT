#include "rat/media.hpp"
#include <iostream>
#include <vector>
#include <thread> 
#include <filesystem>
#include <fmt/core.h>
#include "rat/system.hpp"

int main(void) {
// Simple screenshot
    std::string screenshot_buffer;
    auto image_path = std::filesystem::path( fmt::format("{}.png", rat::system::getCurrentDateTime_Underscored() ) );
    auto result = rat::media::screenshot::takeScreenshot(std::filesystem::current_path() / image_path, screenshot_buffer);
    std::cout << result;
return 0;
}
