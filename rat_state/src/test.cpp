#include "rat/RatState.hpp"
#include <iostream>

int main() {
    rat::RatState state;

    std::cout << "Host OS: ";
    if (state.host_system == rat::HostOperatingSystem::WINDOWS) std::cout << "Windows\n";
    else std::cout << "Linux\n";

    // -------------------
    // Test dynamic commands
    // -------------------
    state.addCommand("hello", "echo Hello World");
    state.addCommand("list", "ls -la");
    std::cout << "\nDynamic Commands:\n";
    for (auto& cmd : state.listDynamicCommands()) {
        std::cout << "  " << cmd << " -> " << state.getCommand(cmd) << "\n";
    }

    std::cout << "Resetting commands...\n";
    state.resetCommands();
    std::cout << "Commands after reset: " << state.listDynamicCommands().size() << "\n";

    // -------------------
    // Test dynamic globals
    // -------------------
    state.setGlobal("user", "admin");
    state.setGlobal("mode", "debug");
    std::cout << "\nDynamic Globals:\n";
    for (auto& key : state.listDynamicGlobals()) {
        std::cout << "  " << key << " = " << state.getGlobal(key) << "\n";
    }

    std::cout << "Resetting globals...\n";
    state.resetGlobals();
    std::cout << "Globals after reset: " << state.listDynamicGlobals().size() << "\n";

    // -------------------
    // Test system utilities
    // -------------------
    std::cout << "\nScanning system PATH for utilities...\n";
    state.scanSystemPathsForUtilities();
    state.detectCommonUtilities(); // optional

    auto tools = state.listDynamicTools();
    std::cout << "Detected utilities: " << tools.size() << "\n";
    for (const auto& tool : tools) {
        std::cout << "  " << tool << " -> " << state.getUtilityPath(tool) << "\n";
    }

    // -------------------
    // Test adding a custom tool dir (if you have one)
    // -------------------
    // state.addCustomToolDir("/usr/local/bin");

    return 0;
}
