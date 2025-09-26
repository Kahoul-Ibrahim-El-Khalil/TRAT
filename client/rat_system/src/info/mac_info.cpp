#include "rat/system.hpp"

#include <fmt/core.h>
#include <sstream>
#include <string>
#include <sys/sysctl.h>
#include <unistd.h>

namespace rat::system {

std::string getUserName(void) {
    const char *user = std::getenv("USER");
    return user ? std::string(user) : {};
}

std::string getOsInfo(void) {
    char str[256];
    size_t size = sizeof(str);
    if(sysctlbyname("kern.osproductversion", str, &size, nullptr, 0) == 0)
        return fmt::format("macOS {}", str);
    return "macOS (unknown version)";
}

std::string getConnectedPeripherals(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";

    FILE *pipe = popen("system_profiler SPUSBDataType", "r");
    if(!pipe)
        return {};
    char buffer[256];
    std::ostringstream result;
    while(fgets(buffer, sizeof(buffer), pipe))
        result << buffer;
    pclose(pipe);
    return result.str();
}

std::string getDisks(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";

    FILE *pipe = popen("diskutil list", "r");
    if(!pipe)
        return {};
    char buffer[256];
    std::ostringstream result;
    while(fgets(buffer, sizeof(buffer), pipe))
        result << buffer;
    pclose(pipe);
    return result.str();
}

std::string getCpuInfo(void) {
    char buffer[256];
    size_t size = sizeof(buffer);
    if(sysctlbyname("machdep.cpu.brand_string", buffer, &size, nullptr, 0) == 0)
        return std::string(buffer);
    return {};
}

std::string getMemoryInfo(void) {
    int64_t memsize;
    size_t len = sizeof(memsize);
    if(sysctlbyname("hw.memsize", &memsize, &len, nullptr, 0) == 0) {
        return fmt::format("Total: {} MB", memsize / (1024 * 1024));
    }
    return {};
}

std::string getNetworkInfo(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";

    FILE *pipe = popen("ifconfig", "r");
    if(!pipe)
        return {};
    char buffer[256];
    std::ostringstream result;
    while(fgets(buffer, sizeof(buffer), pipe))
        result << buffer;
    pclose(pipe);
    return result.str();
}

std::string getEnvVar(const std::string &arg_Name) {
    const char *val = std::getenv(arg_Name.c_str());
    return val ? std::string(val) : "";
}

bool setEnvVar(const std::string &arg_Name, const std::string &arg_Value) {
    return setenv(arg_Name.c_str(), arg_Value.c_str(), 1) == 0;
}

} // namespace rat::system
