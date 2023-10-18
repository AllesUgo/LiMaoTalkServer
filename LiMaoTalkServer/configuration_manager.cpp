#include "configuration_manager.h"
#include "FileIO.h"
#include "CJsonObject.h"
#include "file_rw_lock.h"

namespace cfg = LiMao::Config;
static LiMao::Service::FileRWLock::FileRWLock rw_lock;
static RbsLib::Storage::StorageFile config_file_path;
static neb::CJsonObject config;
void LiMao::Config::ConfigManager::SetConfigPath(const std::string& str)
{
	config_file_path = str;
}

void LiMao::Config::ConfigManager::ReadConfig(void)
{
	if (!config_file_path.IsExist())
	{
		throw cfg::ConfigException("Configuration file not exist");
	}
	size_t file_size = config_file_path.GetFileSize();
	if (!file_size) throw cfg::ConfigException("Configuration file is empty");
	RbsLib::Storage::FileIO::File f(config_file_path.Path());
	auto buf = f.Read(file_size);
	char* data = new char[buf.GetLength() + 1];
	memcpy(data, buf.Data(), buf.GetLength());
	data[buf.GetLength()] = 0;
	config.Clear();
	if (!config.Parse(data))
	{
		delete[]data;
		throw cfg::ConfigException(std::string("Configuration file format error:") + config.GetErrMsg());
	}
	delete[]data;
}

int LiMao::Config::ConfigManager::SaveConfig(void)
{
	RbsLib::Storage::FileIO::File fp(config_file_path.Path(),
		RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Replace);
	fp.Write(RbsLib::Buffer(config.ToFormattedString()));
	return 0;
}

std::string LiMao::Config::ConfigManager::LocalAddress(void)
{
	if (!config.KeyExist("LocalAddress") || config.GetValueType("LocalAddress") != cJSON_String)
		throw cfg::ConfigException("Configration file key \"LocalAddress\" error");
	return config("LocalAddress");
}

void LiMao::Config::ConfigManager::LocalAddress(const std::string& ip)
{
	config.ReplaceAdd("LocalAddress", ip);
}

int LiMao::Config::ConfigManager::LocalPort(void)
{
	if (!config.KeyExist("LocalPort") || config.GetValueType("LocalPort") != cJSON_Int)
		throw cfg::ConfigException("Configration file key \"LocalPort\" error");
	int n;
	config.Get("LocalPort", n);
	return n;
}

void LiMao::Config::ConfigManager::LocalPort(int port)
{
	config.ReplaceAdd("LocalPort", port);
}

std::string LiMao::Config::ConfigManager::UidPoolDir(void)
{
	if (!config.KeyExist("UidPoolDir") || config.GetValueType("UidPoolDir") != cJSON_String)
		throw cfg::ConfigException("Configration file key \"UidPoolDir\" error");
	return config("UidPoolDir");
}

void LiMao::Config::ConfigManager::UidPoolDir(const std::string& path)
{
	config.ReplaceAdd("UidPoolDir", path);
}

std::string LiMao::Config::ConfigManager::CoreLogPath(void)
{
	if (!config.KeyExist("CoreLogPath") || config.GetValueType("CoreLogPath") != cJSON_String)
		throw cfg::ConfigException("Configration file key \"CoreLogPath\" error");
	return config("CoreLogPath");
}

void LiMao::Config::ConfigManager::CoreLogPath(const std::string& path)
{
	config.ReplaceAdd("CoreLogPath", path);
}

void LiMao::Config::ConfigManager::UserDataPath(const std::string& path)
{
	config.ReplaceAdd("UserDataPath", path);
}

std::string LiMao::Config::ConfigManager::UserDataPath(void)
{
	if (!config.KeyExist("UserDataPath") || config.GetValueType("UserDataPath") != cJSON_String)
		throw cfg::ConfigException("Configration file key \"UserDataPath\" error");
	return config("UserDataPath");
}

void LiMao::Config::ConfigManager::UserMessageDatabasePath(const std::string& path)
{
	config.ReplaceAdd("UserMessageDatabasePath", path);
}

std::string LiMao::Config::ConfigManager::UserMessageDatabasePath(void)
{
	if (!config.KeyExist("UserMessageDatabasePath") || config.GetValueType("UserMessageDatabasePath") != cJSON_String)
		throw cfg::ConfigException("Configration file key \"UserMessageDatabasePath\" error");
	return config("UserMessageDatabasePath");
}

LiMao::Config::ConfigException::ConfigException(const std::string& reason) noexcept :reason(reason) {}

const char* LiMao::Config::ConfigException::what(void) const noexcept
{
	return this->reason.c_str();
}

std::shared_ptr<std::shared_mutex> LiMao::Config::GlobalFileRWLock::GetMutex(const std::string& path)
{
	return rw_lock.GetMutex(path);
}
