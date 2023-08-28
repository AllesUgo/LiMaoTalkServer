#include "logger.h"
#include <time.h>
#include <cstdarg>
void LiMao::Service::Logger::print_log(int level, const char* log_format, va_list lst)
{
	switch (level)
	{
	case 3:
	case 2:
	case 1:
	case 0:
		vprintf(log_format, lst);
	default:
		break;
	}
}
void LiMao::Service::Logger::LogInfo(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [INFO] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	LiMao::Service::Logger::print_log(0, (std::string(tmp) + format+"\n").c_str(), lst);
	va_end(lst);

}

void LiMao::Service::Logger::LogWarn(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [Warn] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	LiMao::Service::Logger::print_log(1, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

void LiMao::Service::Logger::LogError(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [ERROR] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	LiMao::Service::Logger::print_log(2, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

std::string LiMao::Service::Time::GetFormattedTime(void) noexcept
{
	return LiMao::Service::Time::ConvertTimeStampToFormattedTime(time(0));
}

std::string LiMao::Service::Time::ConvertTimeStampToFormattedTime(time_t time_stamp) noexcept
{
	char tmp[64];
	struct tm* timinfo;
	timinfo = localtime(&time_stamp);

	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H-%M-%S", timinfo);
	return tmp;
}
