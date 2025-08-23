#include "rat/tbot/handler/handleUpdate.hpp"
#include "rat/system.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>
#include <sstream>
#include <fmt/core.h>
#include <fmt/chrono.h>

namespace rat::tbot::handler {
  
static inline void __normalizeWhiteSpaces(std::string& String_Input) {
    boost::trim(String_Input);  // Trim whitespace
    boost::replace_all(String_Input, "  ", " ");  // Replace double spaces
}
static inline size_t __countWhiteSpaces(const std::string& String_Input) {
    size_t whitespace_count = 0;
    for(size_t i = 0; i < String_Input.size(); ++i) {
        if(String_Input[i] == ' ')  ++whitespace_count;
    }
    return whitespace_count;
}
/*Assumes that Telegram_Message*/
Command _parseTelegramMessageToCommand(const Message& Telegram_Message) {
    std::string message_text = Telegram_Message.text;
    __normalizeWhiteSpaces(message_text);
    size_t space_count = __countWhiteSpaces(message_text);

    std::istringstream iss(std::move(message_text));
    std::string directive;
    std::vector<std::string> parameters;
    parameters.reserve(space_count);

    iss >> directive;
    std::string param;
    while (iss >> param) {
        parameters.emplace_back(std::move(param));
    }

    return Command(std::move(directive), std::move(parameters));
}
static inline size_t __findFirstOccurenceOfChar(const std::string& Input_String, const char arg_C) {
  size_t pos = 0; 
  for(size_t i = 0; i < Input_String.size(); ++i) {
      if(Input_String[i] == arg_C) {
          pos = i;
          break;
      }
  }
  return pos;
}

std::string _parseAndHandleShellCommand(Message& Telegram_Message) {
    Telegram_Message.text.erase(0, 4); //erase / s h and space;
    
    boost::trim(Telegram_Message.text);
    
    size_t space_char_pos = __findFirstOccurenceOfChar(Telegram_Message.text, ' ');
    const std::string timeout_string = Telegram_Message.text.substr(0, space_char_pos);
    
    Telegram_Message.text.erase(0, space_char_pos);
    
    boost::trim(Telegram_Message.text);
    
    const std::string& command = Telegram_Message.text;
    
    int timeout = 0;
    
    try {
        timeout = std::stoi(timeout_string);
    }
    catch (const std::exception&) {
        return fmt::format("Invalid timeout value: {}", timeout_string);
    }

    return rat::system::runShellCommand(command, static_cast<unsigned int>(timeout) );
}

std::string _parseAndHandleProcessCommand(Bot& arg_Bot, Message& Telegram_Message) {
    Telegram_Message.text.erase(0, 9); //erase / s h and space;
    
    boost::trim(Telegram_Message.text);
    
    size_t space_char_pos = __findFirstOccurenceOfChar(Telegram_Message.text, ' ');
    const std::string timeout_string = Telegram_Message.text.substr(0, space_char_pos);
    
    Telegram_Message.text.erase(0, space_char_pos);
    
    boost::trim(Telegram_Message.text);
    
    const std::string& command = Telegram_Message.text;
    
    int timeout = 0;
    
    try {
        timeout = std::stoi(timeout_string);
    }
    catch (const std::exception&) {
        return fmt::format("Invalid timeout value: {}", timeout_string);
    }
    
    return rat::system::runProcess(
        command, 
        timeout, 
        [bot_copy = arg_Bot](rat::system::ProcessResult res) mutable {
            std::string feedback;
            if (res.exit_code == -1) {
                feedback = "Process killed (timeout exceeded).";
            } else {
                feedback = fmt::format(
                    "Process exited with code {}\nSTDOUT:\n{}\nSTDERR:\n{}",
                    res.exit_code,
                    res.stdout_str,
                    res.stderr_str
                );
            }
            bot_copy.sendMessage(feedback);
        }
    );
}
}//namespace rat::tbot::handler
