#include "rat/system.hpp"

#include <chrono>
#include <ctime>
#include <string>

namespace rat::system {

// -------------------------------
// Helpers
// -------------------------------
static std::tm getLocalTime(std::time_t time) {
#ifdef _WIN32
	std::tm tm_buffer{};
	localtime_s(&tm_buffer, &time);
	return tm_buffer;
#else
	std::tm tm_buffer{};
	localtime_r(&time, &tm_buffer);
	return tm_buffer;
#endif
}

static int getTimezoneOffsetMinutes(std::time_t time) {
#ifdef _WIN32
	long timezone_sec = 0;
	_get_timezone(&timezone_sec);
	return static_cast<int>(-timezone_sec / 60);
#else
	std::tm local_tm{}, utc_tm{};
	localtime_r(&time, &local_tm);
	gmtime_r(&time, &utc_tm);

	int local_minutes = local_tm.tm_hour * 60 + local_tm.tm_min;
	int utc_minutes = utc_tm.tm_hour * 60 + utc_tm.tm_min;
	int offset = local_minutes - utc_minutes;

	if(offset > 720)
		offset -= 1440;
	if(offset < -720)
		offset += 1440;

	return offset;
#endif
}

// -------------------------------
// Functions
// -------------------------------
std::string getCurrentDateTime(void) {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto now_time = system_clock::to_time_t(now);
	auto tm_buffer = getLocalTime(now_time);

	int tz_offset_min = getTimezoneOffsetMinutes(now_time);
	int tz_hour = tz_offset_min / 60;
	int tz_min = std::abs(tz_offset_min % 60);

	return fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}{:+03}:{:02}",
	                   tm_buffer.tm_year + 1900, tm_buffer.tm_mon + 1,
	                   tm_buffer.tm_mday, tm_buffer.tm_hour, tm_buffer.tm_min,
	                   tm_buffer.tm_sec, tz_hour, tz_min);
}

std::string getCurrentDateTime_Underscored(void) {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto now_time = system_clock::to_time_t(now);
	auto tm_buffer = getLocalTime(now_time);

	return fmt::format("{:04}{:02}{:02}{:02}{:02}{:02}",
	                   tm_buffer.tm_year + 1900, tm_buffer.tm_mon + 1,
	                   tm_buffer.tm_mday, tm_buffer.tm_hour, tm_buffer.tm_min,
	                   tm_buffer.tm_sec);
}

std::string getCurrentTime(void) {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto now_time = system_clock::to_time_t(now);
	auto tm_buffer = getLocalTime(now_time);

	return fmt::format("{:02}:{:02}:{:02}", tm_buffer.tm_hour, tm_buffer.tm_min,
	                   tm_buffer.tm_sec);
}

std::string getCurrentDate(void) {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto now_time = system_clock::to_time_t(now);
	auto tm_buffer = getLocalTime(now_time);

	return fmt::format("{:04}-{:02}-{:02}", tm_buffer.tm_year + 1900,
	                   tm_buffer.tm_mon + 1, tm_buffer.tm_mday);
}

} // namespace rat::system
