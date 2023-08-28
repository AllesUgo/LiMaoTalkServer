#pragma once

#include <string>
#include <exception>
#include <shared_mutex>
#include "Storage.h"
namespace LiMao
{
	namespace Config
	{
		class ConfigException :public std::exception
		{
		private:
			std::string reason;
		public:
			ConfigException(const std::string& reason)noexcept;
			const char* what(void)const noexcept override;
		};
		class ConfigManager
		{
		public:
			static void SetConfigPath(const std::string& str);
			static void ReadConfig(void);
			static int SaveConfig(void);
			static std::string LocalAddress(void);
			static void LocalAddress(const std::string& ip);
			static int LocalPort(void);
			static void LocalPort(int port);
			static std::string UidPoolDir(void);
			static void UidPoolDir(const std::string& path);
			static std::string CoreLogPath(void);
			static void CoreLogPath(const std::string& path);
			static void UserDataPath(const std::string& path);
			static std::string UserDataPath(void);
		};
		class GlobalFileRWLock
		{
		public:
			static std::shared_ptr<std::shared_mutex> GetMutex(const std::string& path);
		};;
	}
}
