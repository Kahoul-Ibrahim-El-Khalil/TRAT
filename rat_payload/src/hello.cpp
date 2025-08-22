#include <stdio.h>
#include <filesystem>
int main(void) {
    std::filesystem::create_directory("empty");
    return 0;
}
