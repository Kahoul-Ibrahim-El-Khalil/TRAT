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

    return rat::system::runCommand(command, static_cast<uint32_t>(timeout) );
}

std::string _parseAndHandleProcessCommand(const Bot& bot, Message& Telegram_Message) {
    Telegram_Message.text.erase(0, 9); // trim the start of /process
    boost::trim(Telegram_Message.text); // trim extra whitespaces at edges

    const std::string& command = Telegram_Message.text;
    auto start_time = std::chrono::system_clock::now();

    rat::system::TinyTask tiny_task;
    tiny_task.command = command;
    tiny_task.parameters = {};
    tiny_task.timeout_ms = 0;
    auto end_time = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();


    std::string response = fmt::format(
                "Process finished.\n"
                "Start: {:%Y-%m-%d %H:%M:%S}\n"
                "End: {:%Y-%m-%d %H:%M:%S}\n"
                "Duration: {} ms\n",
                start_time,  // directly works with chrono::time_point
                end_time,
                ms
            );
            bot.sendMessage(response);

    bool success = rat::system::runSeparateProcess(tiny_task);
    return fmt::format(
        "Process start {}: {}, response_buffer: {}",
        success ? "successful" : "failed",
        command,
        tiny_task.output_buffer
    );
}

std::string _parseAndHandleSystemCommand(const Bot& arg_Bot, Message& Telegram_Message) {
    Telegram_Message.text.erase(0, 5); //erase / s h and space;
    
    boost::trim(Telegram_Message.text);
    
    size_t space_char_pos = __findFirstOccurenceOfChar(Telegram_Message.text, ' ');
    
    Telegram_Message.text.erase(0, space_char_pos);
    
    boost::trim(Telegram_Message.text);
    
    const std::string& command = Telegram_Message.text;
    
    std::string output_buffer;
    std::string error_buffer;
    rat::system::runSystemCommand(command, output_buffer, error_buffer);
    return fmt::format("Output:{}\nError:{}",  output_buffer, error_buffer);

}



}//namespace rat::tbot::handler
