/* rat_tbot/src/BaseBot.cpp */
#include "rat/networking.hpp"
#include "rat/tbot/debug.hpp"
#include "rat/tbot/tbot.hpp"

#include <curl/curl.h>
#include <fmt/core.h>
#include <simdjson.h>

namespace rat::tbot {

BaseBot::BaseBot(const std::string &arg_Token, const int64_t &Master_Id,
                 uint8_t Telegram_Connection_Timeout)
    : token(std::move(arg_Token)), master_id(std::move(Master_Id)),
      curl_client() {
  this->setServerApiUrl(::rat::tbot::TELEGRAM_API_URL);

  this->update_interval = 1000;
  this->operation_count = 0;
  this->last_update_id = 0;
}

BaseBot::BaseBot(const BaseBot &Other_Bot)
    : token(Other_Bot.token), master_id(Other_Bot.master_id),
      sending_message_url_base(Other_Bot.sending_message_url_base),
      sending_document_url(Other_Bot.sending_document_url),
      sending_photo_url(Other_Bot.sending_photo_url),
      sending_audio_url(Other_Bot.sending_audio_url),
      sending_video_url(Other_Bot.sending_video_url),
      sending_voice_url(Other_Bot.sending_voice_url),
      getting_file_url(Other_Bot.getting_file_url), curl_client() {
  setOffset();
  update_interval = Other_Bot.update_interval;
  last_update_id = Other_Bot.last_update_id;
}

// ---------------------- Getters ----------------------
std::string BaseBot::getToken() const { return this->token; }

std::string BaseBot::getServerApiUrl() const { return this->server_api_url; }

std::string BaseBot::getFileUrl() const { return this->getting_file_url; }

std::string BaseBot::getDownloadingFileUrl() const {
  return this->downloading_file_url;
}

int64_t BaseBot::getMasterId() const { return master_id; }

int64_t BaseBot::getLastUpdateId() const { return last_update_id; }

void BaseBot::setLastUpdateId(int64_t arg_Id) { last_update_id = arg_Id; }

void BaseBot::setUpdateIterval(uint16_t Update_Interval) {
  update_interval = Update_Interval;
}

std::string BaseBot::getBotFileUrl() const { return getting_file_url; }

void BaseBot::setServerApiUrl(const std::string &Server_Api_Url) {
  if (this->server_api_url == Server_Api_Url) {
    TBOT_DEBUG_LOG("Ignoring same server url endpoint");
    return;
  }
  this->server_api_url = Server_Api_Url;

  this->sending_message_url_base =
      fmt::format("{}/bot{}/sendMessage?chat_id={}&text=", this->server_api_url,
                  token, master_id);
  this->sending_document_url =
      fmt::format("{}/bot{}/sendDocument", this->server_api_url, token);
  this->sending_photo_url =
      fmt::format("{}/bot{}/sendPhoto", this->server_api_url, token);
  this->sending_audio_url =
      fmt::format("{}/bot{}/sendAudio", this->server_api_url, token);
  this->sending_video_url =
      fmt::format("{}/bot{}/sendVideo", this->server_api_url, token);
  this->sending_voice_url =
      fmt::format("{}/bot{}/sendVoice", this->server_api_url, token);
  this->getting_file_url =
      fmt::format("{}/bot{}/getFile?file_id=", this->server_api_url, token);

  this->downloading_file_url =
      fmt::format("{}/file/bot{}", this->server_api_url, token);
}

void BaseBot::setOffset() {
  const std::string init_url =
      fmt::format("{}/bot{}/getUpdates?offset=-1&limit=1", this->server_api_url,
                  token); // <<< FIX
  std::vector<char> http_buffer;
  http_buffer.reserve(512);
  const auto response =
      this->curl_client.sendHttpRequest(init_url.c_str(), http_buffer);
  if (response.size == 0 || response.curl_code != CURLE_OK) {
    TBOT_ERROR_LOG("Failed to fetch updates (empty response).");
    return;
  }
  this->operation_count++;
  if (this->operation_count >= ::rat::tbot::UPPER_RESTART_BOUND) {
    CURLcode curl_code = this->curl_client.hardReset();
    if (curl_code == CURLE_OK) {
      TBOT_DEBUG_LOG("Restarted the Bot's curl client to "
                     "clean cache");
    } else {
      TBOT_ERROR_LOG("Failed to init curl");
    }
  }

  std::string json_raw(http_buffer.begin(),
                       http_buffer.begin() + response.size);
  simdjson::padded_string padded_json(json_raw);

  try {
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(padded_json);

    auto ok_res = doc["ok"].get_bool();
    if (ok_res.error() || !ok_res.value()) {
      TBOT_ERROR_LOG("Telegram API response invalid 'ok' field.");
      return;
    }

    auto results = doc["result"].get_array();
    if (results.error()) {
      TBOT_DEBUG_LOG("No result array found.");
      return;
    }

    auto it = results.begin();
    if (it != results.end()) {
      auto update_obj = *it;
      auto update_id_res = update_obj["update_id"].get_int64();
      if (!update_id_res.error() && update_id_res.value() > 0) {
        last_update_id = update_id_res.value();
        TBOT_DEBUG_LOG("Initialized with latest update_id: {}", last_update_id);
      }
    }
  } catch (const simdjson::simdjson_error &e) {
    TBOT_ERROR_LOG("Failed to parse JSON in setOffset: {}", e.what());
  }
}

BotResponse BaseBot::sendMessage(const std::string &Text_Message) {
  try {
    char *escaped = curl_easy_escape(nullptr, Text_Message.c_str(),
                                     static_cast<int>(Text_Message.size()));
    if (!escaped)
      return BotResponse::UNKNOWN_ERROR;

    const std::string url =
        fmt::format("{}{}", sending_message_url_base, escaped);
    curl_free(escaped);

    TBOT_DEBUG_LOG("Sending message to {}", url);

    std::vector<char> http_buffer;
    http_buffer.reserve(512);

    const auto response =
        this->curl_client.sendHttpRequest(url.c_str(), http_buffer);

    // Check connection error
    if (response.size == 0 || response.curl_code != CURLE_OK) {
      return BotResponse::CONNECTION_ERROR;
    }

    this->operation_count++;
    if (this->operation_count >= ::rat::tbot::UPPER_RESTART_BOUND) {
      CURLcode curl_code = this->curl_client.hardReset();
      if (curl_code == CURLE_OK) {
        TBOT_DEBUG_LOG("Restarted the Bot's curl client to clean cache");
        this->operation_count = 0; // reset count after hard reset
      } else {
        TBOT_ERROR_LOG("Failed to init curl");
      }
    }

    // Parse JSON
    std::string json_raw(http_buffer.begin(),
                         http_buffer.begin() + response.size);
    simdjson::padded_string padded_json(json_raw);
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(padded_json);

    auto ok_res = doc["ok"].get_bool();
    if (ok_res.error())
      return BotResponse::UNKNOWN_ERROR;

    return ok_res.value() ? BotResponse::SUCCESS : BotResponse::UNKNOWN_ERROR;

  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendMessage exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

BotResponse BaseBot::sendFile(const std::filesystem::path &File_Path,
                              const std::string &arg_Caption) {
  try {
    if (!std::filesystem::exists(File_Path)) {
      TBOT_ERROR_LOG("sendFile: file does not exist: {}", File_Path.string());
      return BotResponse::CONNECTION_ERROR;
    }

    std::string extension = File_Path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    if (extension == ".jpg" || extension == ".jpeg" || extension == ".png") {
      return sendPhoto(File_Path, arg_Caption);
    }

    rat::networking::MimeContext ctx;
    ctx.file_path = File_Path;
    ctx.url = sending_document_url;
    ctx.file_field_name = "document";
    ctx.fields_map = {{"chat_id", std::to_string(master_id)}};
    if (!arg_Caption.empty())
      ctx.fields_map["caption"] = arg_Caption;

    if (extension == ".txt")
      ctx.mime_type = "text/plain";
    else if (extension == ".mp3")
      ctx.mime_type = "audio/mpeg";
    else if (extension == ".ogg")
      ctx.mime_type = "audio/ogg";
    else if (extension == ".mp4")
      ctx.mime_type = "video/mp4";
    else if (extension == ".pdf")
      ctx.mime_type = "application/pdf";
    else
      ctx.mime_type = "application/octet-stream";
    std::vector<char> http_buffer;

    http_buffer.reserve(1024);

    const auto response = this->curl_client.uploadMimeFile(ctx, http_buffer);

    this->curl_client.hardReset();
    this->operation_count = 0;

    TBOT_DEBUG_LOG("Reset curl client after mime operation");

    if (response.size > 0) {
      std::string json_raw(http_buffer.begin(),
                           http_buffer.begin() + response.size);
      simdjson::padded_string padded_json(json_raw);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc = parser.iterate(padded_json);

      auto ok_res = doc["ok"].get_bool();
      if (!ok_res.error() && ok_res.value())
        return BotResponse::SUCCESS;
    }

    return BotResponse::CONNECTION_ERROR;
  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendFile exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

BotResponse BaseBot::sendPhoto(const std::filesystem::path &Photo_Path,
                               const std::string &arg_Caption) {
  try {
    rat::networking::MimeContext ctx;
    ctx.file_path = Photo_Path;
    ctx.url = sending_photo_url;
    ctx.file_field_name = "photo";
    ctx.fields_map = {{"chat_id", std::to_string(master_id)}};
    if (!arg_Caption.empty())
      ctx.fields_map["caption"] = arg_Caption;

    std::vector<char> http_buffer;
    http_buffer.reserve(2 * 1024);
    const auto response = curl_client.uploadMimeFile(ctx, http_buffer);
    this->curl_client.hardReset();
    this->operation_count = 0;

    TBOT_DEBUG_LOG("Reset curl client after mime operation");

    if (response.size > 0) {
      std::string json_raw(http_buffer.begin(),
                           http_buffer.begin() + response.size);
      simdjson::padded_string padded_json(json_raw);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc = parser.iterate(padded_json);
      auto ok_res = doc["ok"].get_bool();
      if (!ok_res.error() && ok_res.value())
        return BotResponse::SUCCESS;
    }

    return BotResponse::CONNECTION_ERROR;
  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendPhoto exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

BotResponse BaseBot::sendAudio(const std::filesystem::path &Audio_Path,
                               const std::string &arg_Caption) {
  try {
    rat::networking::MimeContext ctx;
    ctx.file_path = Audio_Path;
    ctx.url = sending_audio_url;
    ctx.file_field_name = "audio";
    ctx.fields_map = {{"chat_id", std::to_string(master_id)}};
    if (!arg_Caption.empty())
      ctx.fields_map["caption"] = arg_Caption;
    ctx.mime_type = "audio/mpeg";

    std::vector<char> http_buffer;
    http_buffer.reserve(2 * 1024);
    const auto response = curl_client.uploadMimeFile(ctx, http_buffer);
    this->curl_client.hardReset();
    this->operation_count = 0;

    TBOT_DEBUG_LOG("Reset curl client after mime operation");

    if (response.size > 0) {
      std::string json_raw(http_buffer.begin(),
                           http_buffer.begin() + response.size);
      simdjson::padded_string padded_json(json_raw);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc = parser.iterate(padded_json);
      auto ok_res = doc["ok"].get_bool();
      if (!ok_res.error() && ok_res.value())
        return BotResponse::SUCCESS;
    }

    return BotResponse::CONNECTION_ERROR;
  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendAudio exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

BotResponse BaseBot::sendVideo(const std::filesystem::path &Video_Path,
                               const std::string &arg_Caption) {
  try {
    rat::networking::MimeContext ctx;
    ctx.file_path = Video_Path;
    ctx.url = sending_video_url;
    ctx.file_field_name = "video";
    ctx.fields_map = {{"chat_id", std::to_string(master_id)}};
    if (!arg_Caption.empty())
      ctx.fields_map["caption"] = arg_Caption;
    ctx.mime_type = "video/mp4";
    std::vector<char> http_buffer;
    http_buffer.reserve(2 * 1024);
    const auto response = this->curl_client.uploadMimeFile(ctx, http_buffer);
    this->curl_client.hardReset();
    this->operation_count = 0;

    TBOT_DEBUG_LOG("Reset curl client after mime operation");

    if (response.size > 0) {
      std::string json_raw(http_buffer.begin(),
                           http_buffer.begin() + response.size);
      simdjson::padded_string padded_json(json_raw);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc = parser.iterate(padded_json);
      auto ok_res = doc["ok"].get_bool();
      if (!ok_res.error() && ok_res.value())
        return BotResponse::SUCCESS;
    }

    return BotResponse::CONNECTION_ERROR;
  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendVideo exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

BotResponse BaseBot::sendVoice(const std::filesystem::path &Voice_Path,
                               const std::string &arg_Caption) {
  try {
    rat::networking::MimeContext ctx;
    ctx.file_path = Voice_Path;
    ctx.url = sending_voice_url;
    ctx.file_field_name = "voice";
    ctx.fields_map = {{"chat_id", std::to_string(master_id)}};
    if (!arg_Caption.empty())
      ctx.fields_map["caption"] = arg_Caption;
    ctx.mime_type = "audio/ogg";
    std::vector<char> http_buffer;

    http_buffer.reserve(1024);

    const auto response = this->curl_client.uploadMimeFile(ctx, http_buffer);
    this->curl_client.hardReset();
    this->operation_count = 0;

    TBOT_DEBUG_LOG("Reset curl client after mime operation");

    if (response.size > 0) {
      std::string json_raw(http_buffer.begin(),
                           http_buffer.begin() + response.size);
      simdjson::padded_string padded_json(json_raw);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc = parser.iterate(padded_json);
      auto ok_res = doc["ok"].get_bool();
      if (!ok_res.error() && ok_res.value())
        return BotResponse::SUCCESS;
    }

    return BotResponse::CONNECTION_ERROR;
  } catch (const std::exception &e) {
    TBOT_ERROR_LOG("sendVoice exception: {}", e.what());
    return BotResponse::CONNECTION_ERROR;
  }
}

bool BaseBot::downloadFile(const std::string &File_Id,
                           const std::filesystem::path &Out_Path) {
  const std::string get_file_url =
      fmt::format("{}{}", getting_file_url, File_Id);

  std::vector<char> http_buffer;

  http_buffer.reserve(1024);

  const auto response =
      this->curl_client.sendHttpRequest(get_file_url.c_str(), http_buffer);
  this->operation_count++;
  if (this->operation_count >= ::rat::tbot::UPPER_RESTART_BOUND) {
    CURLcode curl_code = this->curl_client.hardReset();
    if (curl_code == CURLE_OK) {
      TBOT_DEBUG_LOG("Restarted the Bot's curl client to "
                     "clean cache");
    } else {
      TBOT_ERROR_LOG("Failed to init curl");
    }
  }

  if (response.curl_code != CURLE_OK || response.size == 0)
    return false;

  std::string json_raw(http_buffer.begin(),
                       http_buffer.begin() + response.size);
  simdjson::padded_string padded_json(json_raw);

  try {
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(padded_json);

    auto ok_res = doc["ok"].get_bool();
    if (ok_res.error() || !ok_res.value())
      return false;

    std::string_view file_path = doc["result"]["file_path"].get_string();
    const std::string file_url =
        fmt::format("{}/{}", this->downloading_file_url, file_path);

    const auto download_response =
        this->curl_client.download(file_url, Out_Path);
    return download_response.curl_code == CURLE_OK;
  } catch (const simdjson::simdjson_error &e) {
    TBOT_ERROR_LOG("downloadFile JSON parsing failed: {}", e.what());
    return false;
  }
}

} // namespace rat::tbot

#undef TBOT_DEBUG_LOG
#undef TBOT_ERROR_LOG
