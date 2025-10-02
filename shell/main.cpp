/* main.cpp */
#include "Shell.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if(argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <update_offset> <message_offset> <chat_id>\n";
        return EXIT_FAILURE;
    }

    int64_t update_offset = 0;
    int64_t message_offset = 0;
    int64_t chat_id = 0;

    try {
        update_offset = std::stoll(argv[1]);
        message_offset = std::stoll(argv[2]);
        chat_id = std::stoll(argv[3]);
    } catch(const std::exception &e) {
        std::cerr << "Invalid numeric argument: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    Shell shell(update_offset, message_offset, chat_id);
    shell.interact();

    return EXIT_SUCCESS;
}
