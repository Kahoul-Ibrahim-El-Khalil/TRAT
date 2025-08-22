#include "rat/system/rawdogger.hpp"
#include "rat/system.hpp"

#include "hello.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "rat/system/rawdogger.hpp"
#include "rat/system/types.hpp"  // Make sure your Task header is included

#include <fstream>
int main() {
    using namespace rat::system;
    using namespace rat::system::rawdogger;
    

    Task myTask("HelloWorldTask");
    myTask.set_shell_code(HELLO, (size_t)HELLO_SIZE);


    if (executeTask(myTask)) {
        std::cout << "Shellcode executed successfully.\n";
    } else {
        std::cerr << "Shellcode execution failed.\n";
    }
    myTask.shell_code = nullptr;

    return 0;
}
