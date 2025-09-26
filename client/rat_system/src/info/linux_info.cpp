#include "rat/system.hpp"

#include <sys/utsname.h>
#include <unistd.h>

namespace rat::system {

std::string getUserName(void) {
    const char *user = std::getenv("USER");
    return user ? std::string(user) : "";
}

std::string getOsInfo(void) {
    struct utsname buffer;
    if(uname(&buffer) == 0)
        return fmt::format("{} {} {}", buffer.sysname, buffer.release, buffer.version);
    return "Linux (unknown version)";
}

std::string getConnectedPeripherals(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";

    FILE *pipe = popen("lsusb", "r");
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

    FILE *pipe = popen("lsblk -o NAME,SIZE,FSTYPE,MOUNTPOINT", "r");
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
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while(std::getline(cpuinfo, line)) {
        if(line.find("model name") != std::string::npos)
            return line.substr(line.find(':') + 2);
    }
    return {};
}

std::string getMemoryInfo(void) {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    std::ostringstream result;
    while(std::getline(meminfo, line)) {
        if(line.find("MemTotal") != std::string::npos ||
           line.find("MemAvailable") != std::string::npos) {
            result << line << '\n';
        }
    }
    return result.str();
}

std::string getNetworkInfo(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";

    FILE *pipe = popen("ip addr show", "r");
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
