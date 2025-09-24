#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace DragonRatServer {
namespace TelegramBotApi {

class Handler {
  private:
	int64_t chat_id{};
	std::string token;
	std::string associated_db_path;
	bool is_main{false};
	std::string backing_bot_token;

	// Consider shared_ptr if multiple handlers might share the mutex
	std::shared_ptr<std::mutex> p_associated_db_mutex{nullptr};

  public:
	explicit Handler() = default;
	Handler(const Handler &) = delete;
	Handler(Handler &&) = delete;

	// Fluent setters
	Handler &setToken(const std::string &arg_Token);
	Handler &setToken(std::string &&arg_Token);

	Handler &setChatId(int64_t Chat_Id);

	Handler &setAssociatedDb(const std::string &dbPath, std::mutex &dbMutex);
	Handler &setAssociatedDb(std::string &&dbPath, std::mutex &dbMutex);

	Handler &setBackingBot(const std::string &Backing_Bot_Token);

	// Getters (return by value for safety)
	std::string getToken() const;
	std::string getAssociatedDbPath() const;
	int64_t getChatId() const;

	std::shared_ptr<std::mutex> &getAssociatedDbMutex() const;

	// Actions
	void handleReceivedMessage();
	void handleReceivedFile();
	void handleSetOffset();

	void handle(std::string &&Url_Path);
};

} // namespace TelegramBotApi
} // namespace DragonRatServer

