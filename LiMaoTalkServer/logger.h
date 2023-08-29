#pragma once
#include "configuration_manager.h"
namespace LiMao
{
	namespace Service
	{
		class Logger
		{
		private:
			Logger() = delete;
			static void print_log(int level, const char* log_format,va_list lst);
		public:
			static void LogInfo(const char*format,...);
			static void LogWarn(const char* format, ...);
			static void LogError(const char* format, ...);
		};
		class Time
		{
		public:
			static std::string GetFormattedTime(void)noexcept;
			static std::string ConvertTimeStampToFormattedTime(time_t time_stamp)noexcept;
		};
	}
}