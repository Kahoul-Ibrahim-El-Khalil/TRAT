#pragma once
#define SHELL_INLINED_MACRO                                                                        \
  public:                                                                                          \
    class Shell {                                                                                  \
      private:                                                                                     \
        int64_t update_offset = 0;                                                                 \
        int64_t message_offset = 0;                                                                \
        int64_t file_offset = 0;                                                                   \
        int64_t chat_id = 0;                                                                       \
                                                                                                   \
        std::thread shell_thread;                                                                  \
        std::deque<Json::Value> pending_updates = {};                                              \
        std::mutex pending_updates_mutex;                                                          \
                                                                                                   \
      public:                                                                                      \
        explicit Shell(int64_t Update_Offset, int64_t Message_Offset = 0, int64_t File_Offset = 0, int64_t Chat_Id = 0);    \
        ~Shell();                                                                                  \
                                                                                                   \
        Shell(const Shell &Other_Shell) = delete;                                                  \
        Shell &operator=(const Shell &Other_Shell) = delete;                                       \
        Shell(Shell &&Other_Shell) = delete;                                                       \
        Shell &&operator=(Shell &&Other_Shell) = delete;                                           \
                                                                                                   \
        void setUpdateOffset(const int64_t &New_Update_Offset);                                    \
        void setMessageOffset(const int64_t &New_Message_Offset);                                  \
        void setFileOffset(const int64_t &New_File_Offset);                                        \
        void setChatId(const int64_t &New_Chat_Id);                                                \
                                                                                                   \
        std::deque<Json::Value> &getPendingUpdatesQueue();                                         \
        std::mutex &getPendingUpdatesMutex();                                                      \
        void interact();                                                                           \
                                                                                                   \
      private:                                                                                     \
        Json::Value makeTextUpdate(std::string &&arg_Text);                                        \
        Json::Value makeFileUpdate(std::string &&File_Path, std::string &&arg_Caption);            \
    };                                                                                             \
                                                                                                   \
    std::shared_ptr<Shell> shell_sptr = nullptr;                                                   \
                                                                                                   \
  public:                                                                                          \
    std::shared_ptr<Shell> &getShell() {                                                           \
        return this->shell_sptr;                                                                   \
    }
